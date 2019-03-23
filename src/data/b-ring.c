/*
 * b-vector-ring.c :
 *
 * Copyright (C) 2016 Scott O. Johnson (scojo202@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include "b-ring.h"
#include <math.h>

#include <string.h>
#include <errno.h>
#include <stdlib.h>

/**
 * SECTION: b-ring
 * @short_description: Data objects that grow up to a maximum size.
 *
 * #BRingVector and #BRingMatrix are data objects that can grow
 * element-by-element or row-by-row up to a maximum size, at which point they
 * become rings, where new elements or rows displace the oldest elements or
 * rows.
 *
 * The append() methods can be used to add elements or rows. Alternatively,
 * #BScalar or #BVector objects can be connected as sources. Whenever these
 * emit a "changed" signal, the new value is appended to the #BRingVector or
 * #BRingMatrix, respectively.
 *
 */

/**
 * BRingVector:
 *
 * A BVector that grows up to a maximum length @nmax.
 **/

struct _BRingVector {
	BVector base;
	unsigned n;
	unsigned int nmax;
	double *val;
	BScalar *source;
	gulong handler;
	BRingVector *timestamps;
};

G_DEFINE_TYPE(BRingVector, b_ring_vector, B_TYPE_VECTOR);

static void b_ring_vector_finalize(GObject * obj)
{
	BRingVector *vec = (BRingVector *) obj;
	if (vec->val)
		g_free(vec->val);
	if (vec->source) {
		g_object_unref(vec->source);
		g_signal_handler_disconnect(vec->source, vec->handler);
	}

	GObjectClass *obj_class = G_OBJECT_CLASS(b_ring_vector_parent_class);

	(*obj_class->finalize) (obj);
}

static BData *b_ring_vector_dup(BData * src)
{
	BRingVector *dst = g_object_new(G_OBJECT_TYPE(src), NULL);
	BRingVector const *src_val = (BRingVector const *)src;
	dst->val = g_new0(double, src_val->nmax);
	memcpy(dst->val, src_val->val, src_val->n * sizeof(double));
	dst->n = src_val->n;
	return B_DATA(dst);
}

static unsigned int b_ring_vector_load_len(BVector * vec)
{
	return ((BRingVector *) vec)->n;
}

static double *b_ring_vector_load_values(BVector * vec)
{
	BRingVector const *val = (BRingVector const *)vec;

	return val->val;
}

static double b_ring_vector_get_value(BVector * vec, unsigned i)
{
	BRingVector const *val = (BRingVector const *)vec;
	g_return_val_if_fail(val != NULL && val->val != NULL
			     && i < val->n, NAN);
	return val->val[i];
}

static double *
b_ring_vector_replace_cache(BVector *vec, unsigned len)
{
	BRingVector const *r = (BRingVector const *)vec;

	if(len!=r->n) {
		g_warning("Trying to replace cache in BRingVector.");
	}
	return r->val;
}

static void b_ring_vector_class_init(BRingVectorClass * val_klass)
{
	BDataClass *BData_klass = (BDataClass *) val_klass;
	BVectorClass *vector_klass = (BVectorClass *) val_klass;
	GObjectClass *gobject_klass = (GObjectClass *) val_klass;

	gobject_klass->finalize = b_ring_vector_finalize;
	BData_klass->dup = b_ring_vector_dup;
	vector_klass->load_len = b_ring_vector_load_len;
	vector_klass->load_values = b_ring_vector_load_values;
	vector_klass->get_value = b_ring_vector_get_value;
	vector_klass->replace_cache = b_ring_vector_replace_cache;
}

static void b_ring_vector_init(BRingVector * val)
{
}

/**
 * b_ring_vector_new:
 * @nmax: maximum length of array
 * @n: initial length of array
 * @track_timestamps: whether to save timestamps for each element
 *
 * If @n is not zero, elements are initialized to zero.
 *
 * Returns: a #BData
 *
 **/
BData *b_ring_vector_new(unsigned nmax, unsigned n, gboolean track_timestamps)
{
	BRingVector *res = g_object_new(B_TYPE_RING_VECTOR, NULL);
	res->val = g_new0(double, nmax);
	res->n = n;
	res->nmax = nmax;
	if(track_timestamps) {
		res->timestamps = B_RING_VECTOR(g_object_ref_sink(b_ring_vector_new(nmax,n,FALSE)));
	}
	return B_DATA(res);
}

/**
 * b_ring_vector_append :
 * @d: #BRingVector
 * @val: new value
 *
 * Append a new value to the vector.
 *
 **/
void b_ring_vector_append(BRingVector * d, double val)
{
	g_assert(B_IS_RING_VECTOR(d));
	unsigned int l = MIN(d->nmax, b_vector_get_len(B_VECTOR(d)));
	double *frames = d->val;
	if (l < d->nmax) {
		frames[l] = val;
		b_ring_vector_set_length(d, l + 1);
	}
	else if (l == d->nmax) {
		memmove(frames, &frames[1], (l - 1) * sizeof(double));
		frames[l - 1] = val;
	}
	else {
		return;
	}
	if(d->timestamps) {
		b_ring_vector_append(d->timestamps,((double)g_get_real_time())/1e6);
	}
	b_data_emit_changed(B_DATA(d));
}

/**
 * b_ring_vector_append_array :
 * @d: #BRingVector
 * @arr: (array length=len): array
 * @len: array length
 *
 * Append a new array of values @arr to the vector.
 *
 **/
void b_ring_vector_append_array(BRingVector * d, const double *arr, unsigned int len)
{
	g_assert(B_IS_RING_VECTOR(d));
	g_assert(arr);
	g_assert(len>=0);
	unsigned int l = MIN(d->nmax, b_vector_get_len(B_VECTOR(d)));
	double *frames = d->val;
	int i;
	double now = ((double)g_get_real_time())/1e6;
	if (l + len < d->nmax) {
		for (i = 0; i < len; i++) {
			frames[i + l] = arr[i];
			if(d->timestamps) {
				b_ring_vector_append(d->timestamps,now);
			}
		}
		b_ring_vector_set_length(d, l + len);
	}
	/*else {
	   memmove(frames, &frames[1], (l-1)*sizeof(double));
	   frames[l-1]=val;
	   } */
	else
		return;
	b_data_emit_changed(B_DATA(d));
}

static void on_source_changed(BData * data, gpointer user_data)
{
	BRingVector *d = B_RING_VECTOR(user_data);
	BScalar *source = B_SCALAR(data);
	b_ring_vector_append(d, b_scalar_get_value(source));
}

/**
 * b_ring_vector_set_source :
 * @d: #BRingVector
 * @source: (nullable): a #BScalar
 *
 * Set a source for the #BRingVector. When the source emits a "changed" signal,
 * a new value will be appended to the vector.
 **/
void b_ring_vector_set_source(BRingVector * d, BScalar * source)
{
  g_assert(B_IS_RING_VECTOR(d));
  g_return_if_fail(B_IS_SCALAR(source) || source == NULL);
  if (d->source) {
    g_signal_handler_disconnect(d->source, d->handler);
    g_clear_object(&d->source);
  }
  if (B_IS_SCALAR(source)) {
    d->source = g_object_ref_sink(source);
    d->handler =
      g_signal_connect_after(source, "changed", G_CALLBACK(on_source_changed), d);
  }
}

/**
 * b_ring_vector_set_length :
 * @d: #BRingVector
 * @newlength: new length of array
 *
 * Set the current length of the #BRingVector to a new value. If the new
 * length is longer than the previous length, tailing elements are set to
 * zero.
 **/
void b_ring_vector_set_length(BRingVector * d, unsigned int newlength)
{
	g_assert(B_IS_RING_VECTOR(d));
	if (newlength <= d->nmax) {
		d->n = newlength;
		b_data_emit_changed(B_DATA(d));
		if(d->timestamps) {
			b_ring_vector_set_length(d->timestamps,newlength);
		}
	}
	/* TODO: set tailing elements to zero */
}

/**
 * b_ring_vector_set_max_length :
 * @d: #BRingVector
 * @newmax: new length of array
 *
 * Set the maximum length of the #BRingVector to a new value. If the current
 * length is longer than the new maximum, oldest elements are freed so that
 * the current length is equal to the new maximum length.
 **/
void b_ring_vector_set_max_length(BRingVector * d, unsigned int newmax)
{
  g_assert(B_IS_RING_VECTOR(d));
  d->nmax = newmax;
  double *newval = g_new0(double, newmax);
  if (d->n > d->nmax) {
    unsigned int oo = d->n - d->nmax;
    memcpy(newval, &d->val[oo], newmax * sizeof(double));
    d->n = newmax;
  }
  else {
    memcpy(newval, d->val, d->n * sizeof(double));
  }
  g_free(d->val);
  d->val = newval;
  b_data_emit_changed(B_DATA(d)); /* cache address has changed */
}

/**
 * b_ring_vector_get_timestamps :
 * @d: #BRingVector
 *
 * Get timestamps for when each element was added.
 *
 * Returns: (transfer none): The timestamps.
 **/

BRingVector *b_ring_vector_get_timestamps(BRingVector *d)
{
	g_assert(B_IS_RING_VECTOR(d));
	return d->timestamps;
}

/********************************************************************/

/**
 * BRingMatrix:
 *
 * A BMatrix that grows up to a maximum height @rmax.
 **/

struct _BRingMatrix {
	BMatrix base;
	unsigned nr, nc;
	unsigned int rmax;
	double *val;
	BVector *source;
	gulong handler;
	BRingVector *timestamps;
};

G_DEFINE_TYPE(BRingMatrix, b_ring_matrix, B_TYPE_MATRIX);

static void ring_matrix_finalize(GObject * obj)
{
	BRingMatrix *vec = (BRingMatrix *) obj;
	if (vec->val) {
		g_free(vec->val);
	}
	if (vec->source) {
		g_object_unref(vec->source);
		g_signal_handler_disconnect(vec->source, vec->handler);
	}

	GObjectClass *obj_class = G_OBJECT_CLASS(b_ring_matrix_parent_class);

	(*obj_class->finalize) (obj);
}

static BData *ring_matrix_dup(BData * src)
{
	BRingMatrix *dst = g_object_new(G_OBJECT_TYPE(src), NULL);
	BRingMatrix const *src_val = (BRingMatrix const *)src;
	dst->val = g_new(double, src_val->nc*src_val->rmax);
	memcpy(dst->val, src_val->val, src_val->nc*src_val->nr * sizeof(double));
	dst->nr = src_val->nr;
	dst->nc = src_val->nc;
	return B_DATA(dst);
}

static BMatrixSize ring_matrix_load_size(BMatrix * mat)
{
	BRingMatrix *ring = (BRingMatrix *) mat;
	BMatrixSize s;
	s.rows = ring->nr;
	s.columns = ring->nc;
	return s;
}

static double *ring_matrix_load_values(BMatrix * vec)
{
	BRingMatrix const *val = (BRingMatrix const *)vec;

	return val->val;
}

static double ring_matrix_get_value(BMatrix * vec, unsigned i, unsigned j)
{
	BRingMatrix const *val = (BRingMatrix const *)vec;
	g_return_val_if_fail(val != NULL && val->val != NULL
                         && i < val->nr && j<val->nc, NAN);
	return val->val[i * val->nc + j];
}

static double *
b_ring_matrix_replace_cache(BMatrix *mat, unsigned len)
{
	BRingMatrix const *r = (BRingMatrix const *)mat;

	if(len!=r->nr*r->nc) {
		g_warning("Trying to replace cache in BRingMatrix.");
	}
	return r->val;
}

static void b_ring_matrix_class_init(BRingMatrixClass * val_klass)
{
	BDataClass *BData_klass = (BDataClass *) val_klass;
	BMatrixClass *matrix_klass = (BMatrixClass *) val_klass;
	GObjectClass *gobject_klass = (GObjectClass *) val_klass;

	gobject_klass->finalize = ring_matrix_finalize;
	BData_klass->dup = ring_matrix_dup;
	matrix_klass->load_size = ring_matrix_load_size;
	matrix_klass->load_values = ring_matrix_load_values;
	matrix_klass->get_value = ring_matrix_get_value;
	matrix_klass->replace_cache = b_ring_matrix_replace_cache;
}

static void b_ring_matrix_init(BRingMatrix * val)
{
}

/**
 * b_ring_matrix_new:
 * @c: number of columns
 * @rmax: maximum number of rows
 * @r: initial number of rows
 * @track_timestamps: whether to save timestamps for each element
 *
 * If @r is not zero, elements are initialized to zero.
 *
 * Returns: a #BData
 *
 **/
BData *b_ring_matrix_new(unsigned int c, unsigned int rmax, unsigned int r, gboolean track_timestamps)
{
	BRingMatrix *res = g_object_new(B_TYPE_RING_MATRIX, NULL);
	res->val = g_new0(double, rmax*c);
	res->nr = r;
	res->nc = c;
	res->rmax = rmax;
	if(track_timestamps) {
		res->timestamps = B_RING_VECTOR(g_object_ref_sink(b_ring_vector_new(rmax,r,FALSE)));
	}
	return B_DATA(res);
}

/**
 * b_ring_matrix_append :
 * @d: #BRingMatrix
 * @values: (array length=len): array
 * @len: array length
 *
 * Append a new row to the matrix.
 *
 **/
void b_ring_matrix_append(BRingMatrix * d, const double *values, unsigned int len)
{
	g_assert(B_IS_RING_MATRIX(d));
	g_assert(values);
	g_return_if_fail(len<=d->nc);
	unsigned int l = MIN(d->rmax, b_matrix_get_rows(B_MATRIX(d)));
	double *frames = d->val;
	int k;
	if (l < d->rmax) {
		for(k=0;k<len;k++) {
			frames[l*d->nc+k] = values[k];
		}
		b_ring_matrix_set_rows(d, l + 1);
	}
	else if (l == d->rmax) {
		memmove(frames, &frames[d->nc], (l - 1) * d->nc*sizeof(double));
		for(k=0;k<len;k++) {
			frames[(l-1)*d->nc+k] = values[k];
		}
	}
	else {
			return;
	}
	if(d->timestamps) {
		b_ring_vector_append(d->timestamps,((double)g_get_real_time())/1e6);
	}
	b_data_emit_changed(B_DATA(d));
}

static void on_vector_source_changed(BData * data, gpointer user_data)
{
	BRingMatrix *d = B_RING_MATRIX(user_data);
	BVector *source = B_VECTOR(data);
	b_ring_matrix_append(d, b_vector_get_values(source), b_vector_get_len(source));
}

/**
 * b_ring_matrix_set_source :
 * @d: #BRingMatrix
 * @source: (nullable): a #BVector or %NULL
 *
 * Set a source for the #BRingMatrix. When the source emits a "changed" signal,
 * a new row will be appended to the matrix.
 **/
void b_ring_matrix_set_source(BRingMatrix * d, BVector * source)
{
  g_assert(B_IS_RING_MATRIX(d));
  g_return_if_fail(B_IS_VECTOR(source) || source == NULL);
  if (d->source) {
    g_signal_handler_disconnect(d->source, d->handler);
    g_clear_object(&d->source);
  }
  if (B_IS_VECTOR(source)) {
    d->source = g_object_ref_sink(source);
    d->handler = g_signal_connect_after(source, "changed",
                             G_CALLBACK(on_vector_source_changed), d);
  }
}

/**
 * b_ring_matrix_set_rows :
 * @d: #BRingMatrix
 * @r: new number of rows
 *
 * Set the current height of the #BRingMatrix to a new value. If the new
 * height is greater than the previous length, tailing elements are set to
 * zero.
 **/
void b_ring_matrix_set_rows(BRingMatrix * d, unsigned int r)
{
	g_assert(B_IS_RING_MATRIX(d));
	if (r <= d->rmax) {
		d->nr = r;
		b_data_emit_changed(B_DATA(d));
		if(d->timestamps) {
			b_ring_vector_set_length(d->timestamps,r);
		}
	}
}

/**
 * b_ring_matrix_set_max_rows :
 * @d: #BRingMatrix
 * @rmax: new maximum number of rows
 *
 * Set the maximum height of the #BRingMatrix to a new value.
 **/

void b_ring_matrix_set_max_rows(BRingMatrix *d, unsigned int rmax)
{
	g_assert(B_IS_RING_MATRIX(d));
	if (rmax<d->rmax) { /* don't bother shrinking the array */
		d->rmax = rmax;
		if(d->nr>d->rmax) {
			d->nr=d->rmax;
		}
	}
	else if (rmax>d->rmax) {
		double *a = g_new0(double, rmax*d->nc);
		memcpy(a,d->val,sizeof(double)*d->rmax*d->nc);
		g_free(d->val);
		d->val = a;
		d->rmax = rmax;
	}
	b_data_emit_changed(B_DATA(d)); /* cache address has changed */
}

/**
 * b_ring_matrix_get_timestamps :
 * @d: #BRingMatrix
 *
 * Get timestamps for when each row was added.
 *
 * Returns: (transfer none): The timestamps.
 **/

BRingVector *b_ring_matrix_get_timestamps(BRingMatrix *d)
{
	g_assert(B_IS_RING_MATRIX(d));
	return d->timestamps;
}

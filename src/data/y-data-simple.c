/*
 * y-data-simple.c :
 *
 * Copyright (C) 2003-2005 Jody Goldberg (jody@gnome.org)
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

#include "y-data-simple.h"
#include <math.h>

#include <string.h>
#include <errno.h>
#include <stdlib.h>

/**
 * SECTION: y-data-simple
 * @short_description: Data objects based on simple arrays.
 *
 * Data classes #YValScalar, #YValVector, and #YValMatrix.
 *
 * In these objects, an array (or, in the case of a #YValScalar, a single double
 * precision value) is maintained that is also the data cache. Therefore, the
 * array should not be freed.
 */

/*****************************************************************************/

/**
 * YValVector:
 * @base: base.
 * @n: the length of the vector.
 * @val: the array
 * @notify: the function to call to free the array
 *
 * Object holding a one-dimensional array of double precision numbers.
 **/

struct _YValVector {
	YVector base;
	unsigned n;
	double *val;
	GDestroyNotify notify;
};

G_DEFINE_TYPE(YValVector, y_val_vector, Y_TYPE_VECTOR);

static void y_val_vector_finalize(GObject * obj)
{
	YValVector *vec = (YValVector *) obj;
	if (vec->notify && vec->val)
		(*vec->notify) (vec->val);

	GObjectClass *obj_class = G_OBJECT_CLASS(y_val_vector_parent_class);

	(*obj_class->finalize) (obj);
}

static YData *y_val_vector_dup(YData * src)
{
	YValVector *dst = g_object_new(G_OBJECT_TYPE(src), NULL);
	YValVector const *src_val = (YValVector const *)src;
	if (src_val->notify) {
		dst->val = g_new0(double, src_val->n);
		memcpy(dst->val, src_val->val, src_val->n * sizeof(double));
		dst->notify = g_free;
	} else
		dst->val = src_val->val;
	dst->n = src_val->n;
	return Y_DATA(dst);
}

static unsigned int y_val_vector_load_len(YVector * vec)
{
	return ((YValVector *) vec)->n;
}

static double *y_val_vector_load_values(YVector * vec)
{
	YValVector const *val = (YValVector const *)vec;

	return val->val;
}

static double y_val_vector_get_value(YVector * vec, unsigned i)
{
	YValVector const *val = (YValVector const *)vec;
	g_return_val_if_fail(val != NULL && val->val != NULL
			     && i < val->n, NAN);
	return val->val[i];
}

static double *
y_val_vector_replace_cache(YVector *vec, unsigned len)
{
	YValVector const *val = (YValVector const *)vec;

	if(len!=val->n) {
		g_warning("Trying to replace cache in YValVector.");
	}
	return val->val;
}

static void y_val_vector_class_init(YValVectorClass * val_klass)
{
	YDataClass *ydata_klass = (YDataClass *) val_klass;
	YVectorClass *vector_klass = (YVectorClass *) val_klass;
	GObjectClass *gobject_klass = (GObjectClass *) val_klass;

	gobject_klass->finalize = y_val_vector_finalize;
	ydata_klass->dup = y_val_vector_dup;
	vector_klass->load_len = y_val_vector_load_len;
	vector_klass->load_values = y_val_vector_load_values;
	vector_klass->get_value = y_val_vector_get_value;
	vector_klass->replace_cache = y_val_vector_replace_cache;
}

static void y_val_vector_init(YValVector * val)
{
}

/**
 * y_val_vector_new: (skip)
 * @val: (array length=n): array of doubles
 * @n: length of array
 * @notify: (nullable): the function to be called to free the array when the #YData is unreferenced, or %NULL
 *
 * Create a new #YValVector from an existing array.
 *
 * Returns: a #YData
 **/

YData *y_val_vector_new(double *val, unsigned n, GDestroyNotify notify)
{
	YValVector *res = g_object_new(Y_TYPE_VAL_VECTOR, NULL);
	res->val = val;
	res->n = n;
	res->notify = notify;
	return Y_DATA(res);
}

/**
 * y_val_vector_new_alloc:
 * @n: length of array
 *
 * Create a new #YValVector of length @n, initialized to zeros.
 *
 * Returns: a #YData
 **/
YData *y_val_vector_new_alloc(unsigned n)
{
	YValVector *res = g_object_new(Y_TYPE_VAL_VECTOR, NULL);
	res->val = g_malloc0(sizeof(double) * n);
	res->n = n;
	res->notify = g_free;
	return Y_DATA(res);
}

/**
 * y_val_vector_new_copy:
 * @val: (array length=n): array of doubles
 * @n: length of array
 *
 * Create a new #YValVector, copying from an existing array.
 *
 * Returns: a #YData
 **/

YData *y_val_vector_new_copy(const double *val, unsigned n)
{
	g_assert(val!=NULL);
	double *val2 = g_memdup(val, sizeof(double) * n);
	return y_val_vector_new(val2, n, g_free);
}

/**
 * y_val_vector_replace_array :
 * @s: #YValVector
 * @array: (array length=n): array of doubles
 * @n: length of array
 * @notify: (nullable): the function to be called to free the array when the #YData is unreferenced, or %NULL
 *
 * Replace the array of values of @s.
 **/
void y_val_vector_replace_array(YValVector * s, double *array, unsigned n,
				GDestroyNotify notify)
{
	g_assert(Y_IS_VAL_VECTOR(s));
	if (s->val && s->notify)
		(*s->notify) (s->val);
	s->val = array;
	s->n = n;
	s->notify = notify;
	y_data_emit_changed(Y_DATA(s));
}

/**
 * y_val_vector_get_array :
 * @s: #YValVector
 *
 * Get the array of values of @vec.
 *
 * Returns: an array. Should not be freed.
 **/
double *y_val_vector_get_array(YValVector * s)
{
	g_assert(Y_IS_VAL_VECTOR(s));
	return s->val;
}

/*****************************************************************************/

/**
 * YValMatrix:
 * @base: base.
 * @size: the size of the matrix.
 * @val: the array
 * @notify: the function to call to free the array
 *
 * Object holding a two-dimensional array of double precision numbers.
 **/

struct _YValMatrix {
	YMatrix base;
	YMatrixSize size;
	double *val;
	GDestroyNotify notify;
};

G_DEFINE_TYPE(YValMatrix, y_val_matrix, Y_TYPE_MATRIX);

static void y_val_matrix_finalize(GObject * obj)
{
	YValMatrix *mat = (YValMatrix *) obj;
	if (mat->notify && mat->val)
		(*mat->notify) (mat->val);

	G_OBJECT_CLASS(y_val_matrix_parent_class)->finalize(obj);
}

static YData *y_val_matrix_dup(YData * src)
{
	YValMatrix *dst = g_object_new(G_OBJECT_TYPE(src), NULL);
	YValMatrix const *src_val = (YValMatrix const *)src;
	if (src_val->notify) {
		dst->val =
		    g_new(double, src_val->size.rows * src_val->size.columns);
		memcpy(dst->val, src_val->val,
		       src_val->size.rows * src_val->size.columns *
		       sizeof(double));
		dst->notify = g_free;
	} else
		dst->val = src_val->val;
	dst->size = src_val->size;
	return Y_DATA(dst);
}

static YMatrixSize y_val_matrix_load_size(YMatrix * mat)
{
	return ((YValMatrix *) mat)->size;
}

static double *y_val_matrix_load_values(YMatrix * mat)
{
	YValMatrix const *val = (YValMatrix const *)mat;
	return val->val;
}

static double y_val_matrix_get_value(YMatrix * mat, unsigned i, unsigned j)
{
	YValMatrix const *val = (YValMatrix const *)mat;

	return val->val[i * val->size.columns + j];
}

static double *
y_val_matrix_replace_cache(YMatrix *mat, unsigned len)
{
	YValMatrix const *val = (YValMatrix const *)mat;

	if(len!=val->size.rows*val->size.columns) {
		g_warning("Trying to replace cache in YValMatrix.");
	}
	return val->val;
}

static void y_val_matrix_class_init(YValMatrixClass * val_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) val_klass;
	YDataClass *ydata_klass = (YDataClass *) gobject_klass;
	YMatrixClass *matrix_klass = (YMatrixClass *) gobject_klass;

	gobject_klass->finalize = y_val_matrix_finalize;
	ydata_klass->dup = y_val_matrix_dup;
	matrix_klass->load_size = y_val_matrix_load_size;
	matrix_klass->load_values = y_val_matrix_load_values;
	matrix_klass->get_value = y_val_matrix_get_value;
	matrix_klass->replace_cache = y_val_matrix_replace_cache;
}

static void y_val_matrix_init(YValMatrix * val)
{
}

/**
 * y_val_matrix_new: (skip)
 * @val: array of doubles
 * @rows: number of rows
 * @columns: number of columns
 * @notify: (nullable): the function to be called to free the array when the #YData is unreferenced, or %NULL
 *
 * Create a new #YValMatrix using an existing array.
 *
 * Returns: a #YData
 **/
YData *y_val_matrix_new(double *val, unsigned rows, unsigned columns,
			GDestroyNotify notify)
{
	YValMatrix *res = g_object_new(Y_TYPE_VAL_MATRIX, NULL);
	res->val = val;
	res->size.rows = rows;
	res->size.columns = columns;
	res->notify = notify;
	return Y_DATA(res);
}

/**
 * y_val_matrix_new_copy:
 * @val: array of doubles with at least @rows*@columns elements
 * @rows: number of rows
 * @columns: number of columns
 *
 * Create a new #YValMatrix, copying from an existing array.
 *
 * Returns: a #YData
 **/
YData *y_val_matrix_new_copy(const double *val, unsigned rows, unsigned columns)
{
	g_assert(val!=NULL);
	return y_val_matrix_new(g_memdup(val, sizeof(double) * rows * columns),
				rows, columns, g_free);
}

/**
 * y_val_matrix_new_alloc:
 * @rows: number of rows
 * @columns: number of columns
 *
 * Allocate a new array with @rows rows and @columns columns and use it in a new #YValMatrix.
 *
 * Returns: a #YData
 **/
YData *y_val_matrix_new_alloc(unsigned rows, unsigned columns)
{
	YValMatrix *res = g_object_new(Y_TYPE_VAL_MATRIX, NULL);
	res->val = g_new0(double, rows * columns);
	res->size.rows = rows;
	res->size.columns = columns;
	res->notify = g_free;
	return Y_DATA(res);
}

/**
 * y_val_matrix_get_array :
 * @s: #YValVector
 *
 * Get the array of values of @s.
 *
 * Returns: an array. Should not be freed.
 **/

double *y_val_matrix_get_array(YValMatrix * s)
{
	g_assert(Y_IS_VAL_MATRIX(s));
	return s->val;
}

/**
 * y_val_matrix_replace_array : (skip)
 * @s: #YValMatrix
 * @array: array of doubles
 * @rows: number of rows
 * @columns: number of columns
 * @notify: (nullable): the function to be called to free the array when the #YData is unreferenced, or %NULL
 *
 * Get the array of values of @s.
 *
 **/
void y_val_matrix_replace_array(YValMatrix * s, double *array, unsigned rows,
				unsigned columns, GDestroyNotify notify)
{
	g_assert(Y_IS_VAL_MATRIX(s));
	if (s->val && s->notify)
		(*s->notify) (s->val);
	s->val = array;
	s->size.rows = rows;
	s->size.columns = columns;
	s->notify = notify;
	y_data_emit_changed(Y_DATA(s));
}

/********************************************/

/**
 * y_data_dup_to_simple:
 * @src: #YData
 *
 * Duplicates a #YData object.
 *
 * Returns: (transfer full): A deep copy of @src.
 **/
YData *y_data_dup_to_simple(YData * src)
{
	g_assert(Y_IS_DATA(src));
	YData *d = NULL;
	if (Y_IS_SCALAR(src)) {
		double v = y_scalar_get_value(Y_SCALAR(src));
		d = Y_DATA(y_val_scalar_new(v));
	} else if (Y_IS_VECTOR(src)) {
		const double *v = y_vector_get_values(Y_VECTOR(src));
		d = Y_DATA(y_val_vector_new_copy
			(v, y_vector_get_len(Y_VECTOR(src))));
	} else if (Y_IS_MATRIX(src)) {
		const double *v = y_matrix_get_values(Y_MATRIX(src));
		YMatrixSize s = y_matrix_get_size(Y_MATRIX(src));
		d = Y_DATA(y_val_matrix_new_copy(v, s.rows, s.columns));
	} else if (Y_IS_THREE_D_ARRAY(src)) {
		g_error("dup to simple not yet implemented for 3D arrays.");
	}
	return d;
}

/*****************************************************************************/

/**
 * YValThreeDArray:
 * @base: base.
 * @size: the length of the vector.
 * @val: the array
 * @notify: the function to call to free the array
 *
 * Object holding a three-dimensional array of double precision numbers.
 **/

struct _YValThreeDArray {
	YThreeDArray base;
	YThreeDArraySize size;
	double *val;
	GDestroyNotify notify;
};

G_DEFINE_TYPE(YValThreeDArray, y_val_three_d_array, Y_TYPE_THREE_D_ARRAY);

static void y_val_three_d_array_finalize(GObject * obj)
{
	YValThreeDArray *mat = (YValThreeDArray *) obj;
	if (mat->notify && mat->val)
		(*mat->notify) (mat->val);

	G_OBJECT_CLASS(y_val_three_d_array_parent_class)->finalize(obj);
}

static YData *y_val_three_d_array_dup(YData * src)
{
	YValThreeDArray *dst = g_object_new(G_OBJECT_TYPE(src), NULL);
	YValThreeDArray const *src_val = (YValThreeDArray const *)src;
	if (src_val->notify) {
		dst->val =
		    g_new(double,
			  src_val->size.rows * src_val->size.columns *
			  src_val->size.layers);
		memcpy(dst->val, src_val->val,
		       src_val->size.rows * src_val->size.columns *
		       src_val->size.layers * sizeof(double));
		dst->notify = g_free;
	} else
		dst->val = src_val->val;
	dst->size = src_val->size;
	return Y_DATA(dst);
}

static YThreeDArraySize y_val_three_d_array_load_size(YThreeDArray * mat)
{
	return ((YValThreeDArray *) mat)->size;
}

static double *y_val_three_d_array_load_values(YThreeDArray * mat)
{
	YValThreeDArray const *val = (YValThreeDArray const *)mat;
	return val->val;
}

static double
y_val_three_d_array_get_value(YThreeDArray * mat, unsigned i, unsigned j,
			      unsigned k)
{
	YValThreeDArray const *val = (YValThreeDArray const *)mat;

	return val->val[i * val->size.columns * val->size.rows +
			j * val->size.columns + k];
}

static void y_val_three_d_array_class_init(YValThreeDArrayClass * val_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) val_klass;
	YDataClass *ydata_klass = (YDataClass *) gobject_klass;
	YThreeDArrayClass *matrix_klass = (YThreeDArrayClass *) gobject_klass;

	gobject_klass->finalize = y_val_three_d_array_finalize;
	ydata_klass->dup = y_val_three_d_array_dup;
	matrix_klass->load_size = y_val_three_d_array_load_size;
	matrix_klass->load_values = y_val_three_d_array_load_values;
	matrix_klass->get_value = y_val_three_d_array_get_value;
}

static void y_val_three_d_array_init(YValThreeDArray * val)
{
}

/**
 * y_val_three_d_array_new: (skip)
 * @val: array of doubles
 * @rows: number of rows
 * @columns: number of columns
 * @layers: number of layers
 * @notify: (nullable): the function to be called to free the array when the #YData is unreferenced, or %NULL
 *
 * Create a new #YThreeDArray from an existing array.
 *
 * Returns: a #YData
 **/
YData *y_val_three_d_array_new(double *val, unsigned rows, unsigned columns,
			       unsigned layers, GDestroyNotify notify)
{
	YValThreeDArray *res = g_object_new(Y_TYPE_VAL_THREE_D_ARRAY, NULL);
	res->val = val;
	res->size.rows = rows;
	res->size.columns = columns;
	res->size.layers = layers;
	res->notify = notify;
	return Y_DATA(res);
}

/**
 * y_val_three_d_array_new_copy:
 * @val: array of doubles with at least @rows*@columns elements
 * @rows: number of rows
 * @columns: number of columns
 * @layers: number of layers
 *
 * Create a new #YThreeDArray, making a copy of an existing array.
 *
 * Returns: a #YData
 **/
YData *y_val_three_d_array_new_copy(double *val,
				    unsigned rows, unsigned columns,
				    unsigned layers)
{
	return
	    y_val_three_d_array_new(g_memdup
				    (val,
				     sizeof(double) * rows * columns * layers),
				    rows, columns, layers, g_free);
}

/**
 * y_val_three_d_array_new_alloc:
 * @rows: number of rows
 * @columns: number of columns
 * @layers: number of layers
 *
 * Allocate a new array with @rows rows and @columns columns and use it in a new #YValThreeDArray.
 *
 * Returns: a #YData
 **/
YData *y_val_three_d_array_new_alloc(unsigned rows, unsigned columns,
				     unsigned layers)
{
	YValThreeDArray *res = g_object_new(Y_TYPE_VAL_THREE_D_ARRAY, NULL);
	res->val = g_new0(double, rows * columns);
	res->size.rows = rows;
	res->size.columns = columns;
	res->size.layers = layers;
	res->notify = g_free;
	return Y_DATA(res);
}

/**
 * y_val_three_d_array_get_array :
 * @s: #YValThreeDArray
 *
 * Get the array of values of @s.
 *
 * Returns: an array. Should not be freed.
 **/

double *y_val_three_d_array_get_array(YValThreeDArray * s)
{
	return s->val;
}

/*
 * b-linear-range.c :
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

#include "b-linear-range.h"
#include <math.h>

/**
 * SECTION: b-linear-range
 * @short_description: Vector for equally spaced data.
 *
 * A vector y_i = v_0 + i*dv, where i ranges from 0 to n-1.
 */

struct _BLinearRangeVector {
  BVector	 base;
  double v0;
  double dv;
  unsigned n;
};

G_DEFINE_TYPE (BLinearRangeVector, b_linear_range_vector, B_TYPE_VECTOR);

static GObjectClass *vector_parent_klass;

static BData *
linear_range_vector_dup (BData *src)
{
  BLinearRangeVector *dst = g_object_new (G_OBJECT_TYPE (src), NULL);
  BLinearRangeVector const *src_val = (BLinearRangeVector const *)src;
  dst->v0 = src_val->v0;
  dst->dv = src_val->dv;
  b_linear_range_vector_set_length(dst, src_val->n);
  return B_DATA (dst);
}

static unsigned int
linear_range_vector_load_len (BVector *vec)
{
  return ((BLinearRangeVector *)vec)->n;
}

#define get_val(d,i) (d->v0+i*(d->dv))

static double *
linear_range_vector_load_values (BVector *vec)
{
  BLinearRangeVector *val = (BLinearRangeVector *)vec;
  int i = val->n;

  g_assert(isfinite(val->v0));
  g_assert(isfinite(val->dv));

  double *values = b_vector_replace_cache(vec,val->n);

  while (i-- > 0) {
    values[i]=get_val(val,i);
  }
  return values;
}

static double
linear_range_vector_get_value (BVector *vec, unsigned i)
{
  BLinearRangeVector const *val = (BLinearRangeVector const *)vec;
  g_return_val_if_fail (val != NULL && i < val->n, NAN);
  return get_val(val,i);
}

static gboolean
linear_range_vector_has_value (BData *dat)
{
  BLinearRangeVector const *val = (BLinearRangeVector const *)dat;
  return (isfinite(val->v0) && isfinite(val->dv));
}

static void
b_linear_range_vector_init(BLinearRangeVector *v) {}

static void
b_linear_range_vector_class_init (BLinearRangeVectorClass *klass)
{
  GObjectClass *gobject_klass = (GObjectClass *) klass;
  BDataClass *data_klass = (BDataClass *) klass;
  data_klass->has_value = linear_range_vector_has_value;
  BVectorClass *vector_klass = (BVectorClass *) klass;

  vector_parent_klass = g_type_class_peek_parent (gobject_klass);
  data_klass->dup	= linear_range_vector_dup;
  vector_klass->load_len    = linear_range_vector_load_len;
  vector_klass->load_values = linear_range_vector_load_values;
  vector_klass->get_value   = linear_range_vector_get_value;
}

/**
 * b_linear_range_vector_set_length :
 * @d: a #BLinearRangeVector
 * @n: length
 *
 * Set the length of @d.
 *
 **/

void b_linear_range_vector_set_length(BLinearRangeVector *d, unsigned int n)
{
  g_return_if_fail(B_IS_LINEAR_RANGE_VECTOR(d));

  if(n!=d->n) {
    d->n = n;
    b_data_emit_changed(B_DATA(d));
  }
}

/**
 * b_linear_range_vector_set_pars :
 * @d: a #BLinearRangeVector
 * @v0: first value
 * @dv: step size
 *
 * Set the initial value @v0 and step size @dv of @d.
 *
 **/

void b_linear_range_vector_set_pars(BLinearRangeVector *d, double v0, double dv)
{
  g_return_if_fail(B_IS_LINEAR_RANGE_VECTOR(d));
  d->v0 = v0;
  d->dv = dv;
  b_data_emit_changed(B_DATA(d));
}

/**
 * b_linear_range_vector_set_v0 :
 * @d: a #BLinearRangeVector
 * @v0: first value
 *
 * Set the initial value @v0 of @d.
 *
 **/

void b_linear_range_vector_set_v0(BLinearRangeVector *d, double v0)
{
  g_return_if_fail(B_IS_LINEAR_RANGE_VECTOR(d));
  d->v0 = v0;
  b_data_emit_changed(B_DATA(d));
}

/**
 * b_linear_range_vector_set_dv :
 * @d: a #BLinearRangeVector
 * @dv: step size
 *
 * Set the step size @dv of @d.
 *
 **/

void b_linear_range_vector_set_dv(BLinearRangeVector *d, double dv)
{
  g_return_if_fail(B_IS_LINEAR_RANGE_VECTOR(d));
  d->dv = dv;
  b_data_emit_changed(B_DATA(d));
}

double b_linear_range_vector_get_v0(BLinearRangeVector *d)
{
  g_return_val_if_fail(B_IS_LINEAR_RANGE_VECTOR(d),NAN);
  return d->v0;
}

double b_linear_range_vector_get_dv(BLinearRangeVector *d)
{
  g_return_val_if_fail(B_IS_LINEAR_RANGE_VECTOR(d),NAN);
  return d->dv;
}

/**
 * b_linear_range_vector_new :
 * @v0: first value
 * @dv: step size
 * @n: length
 *
 * Create a new #BLinearRangeVector.
 *
 * Returns: a new #BLinearRangeVector as a #BData
 **/

BData *
b_linear_range_vector_new (double v0, double dv, unsigned n)
{
  BLinearRangeVector *res = g_object_new (B_TYPE_LINEAR_RANGE_VECTOR, NULL);
  res->v0 = v0;
  res->dv = dv;
  b_linear_range_vector_set_length(res,n);
  return B_DATA (res);
}

/******************************************************************/

struct _BFourierLinearRangeVector {
  BVector     base;
  BLinearRangeVector *range;
  unsigned int n;
  gboolean inverse;
};

G_DEFINE_TYPE (BFourierLinearRangeVector, b_fourier_linear_range_vector, B_TYPE_VECTOR);

static void
fourier_linear_range_vector_finalize (GObject *obj)
{
  BFourierLinearRangeVector *vec = (BFourierLinearRangeVector *)obj;

  g_signal_handlers_disconnect_by_data(vec->range, obj);
  g_object_unref(vec->range);

  (*vector_parent_klass->finalize) (obj);
}

static BData *
fourier_linear_range_vector_dup (BData *src)
{
  BFourierLinearRangeVector *dst = g_object_new (G_OBJECT_TYPE (src), NULL);
  BFourierLinearRangeVector const *src_val = (BFourierLinearRangeVector const *)src;
  dst->range = g_object_ref(src_val->range);
  return B_DATA (dst);
}

static unsigned int
fourier_linear_range_vector_load_len (BVector *vec)
{
  BFourierLinearRangeVector *f = (BFourierLinearRangeVector *) vec;
  return f->range->n/2 + 1;
}

static double *
fourier_linear_range_vector_load_values (BVector *vec)
{
  BFourierLinearRangeVector *val = (BFourierLinearRangeVector *)vec;
  BLinearRangeVector *range = val->range;
  int i = range->n/2 + 1;

  g_assert(isfinite(range->v0));
  g_assert(isfinite(range->dv));
  if(range->n ==0 )
    return NULL;

  double *values = b_vector_replace_cache(vec,range->n/2+1);

  double df = 1./range->n/range->dv;
  if(val->inverse) {
    df *= 2*G_PI;
  }

  while (i-- > 0) {
    values[i]=i*df;
  }
  return values;
}

static double
fourier_linear_range_vector_get_value (BVector *vec, unsigned i)
{
  BFourierLinearRangeVector const *val = (BFourierLinearRangeVector const *)vec;
  BLinearRangeVector *range = val->range;

  double df = 1./range->n/range->dv;
  if(val->inverse) {
    df *= 2*G_PI;
  }
  return i*df;
}

static gboolean
fourier_linear_range_vector_has_value (BData *dat)
{
  BFourierLinearRangeVector const *val = (BFourierLinearRangeVector const *)dat;
  return linear_range_vector_has_value(B_DATA(val->range));
}

static void
b_fourier_linear_range_vector_init(BFourierLinearRangeVector *v) {}

static void
b_fourier_linear_range_vector_class_init (BFourierLinearRangeVectorClass *klass)
{
  GObjectClass *gobject_klass = (GObjectClass *) klass;
  BDataClass *ydata_klass = (BDataClass *) klass;
  ydata_klass->has_value = fourier_linear_range_vector_has_value;
  BVectorClass *vector_klass = (BVectorClass *) klass;

  vector_parent_klass = g_type_class_peek_parent (gobject_klass);
  gobject_klass->finalize = fourier_linear_range_vector_finalize;
  ydata_klass->dup    = fourier_linear_range_vector_dup;
  vector_klass->load_len    = fourier_linear_range_vector_load_len;
  vector_klass->load_values = fourier_linear_range_vector_load_values;
  vector_klass->get_value   = fourier_linear_range_vector_get_value;
}

static void
on_range_changed (BData *d, gpointer user_data)
{
  BData *dat = B_DATA(user_data);
  BFourierLinearRangeVector *res = B_FOURIER_LINEAR_RANGE_VECTOR(user_data);
  g_return_if_fail(B_IS_LINEAR_RANGE_VECTOR(res->range));
  g_return_if_fail(d == B_DATA(res->range));
  if(res->n != res->range->n/2 + 1) {
    res->n = res->range->n/2 + 1;
  }
  b_data_emit_changed(dat);
}

void
b_fourier_linear_range_vector_set_inverse(BFourierLinearRangeVector *v, gboolean val)
{
  g_return_if_fail(B_IS_FOURIER_LINEAR_RANGE_VECTOR(v));
  if(v->inverse!=val) {
    v->inverse = val;
    b_data_emit_changed(B_DATA(v));
  }
}

/**
 * b_fourier_linear_range_vector_new :
 * @v: real space range
 *
 * Create a new #BFourierLinearRangeVector connected to @v.
 *
 * Returns: a new #BFourierLinearRangeVector as a #BData
 **/

BData *
b_fourier_linear_range_vector_new (BLinearRangeVector *v)
{
  BFourierLinearRangeVector *res = g_object_new (B_TYPE_FOURIER_LINEAR_RANGE_VECTOR, NULL);
  g_return_val_if_fail(B_IS_LINEAR_RANGE_VECTOR(v),NULL);
  res->range = g_object_ref_sink(v);
  res->n = res->range->n/2 + 1;
  g_signal_connect_after(res->range,"changed",G_CALLBACK(on_range_changed),res);
  return B_DATA (res);
}

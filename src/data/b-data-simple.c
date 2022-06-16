/*
 * b-data-simple.c :
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

#include "b-data-simple.h"
#include <math.h>

#include <string.h>
#include <errno.h>
#include <stdlib.h>

/**
 * SECTION: b-data-simple
 * @short_description: Data objects based on simple arrays.
 *
 * Data classes #BValScalar, #BValVector, and #BValMatrix. These are the trivial
 * implementations of the abstract data classes #BScalar, #BVector, and
 * #BMatrix.
 *
 * In these objects, an array (or, in the case of a #BValScalar, a single double
 * precision value) is maintained that also serves as the data cache. Therefore,
 * this array should not be freed.
 *
 * The get_values() methods can be used to get a const version of the array. To
 * get a modifiable version, use the get_array() methods for #BValVector and
 * #BValMatrix.
 */

/*****************************************************************************/

/**
 * BValVector:
 *
 * Object holding a one-dimensional array of double precision numbers.
 **/

struct _BValVector
{
  BVector base;
  guint n;
  double *val;
  GDestroyNotify notify;
};

G_DEFINE_TYPE (BValVector, b_val_vector, B_TYPE_VECTOR);

static void
b_val_vector_finalize (GObject * obj)
{
  BValVector *vec = (BValVector *) obj;
  if (vec->notify && vec->val)
    (*vec->notify) (vec->val);

  GObjectClass *obj_class = G_OBJECT_CLASS (b_val_vector_parent_class);

  (*obj_class->finalize) (obj);
}

static BData *
b_val_vector_dup (BData * src)
{
  BValVector *dst = g_object_new (G_OBJECT_TYPE (src), NULL);
  BValVector const *src_val = (BValVector const *) src;
  if (src_val->notify)
    {
      dst->val = g_new0 (double, src_val->n);
      memcpy (dst->val, src_val->val, src_val->n * sizeof (double));
      dst->notify = g_free;
    }
  else
    dst->val = src_val->val;
  dst->n = src_val->n;
  return B_DATA (dst);
}

static guint
b_val_vector_load_len (BVector * vec)
{
  return ((BValVector *) vec)->n;
}

static double *
b_val_vector_load_values (BVector * vec)
{
  BValVector const *val = (BValVector const *) vec;

  return val->val;
}

static double
b_val_vector_get_value (BVector * vec, guint i)
{
  BValVector const *val = (BValVector const *) vec;
  g_return_val_if_fail (val != NULL && val->val != NULL && i < val->n, NAN);
  return val->val[i];
}

static double *
b_val_vector_replace_cache (BVector * vec, guint len)
{
  BValVector const *val = (BValVector const *) vec;

  if (len != val->n)
    {
      g_warning ("Trying to replace cache in BValVector.");
    }
  return val->val;
}

static void
b_val_vector_class_init (BValVectorClass * val_klass)
{
  BDataClass *ydata_klass = (BDataClass *) val_klass;
  BVectorClass *vector_klass = (BVectorClass *) val_klass;
  GObjectClass *gobject_klass = (GObjectClass *) val_klass;

  gobject_klass->finalize = b_val_vector_finalize;
  ydata_klass->dup = b_val_vector_dup;
  vector_klass->load_len = b_val_vector_load_len;
  vector_klass->load_values = b_val_vector_load_values;
  vector_klass->get_value = b_val_vector_get_value;
  vector_klass->replace_cache = b_val_vector_replace_cache;
}

static void
b_val_vector_init (BValVector * val)
{
}

/**
 * b_val_vector_new: (skip)
 * @val: (array length=n): array of doubles
 * @n: length of array
 * @notify: (nullable): the function to be called to free the array when the #BData is unreferenced, or %NULL
 *
 * Create a new #BValVector from an existing array.
 *
 * Returns: a #BData
 **/

BData *
b_val_vector_new (double *val, guint n, GDestroyNotify notify)
{
  BValVector *res = g_object_new (B_TYPE_VAL_VECTOR, NULL);
  res->val = val;
  res->n = n;
  res->notify = notify;
  return B_DATA (res);
}

/**
 * b_val_vector_new_alloc:
 * @n: length of array
 *
 * Create a new #BValVector of length @n, initialized to zeros.
 *
 * Returns: a #BData
 **/
BData *
b_val_vector_new_alloc (guint n)
{
  BValVector *res = g_object_new (B_TYPE_VAL_VECTOR, NULL);
  res->val = g_malloc0 (sizeof (double) * n);
  res->n = n;
  res->notify = g_free;
  return B_DATA (res);
}

/**
 * b_val_vector_new_copy:
 * @val: (array length=n): array of doubles
 * @n: length of array
 *
 * Create a new #BValVector, copying from an existing array.
 *
 * Returns: a #BData
 **/

BData *
b_val_vector_new_copy (const double *val, guint n)
{
  g_return_val_if_fail (val != NULL, NULL);
  double *val2 = g_memdup2 (val, sizeof (double) * n);
  return b_val_vector_new (val2, n, g_free);
}

/**
 * b_val_vector_replace_array :
 * @s: #BValVector
 * @array: (array length=n): array of doubles
 * @n: length of array
 * @notify: (nullable): the function to be called to free the array when the #BData is unreferenced, or %NULL
 *
 * Replace the array of values of @s.
 **/
void
b_val_vector_replace_array (BValVector * s, double *array, guint n,
			    GDestroyNotify notify)
{
  g_return_if_fail (B_IS_VAL_VECTOR (s));
  if (s->val && s->notify)
    (*s->notify) (s->val);
  s->val = array;
  s->n = n;
  s->notify = notify;
  b_data_emit_changed (B_DATA (s));
}

/**
 * b_val_vector_get_array :
 * @s: #BValVector
 *
 * Get the array of values of @vec.
 *
 * Returns: an array. Should not be freed.
 **/
double *
b_val_vector_get_array (BValVector * s)
{
  g_return_val_if_fail (B_IS_VAL_VECTOR (s), NULL);
  return s->val;
}

/*****************************************************************************/

/**
 * BValMatrix:
 *
 * Object holding a two-dimensional array of double precision numbers.
 **/

struct _BValMatrix
{
  BMatrix base;
  BMatrixSize size;
  double *val;
  GDestroyNotify notify;
};

G_DEFINE_TYPE (BValMatrix, b_val_matrix, B_TYPE_MATRIX);

static void
b_val_matrix_finalize (GObject * obj)
{
  BValMatrix *mat = (BValMatrix *) obj;
  if (mat->notify && mat->val)
    (*mat->notify) (mat->val);

  G_OBJECT_CLASS (b_val_matrix_parent_class)->finalize (obj);
}

static BData *
b_val_matrix_dup (BData * src)
{
  BValMatrix *dst = g_object_new (G_OBJECT_TYPE (src), NULL);
  BValMatrix const *src_val = (BValMatrix const *) src;
  if (src_val->notify)
    {
      dst->val = g_new (double, src_val->size.rows * src_val->size.columns);
      memcpy (dst->val, src_val->val,
	      src_val->size.rows * src_val->size.columns * sizeof (double));
      dst->notify = g_free;
    }
  else
    dst->val = src_val->val;
  dst->size = src_val->size;
  return B_DATA (dst);
}

static BMatrixSize
b_val_matrix_load_size (BMatrix * mat)
{
  return ((BValMatrix *) mat)->size;
}

static double *
b_val_matrix_load_values (BMatrix * mat)
{
  BValMatrix const *val = (BValMatrix const *) mat;
  return val->val;
}

static double
b_val_matrix_get_value (BMatrix * mat, guint i, guint j)
{
  BValMatrix const *val = (BValMatrix const *) mat;

  return val->val[i * val->size.columns + j];
}

static double *
b_val_matrix_replace_cache (BMatrix * mat, guint len)
{
  BValMatrix const *val = (BValMatrix const *) mat;

  if (len != val->size.rows * val->size.columns)
    {
      g_warning ("Trying to replace cache in BValMatrix.");
    }
  return val->val;
}

static void
b_val_matrix_class_init (BValMatrixClass * val_klass)
{
  GObjectClass *gobject_klass = (GObjectClass *) val_klass;
  BDataClass *ydata_klass = (BDataClass *) gobject_klass;
  BMatrixClass *matrix_klass = (BMatrixClass *) gobject_klass;

  gobject_klass->finalize = b_val_matrix_finalize;
  ydata_klass->dup = b_val_matrix_dup;
  matrix_klass->load_size = b_val_matrix_load_size;
  matrix_klass->load_values = b_val_matrix_load_values;
  matrix_klass->get_value = b_val_matrix_get_value;
  matrix_klass->replace_cache = b_val_matrix_replace_cache;
}

static void
b_val_matrix_init (BValMatrix * val)
{
}

/**
 * b_val_matrix_new: (skip)
 * @val: array of doubles
 * @rows: number of rows
 * @columns: number of columns
 * @notify: (nullable): the function to be called to free the array when the #BData is unreferenced, or %NULL
 *
 * Create a new #BValMatrix using an existing array.
 *
 * Returns: a #BData
 **/
BData *
b_val_matrix_new (double *val, guint rows, guint columns,
		  GDestroyNotify notify)
{
  BValMatrix *res = g_object_new (B_TYPE_VAL_MATRIX, NULL);
  res->val = val;
  res->size.rows = rows;
  res->size.columns = columns;
  res->notify = notify;
  return B_DATA (res);
}

/**
 * b_val_matrix_new_copy:
 * @val: array of doubles with at least @rows*@columns elements
 * @rows: number of rows
 * @columns: number of columns
 *
 * Create a new #BValMatrix, copying from an existing array.
 *
 * Returns: a #BData
 **/
BData *
b_val_matrix_new_copy (const double *val, guint rows, guint columns)
{
  g_return_val_if_fail (val != NULL, NULL);
  return b_val_matrix_new (g_memdup2 (val, sizeof (double) * rows * columns),
			   rows, columns, g_free);
}

/**
 * b_val_matrix_new_alloc:
 * @rows: number of rows
 * @columns: number of columns
 *
 * Allocate a new array with @rows rows and @columns columns and use it in a new #BValMatrix.
 *
 * Returns: a #BData
 **/
BData *
b_val_matrix_new_alloc (guint rows, guint columns)
{
  BValMatrix *res = g_object_new (B_TYPE_VAL_MATRIX, NULL);
  res->val = g_new0 (double, rows * columns);
  res->size.rows = rows;
  res->size.columns = columns;
  res->notify = g_free;
  return B_DATA (res);
}

/**
 * b_val_matrix_get_array :
 * @s: #BValVector
 *
 * Get the array of values of @s.
 *
 * Returns: an array. Should not be freed.
 **/

double *
b_val_matrix_get_array (BValMatrix * s)
{
  g_return_val_if_fail (B_IS_VAL_MATRIX (s), NULL);
  return s->val;
}

/**
 * b_val_matrix_replace_array : (skip)
 * @s: #BValMatrix
 * @array: array of doubles
 * @rows: number of rows
 * @columns: number of columns
 * @notify: (nullable): the function to be called to free the array when the #BData is unreferenced, or %NULL
 *
 * Get the array of values of @s.
 *
 **/
void
b_val_matrix_replace_array (BValMatrix * s, double *array, guint rows,
			    guint columns, GDestroyNotify notify)
{
  g_return_if_fail (B_IS_VAL_MATRIX (s));
  if (s->val && s->notify)
    (*s->notify) (s->val);
  s->val = array;
  s->size.rows = rows;
  s->size.columns = columns;
  s->notify = notify;
  b_data_emit_changed (B_DATA (s));
}

/********************************************/

/**
 * b_data_dup_to_simple:
 * @src: #BData
 *
 * Duplicates a #BData object, creating a simple data object of the same size
 * and contents. So for example, any subclass of #BVector is duplicated as a
 * #BValVector.
 *
 * Returns: (transfer full): A deep copy of @src.
 **/
BData *
b_data_dup_to_simple (BData * src)
{
  g_return_val_if_fail (B_IS_DATA (src), NULL);
  BData *d = NULL;
  if (B_IS_SCALAR (src))
    {
      double v = b_scalar_get_value (B_SCALAR (src));
      d = B_DATA (b_val_scalar_new (v));
    }
  else if (B_IS_VECTOR (src))
    {
      const double *v = b_vector_get_values (B_VECTOR (src));
      d = B_DATA (b_val_vector_new_copy
		  (v, b_vector_get_len (B_VECTOR (src))));
    }
  else if (B_IS_MATRIX (src))
    {
      const double *v = b_matrix_get_values (B_MATRIX (src));
      BMatrixSize s = b_matrix_get_size (B_MATRIX (src));
      d = B_DATA (b_val_matrix_new_copy (v, s.rows, s.columns));
    }
  return d;
}

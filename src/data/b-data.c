/*
 * b-data.c :
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

#include "b-data-class.h"
#include <math.h>
#include <string.h>
#include <errno.h>

/**
 * SECTION: b-data
 * @short_description: Base class for data objects.
 *
 * Abstract base class for data classes, including #BScalar,
 * #BVector, and #BMatrix, representing single numbers or arrays of numbers,
 * respectively.
 *
 * Data objects can maintain a cache for fast access. When the underlying data
 * changes, the "changed" signal is emitted, and the default signal handler
 * invalidates the cache. Subsequent calls to "get_values" will refill the
 * cache. The size of the array and minimum and maximum values are also cached.
 * Depending on the implementation, the get_value() functions (for getting
 * single values) may not refill the cache.
 *
 * Data objects also maintain a timestamp that updates when the "changed" signal
 * is emitted.
 */

typedef enum
{
  B_DATA_CACHE_IS_VALID = 1 << 0,
  B_DATA_IS_EDITABLE = 1 << 1,
  B_DATA_SIZE_CACHED = 1 << 2,
  B_DATA_HAS_VALUE = 1 << 3,
  B_DATA_MINMAX_CACHED = 1 << 4
} BDataFlags;

typedef struct
{
  guint32 flags;
  gint64 timestamp;
} BDataPrivate;

enum
{
  CHANGED,
  LAST_SIGNAL
};

static gulong b_data_signals[LAST_SIGNAL] = { 0, };

static char *
render_val (double val)
{
  char buf[G_ASCII_DTOSTR_BUF_SIZE];
  g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, val);
  return g_strdup (buf);
}

static char *
format_val (double val, const gchar * format)
{
  char buf[G_ASCII_DTOSTR_BUF_SIZE];
  g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, format, val);
  return g_strdup (buf);
}

/**
 * BData:
 *
 * Object representing data.
 **/

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (BData, b_data, G_TYPE_INITIALLY_UNOWNED);

static void
b_data_init (BData * data)
{
  BDataPrivate *priv = b_data_get_instance_private (data);
  priv->timestamp = g_get_real_time ();
}

static void
b_data_class_init (BDataClass * klass)
{
/**
 * BData::changed:
 * @BData: the data object that changed
 *
 * The ::changed signal is emitted when the data changes.
 */

  b_data_signals[CHANGED] = g_signal_new ("changed",
					  G_TYPE_FROM_CLASS (klass),
					  G_SIGNAL_RUN_FIRST |
					  G_SIGNAL_NO_RECURSE,
					  G_STRUCT_OFFSET (BDataClass,
							   emit_changed),
					  NULL, NULL,
					  g_cclosure_marshal_VOID__VOID,
					  G_TYPE_NONE, 0);

  klass->dup = b_data_dup_to_simple;
}

/**
 * b_data_dup:
 * @src: #BData
 *
 * Duplicates a #BData object.
 *
 * Returns: (transfer full): A deep copy of @src.
 **/
BData *
b_data_dup (BData * src)
{
  g_assert (B_IS_DATA (src));
  if (src != NULL)
    {
      BDataClass *klass = B_DATA_GET_CLASS (src);
      g_return_val_if_fail (klass != NULL, NULL);
      return (*klass->dup) (src);
    }
  return NULL;
}

/**
 * b_data_serialize :
 * @dat: #BData
 * @user: a pointer describing the context.
 *
 * Returns: a string representation of the data that the caller is
 * 	responsible for freeing
 **/
char *
b_data_serialize (BData * dat, gpointer user)
{
  g_assert (B_IS_DATA (dat));
  BDataClass *klass = B_DATA_GET_CLASS (dat);
  g_return_val_if_fail (klass != NULL, NULL);
  return (*klass->serialize) (dat, user);
}

/**
 * b_data_emit_changed :
 * @data: #BData
 *
 * Utility to emit a 'changed' signal
 **/
void
b_data_emit_changed (BData * data)
{
  BDataClass *klass = B_DATA_GET_CLASS (data);

  g_return_if_fail (klass != NULL);

  g_signal_emit (G_OBJECT (data), b_data_signals[CHANGED], 0);
}

/**
 * b_data_get_timestamp :
 * @data: #BData
 *
 * Returns a timestamp (microseconds since January 1, 1970, UTC) giving the last time the data changed.
 **/
gint64
b_data_get_timestamp (BData * data)
{
  g_return_val_if_fail (B_IS_DATA (data), 0);
  BDataPrivate *priv = b_data_get_instance_private (data);
  return priv->timestamp;
}

/**
 * b_data_has_value :
 * @data: #BData
 *
 * Returns whether @data contains a finite value.
 *
 * Returns: TRUE if @data has at least one finite value.
 **/
gboolean
b_data_has_value (BData * data)
{
  g_return_val_if_fail (B_IS_DATA (data), FALSE);
  BDataClass *data_class = B_DATA_GET_CLASS (data);
  BDataPrivate *priv = b_data_get_instance_private (data);
  if (!(priv->flags & B_DATA_HAS_VALUE))
    {
      g_return_val_if_fail (data_class->has_value != NULL, FALSE);
      gboolean has_value = data_class->has_value (data);
      if (has_value)
        priv->flags |= B_DATA_HAS_VALUE;
    }
  return priv->flags & B_DATA_HAS_VALUE;
}

/**
 * b_data_get_n_dimensions :
 * @data: #BData
 *
 * Get the number of dimensions in @data, i.e. 0 for a scalar, 1 for a vector,
 * and 2 for a matrix. Returns -1 for a struct.
 *
 * Returns: the number of dimensions
 **/
char
b_data_get_n_dimensions (BData * data)
{
  BDataClass *data_class;

  g_return_val_if_fail (B_IS_DATA (data), 0);

  data_class = B_DATA_GET_CLASS (data);

  g_return_val_if_fail (data_class->get_sizes != NULL, 0);

  return data_class->get_sizes (data, NULL);
}

/**
 * b_data_get_n_values :
 * @data: #BData
 *
 * Get the number of values in @data.
 *
 * Returns: the number of elements
 **/
unsigned int
b_data_get_n_values (BData * data)
{
  BDataClass const *data_class;
  unsigned int n_values;
  int n_dimensions;
  unsigned int sizes[3];

  g_return_val_if_fail (B_IS_DATA (data), 0);

  data_class = B_DATA_GET_CLASS (data);

  n_dimensions = data_class->get_sizes (data, sizes);

  if (n_dimensions < 1)
    return 1;

  g_assert (n_dimensions < 4);

  g_return_val_if_fail (data_class->get_sizes != NULL, 0);

  n_values = 1;
  for (unsigned int i = 0; i < n_dimensions; i++)
    n_values *= sizes[i];

  return n_values;
}

/*************************************************************************/

/**
 * SECTION: b-scalar
 * @short_description: Base class for scalar data objects.
 *
 * Abstract base class for data classes representing scalar values.
 */

typedef struct
{
  double value;
} BScalarPrivate;

/**
 * BScalar:
 *
 * Object representing a single double-precision number.
 **/

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (BScalar, b_scalar, B_TYPE_DATA);

static gboolean
_data_scalar_has_value (BData * data)
{
  BScalar *scalar = (BScalar *) data;

  double v = b_scalar_get_value (scalar);

  return isfinite (v);
}

static char *
_scalar_serialize (BData * dat, gpointer user)
{
  BScalar *scalar = (BScalar *) dat;
  BScalarPrivate *priv = b_scalar_get_instance_private (scalar);
  return render_val (priv->value);
}

/* TODO: is it really necessary to have one for scalar and one for arrays? */
static void
_data_scalar_emit_changed (BData * data)
{
  BDataPrivate *priv = b_data_get_instance_private (data);
  priv->timestamp = g_get_real_time ();
  priv->flags &= ~(B_DATA_CACHE_IS_VALID | B_DATA_HAS_VALUE);
}

static char
_scalar_get_sizes (BData * data, unsigned int *sizes)
{
  return 0;
}

static void
b_scalar_class_init (BScalarClass * scalar_class)
{
  BDataClass *data_class = B_DATA_CLASS (scalar_class);
  data_class->has_value = _data_scalar_has_value;
  data_class->serialize = _scalar_serialize;
  data_class->emit_changed = _data_scalar_emit_changed;
  data_class->get_sizes = _scalar_get_sizes;
}

static void
b_scalar_init (BScalar * scalar)
{
}

/**
 * b_scalar_get_value :
 * @scalar: #BScalar
 *
 * Get the value of @scalar. If the cache is valid, it will use that.
 * Otherwise, it will call the #BScalar's get_value() method.
 *
 * Returns: the value
 **/
double
b_scalar_get_value (BScalar * scalar)
{
  BScalarClass const *klass = B_SCALAR_GET_CLASS (scalar);
  g_return_val_if_fail (klass != NULL, NAN);
  BDataPrivate *dpriv = b_data_get_instance_private (B_DATA (scalar));
  BScalarPrivate *priv = b_scalar_get_instance_private (scalar);
  if (!(dpriv->flags & B_DATA_CACHE_IS_VALID))
    {
      priv->value = (*klass->get_value) (scalar);
    }
  return priv->value;
}

/**
 * b_scalar_get_str :
 * @scalar: #BScalar
 * @format: a format string to use
 *
 * Get a string representation of @scalar.
 *
 * Returns: the string. The caller is
 * 	responsible for freeing it.
 **/
char *
b_scalar_get_str (BScalar * scalar, const gchar * format)
{
  double val = b_scalar_get_value (scalar);
  return format_val (val, format);
}

/**********************************************************/

/**
 * BValScalar:
 *
 * Object holding a single double precision number.
 **/

struct _BValScalar
{
  BScalar base;
};

G_DEFINE_TYPE (BValScalar, b_val_scalar, B_TYPE_SCALAR);

static BData *
b_val_scalar_dup (BData * src)
{
  BValScalar *dst = g_object_new (G_OBJECT_TYPE (src), NULL);
  BScalarPrivate *priv = b_scalar_get_instance_private (B_SCALAR (src));
  BScalarPrivate *dpriv = b_scalar_get_instance_private (B_SCALAR (dst));
  dpriv->value = priv->value;
  return B_DATA (dst);
}

static double
b_val_scalar_get_value (BScalar * dat)
{
  BScalarPrivate *priv = b_scalar_get_instance_private (B_SCALAR (dat));
  return priv->value;
}

static void
b_val_scalar_class_init (BValScalarClass * scalarval_klass)
{
  BDataClass *data_klass = (BDataClass *) scalarval_klass;
  BScalarClass *scalar_klass = (BScalarClass *) scalarval_klass;

  data_klass->dup = b_val_scalar_dup;
  scalar_klass->get_value = b_val_scalar_get_value;
}

static void
b_val_scalar_init (BValScalar * val)
{
}

/**
 * b_val_scalar_new:
 * @val: initial value
 *
 * Creates a new #BValScalar object.
 *
 * Returns: (transfer full): The new object.
 **/
BData *
b_val_scalar_new (double val)
{
  BValScalar *res = g_object_new (B_TYPE_VAL_SCALAR, NULL);
  BScalarPrivate *priv = b_scalar_get_instance_private (B_SCALAR (res));
  priv->value = val;

  return B_DATA (res);
}

/**
 * b_val_scalar_get_val:
 * @s: a #BValScalar
 *
 * Gets a pointer to the value of a #BValScalar.
 *
 * Returns: (transfer none): A pointer to the scalar value.
 **/
double *
b_val_scalar_get_val (BValScalar * s)
{
  g_assert (B_IS_VAL_SCALAR (s));
  BScalarPrivate *priv = b_scalar_get_instance_private (B_SCALAR (s));
  return &priv->value;
}

/**
 * b_val_scalar_set_val:
 * @s: a #BValScalar
 * @val: the value
 *
 * Sets the value of a #BValScalar.
 **/
void
b_val_scalar_set_val (BValScalar * s, double val)
{
  BScalarPrivate *priv = b_scalar_get_instance_private (B_SCALAR (s));
  priv->value = val;
  b_data_emit_changed (B_DATA (s));
}


/*************************************************************************/

/**
 * SECTION: b-vector
 * @short_description: Base class for one-dimensional array data objects.
 *
 * Abstract base class for data classes representing one dimensional arrays.
 */

typedef struct
{
  unsigned int len;
  double *values;		/* NULL = uninitialized/unsupported, nan = missing */
  double minimum, maximum;
} BVectorPrivate;

/**
 * BVector:
 *
 * Object representing a one-dimensional array of numbers.
 **/

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (BVector, b_vector, B_TYPE_DATA);

static void
_data_array_emit_changed (BData * data)
{
  BDataPrivate *priv = b_data_get_instance_private (data);
  priv->timestamp = g_get_real_time ();
  priv->flags &=
    ~(B_DATA_CACHE_IS_VALID | B_DATA_SIZE_CACHED | B_DATA_HAS_VALUE |
      B_DATA_MINMAX_CACHED);
}

static void
_vector_finalize (GObject * dat)
{
  BVector *vec = (BVector *) dat;
  BVectorPrivate *vpriv = b_vector_get_instance_private (vec);
  BVectorClass *vec_class = B_VECTOR_GET_CLASS (dat);

  if (vec_class->replace_cache == NULL)
    g_clear_pointer(&vpriv->values,g_free);
}

static char
_data_vector_get_sizes (BData * data, unsigned int *sizes)
{
  BVector *vector = (BVector *) data;

  if (sizes != NULL)
    sizes[0] = b_vector_get_len (vector);

  return 1;
}

static gboolean
_vector_has_value (BData * dat)
{
  BVector *vec = (BVector *) dat;
  double minimum, maximum;
  b_vector_get_minmax (vec, &minimum, &maximum);
  return (isfinite (minimum) && isfinite (maximum) && minimum <= maximum);
}

static char *
_vector_serialize (BData * dat, gpointer user)
{
  BVector *vec = (BVector *) dat;
  BVectorPrivate *vpriv = b_vector_get_instance_private (vec);
  GString *str;
  char sep;

  sep = '\t';
  str = g_string_new (NULL);

  for (unsigned int i = 0; i < vpriv->len; i++)
    {
      char *s = render_val (vpriv->values[i]);
      if (i)
        g_string_append_c (str, sep);
      g_string_append (str, s);
      g_free (s);
    }
  return g_string_free (str, FALSE);
}

static void
b_vector_init (BVector * vec)
{
}

static void
b_vector_class_init (BVectorClass * vec_class)
{
  GObjectClass *gobj_class = (GObjectClass *) vec_class;
  BDataClass *data_class = (BDataClass *) vec_class;
  data_class->emit_changed = _data_array_emit_changed;
  data_class->get_sizes = _data_vector_get_sizes;
  data_class->serialize = _vector_serialize;
  data_class->has_value = _vector_has_value;
  gobj_class->finalize = _vector_finalize;
}

/**
 * b_vector_get_len :
 * @vec: #BVector
 *
 * Get the number of values in @vec and caches it.
 *
 * Returns: the length
 **/
unsigned int
b_vector_get_len (BVector * vec)
{
  g_return_val_if_fail (B_IS_VECTOR (vec), 0);
  BData *data = B_DATA (vec);
  BDataPrivate *priv = b_data_get_instance_private (data);
  BVectorPrivate *vpriv = b_vector_get_instance_private (vec);
  if (!(priv->flags & B_DATA_SIZE_CACHED))
    {
      BVectorClass const *klass = B_VECTOR_GET_CLASS (vec);

      g_return_val_if_fail (klass != NULL, 0);

      vpriv->len = (*klass->load_len) (vec);
      priv->flags |= B_DATA_SIZE_CACHED;
    }

  return vpriv->len;
}

/**
 * b_vector_get_values :
 * @vec: #BVector
 *
 * Get the full array of values of @vec and cache them.
 *
 * Returns: an array.
 **/
const double *
b_vector_get_values (BVector * vec)
{
  g_return_val_if_fail (B_IS_VECTOR (vec), NULL);
  BData *data = B_DATA (vec);
  BDataPrivate *priv = b_data_get_instance_private (data);
  BVectorPrivate *vpriv = b_vector_get_instance_private (vec);
  if (!(priv->flags & B_DATA_CACHE_IS_VALID))
    {
      BVectorClass const *klass = B_VECTOR_GET_CLASS (vec);

      g_return_val_if_fail (klass != NULL, NULL);

      vpriv->values = (*klass->load_values) (vec);

      priv->flags |= B_DATA_CACHE_IS_VALID;
    }

  return vpriv->values;
}

/**
 * b_vector_get_value :
 * @vec: #BVector
 * @i: index
 *
 * Get a value in @vec.
 *
 * Returns: the value
 **/
double
b_vector_get_value (BVector * vec, unsigned i)
{
  g_return_val_if_fail (B_IS_VECTOR (vec), NAN);
  BData *data = B_DATA (vec);
  BDataPrivate *priv = b_data_get_instance_private (data);
  unsigned int len = b_vector_get_len (vec);
  g_return_val_if_fail (i < len, NAN);
  if (!(priv->flags & B_DATA_CACHE_IS_VALID))
    {
      BVectorClass const *klass = B_VECTOR_GET_CLASS (vec);
      g_return_val_if_fail (klass != NULL, NAN);
      return (*klass->get_value) (vec, i);
    }
  BVectorPrivate *vpriv = b_vector_get_instance_private (vec);
  return vpriv->values[i];
}

/**
 * b_vector_get_str :
 * @vec: #BVector
 * @i: index
 * @format: a format string
 *
 * Get a string representation of an element in @vec.
 *
 * Returns: the string. The caller is
 * 	responsible for freeing it.
 **/
char *
b_vector_get_str (BVector * vec, unsigned int i, const gchar * format)
{
  g_assert (B_IS_VECTOR (vec));
  double val = b_vector_get_value (vec, i);
  return format_val (val, format);
}

static int
range_increasing (double const *xs, int n)
{
  int i = 0;
  double last;
  g_return_val_if_fail (n == 0 || xs != NULL, 0);
  while (i < n && isnan (xs[i]))
    i++;
  if (i == n)
    return 0;
  last = xs[i];
  for (i = i + 1; i < n; i++)
    {
      if (isnan (xs[i]))
        continue;
      if (last >= xs[i])
        return 0;
      last = xs[i];
    }
  return 1;
}

static int
range_decreasing (double const *xs, int n)
{
  int i = 0;
  double last;
  g_return_val_if_fail (n == 0 || xs != NULL, 0);
  while (i < n && isnan (xs[i]))
    i++;
  if (i == n)
    return 0;
  last = xs[i];
  for (i = i + 1; i < n; i++)
    {
      if (isnan (xs[i]))
        continue;
      if (last <= xs[i])
        return 0;
      last = xs[i];
    }
  return 1;
}

static int
range_vary_uniformly (double const *xs, int n)
{
  return range_increasing (xs, n) || range_decreasing (xs, n);
}

/**
 * b_vector_is_varying_uniformly :
 * @data: #BVector
 *
 * Returns whether elements of @data only increase or only decrease.
 *
 * Returns: TRUE if elements of @data strictly increase or decrease.
 **/
gboolean
b_vector_is_varying_uniformly (BVector * data)
{
  double const *values;
  unsigned int n_values;

  g_return_val_if_fail (B_IS_VECTOR (data), FALSE);

  values = b_vector_get_values (data);
  if (values == NULL)
    return FALSE;

  n_values = b_vector_get_len (data);
  if (n_values < 1)
    return FALSE;

  return range_vary_uniformly (values, n_values);
}

/**
 * b_vector_get_minmax :
 * @vec: #BVector
 * @min: (out)(nullable): return location for minimum value, or @NULL
 * @max: (out)(nullable): return location for maximum value, or @NULL
 *
 * Get the minimum and maximum values in @vec and cache them.
 **/
void
b_vector_get_minmax (BVector * vec, double *min, double *max)
{
  BData *data = B_DATA (vec);
  BDataPrivate *priv = b_data_get_instance_private (data);
  BVectorPrivate *vpriv = b_vector_get_instance_private (vec);

  if (!(priv->flags & B_DATA_MINMAX_CACHED))
    {
      const double *v = b_vector_get_values (vec);
      if (v == NULL)
        return;

      double minimum = DBL_MAX, maximum = -DBL_MAX;

      unsigned int i = b_vector_get_len (vec);

      while (i-- > 0)
        {
          if (!isfinite (v[i]))
            continue;
          if (minimum > v[i])
            minimum = v[i];
          if (maximum < v[i])
            maximum = v[i];
        }
      vpriv->minimum = minimum;
      vpriv->maximum = maximum;
      priv->flags |= B_DATA_MINMAX_CACHED;
    }

  if (min != NULL)
    *min = vpriv->minimum;
  if (max != NULL)
    *max = vpriv->maximum;
}

/**
 * b_vector_replace_cache :
 * @vec: #BVector
 * @len: new length of cache
 *
 * Frees old cache and replaces it with newly allocated memory, of length @len.
 * Used by subclasses of #BVector.
 *
 * Returns: Pointer to the new cache.
 **/
double *
b_vector_replace_cache (BVector * vec, unsigned int len)
{
  BData *data = B_DATA (vec);
  BDataPrivate *priv = b_data_get_instance_private (data);
  BVectorPrivate *vpriv = b_vector_get_instance_private (vec);

  BVectorClass const *klass = B_VECTOR_GET_CLASS (vec);
  g_return_val_if_fail (klass != NULL, NULL);

  if (vpriv->values != NULL && len == b_vector_get_len (vec))
    {
      return vpriv->values;
    }

  /* if subclass has a replace_cache function, it is handling this */
  if (klass->replace_cache)
    {
      priv->flags &=
        ~(B_DATA_CACHE_IS_VALID | B_DATA_SIZE_CACHED | B_DATA_HAS_VALUE |
          B_DATA_MINMAX_CACHED);
      return (*klass->replace_cache) (vec, len);
    }

  g_clear_pointer (&vpriv->values, g_free);
  vpriv->values = g_new0 (double, len);

  priv->flags &=
    ~(B_DATA_CACHE_IS_VALID | B_DATA_SIZE_CACHED | B_DATA_HAS_VALUE |
      B_DATA_MINMAX_CACHED);

  return vpriv->values;
}

/*************************************************************************/

/**
 * SECTION: b-matrix
 * @short_description: Base class for two-dimensional array data objects.
 *
 * Abstract base class for data classes representing two dimensional arrays.
 */

typedef struct
{
  BMatrixSize size;		/* negative if dirty, includes missing values */
  double *values;		/* NULL = uninitialized/unsupported, nan = missing */
  double minimum, maximum;
} BMatrixPrivate;

/**
 * BMatrix:
 *
 * Object representing a two-dimensional array of numbers.
 **/

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (BMatrix, b_matrix, B_TYPE_DATA);

static char
_data_matrix_get_sizes (BData * data, unsigned int *sizes)
{
  BMatrix *matrix = (BMatrix *) data;
  BMatrixSize size;

  if (sizes != NULL)
    {

      size = b_matrix_get_size (matrix);

      sizes[0] = size.rows;
      sizes[1] = size.columns;
    }
  return 2;
}

static gboolean
_matrix_has_value (BData * dat)
{
  BMatrix *mat = (BMatrix *) dat;
  double minimum, maximum;
  b_matrix_get_minmax (mat, &minimum, &maximum);
  return (isfinite (minimum) && isfinite (maximum) && minimum <= maximum);
}

static char *
_matrix_serialize (BData * dat, gpointer user)
{
  BMatrix *mat = (BMatrix *) dat;
  BMatrixPrivate *mpriv = b_matrix_get_instance_private (mat);
  GString *str;
  char col_sep = '\t';
  char row_sep = '\n';

  str = g_string_new (NULL);
  for (size_t r = 0; r < mpriv->size.rows; r++)
    {
      if (r)
        g_string_append_c (str, row_sep);
      for (size_t c = 0; c < mpriv->size.columns; c++)
        {
          double val = mpriv->values[r * mpriv->size.columns + c];
          char *s = render_val (val);
          if (c)
            g_string_append_c (str, col_sep);
            g_string_append (str, s);
            g_free (s);
        }
    }

  return g_string_free (str, FALSE);
}

static void
_matrix_finalize (GObject * dat)
{
  BMatrix *mat = (BMatrix *) dat;
  BMatrixPrivate *mpriv = b_matrix_get_instance_private (mat);
  BMatrixClass *mat_class = B_MATRIX_GET_CLASS (dat);

  if (mat_class->replace_cache == NULL)
      g_clear_pointer (&mpriv->values, g_free);
}

static void
b_matrix_class_init (BMatrixClass * mat_class)
{
  GObjectClass *gobj_class = (GObjectClass *) mat_class;
  BDataClass *data_class = B_DATA_CLASS (mat_class);

  gobj_class->finalize = _matrix_finalize;

  data_class->emit_changed = _data_array_emit_changed;
  data_class->get_sizes = _data_matrix_get_sizes;
  data_class->serialize = _matrix_serialize;
  data_class->has_value = _matrix_has_value;
}

static void
b_matrix_init (BMatrix * mat)
{
}

/**
 * b_matrix_get_size: (skip)
 * @mat: #BMatrix
 *
 * Get the size of a #BMatrix.
 *
 * Returns: the matrix size
 **/
BMatrixSize
b_matrix_get_size (BMatrix * mat)
{
  g_assert (B_IS_MATRIX (mat));
  static BMatrixSize null_size = { 0, 0 };
  if (!mat)
    return null_size;
  BData *data = B_DATA (mat);
  BDataPrivate *priv = b_data_get_instance_private (data);
  BMatrixPrivate *mpriv = b_matrix_get_instance_private (mat);
  if (!(priv->flags & B_DATA_SIZE_CACHED))
    {
      BMatrixClass const *klass = B_MATRIX_GET_CLASS (mat);

      g_return_val_if_fail (klass != NULL, null_size);

      mpriv->size = (*klass->load_size) (mat);
      priv->flags |= B_DATA_SIZE_CACHED;
    }

  return mpriv->size;
}

/**
 * b_matrix_get_rows:
 * @mat: #BMatrix
 *
 * Get the number of rows in a #BMatrix.
 *
 * Returns: the number of rows in @mat
 **/
unsigned int
b_matrix_get_rows (BMatrix * mat)
{
  g_return_val_if_fail (B_IS_MATRIX (mat), 0);
  BData *data = B_DATA (mat);
  BDataPrivate *priv = b_data_get_instance_private (data);
  BMatrixPrivate *mpriv = b_matrix_get_instance_private (mat);
  if (!(priv->flags & B_DATA_SIZE_CACHED))
    {
      BMatrixClass const *klass = B_MATRIX_GET_CLASS (mat);

      g_return_val_if_fail (klass != NULL, 0);

      mpriv->size = (*klass->load_size) (mat);
      priv->flags |= B_DATA_SIZE_CACHED;
    }

  return mpriv->size.rows;
}

/**
 * b_matrix_get_columns :
 * @mat: #BMatrix
 *
 * Get the number of columns in a #BMatrix.
 *
 * Returns: the number of columns in @mat
 **/
unsigned int
b_matrix_get_columns (BMatrix * mat)
{
  g_return_val_if_fail (B_IS_MATRIX (mat), 0);
  BData *data = B_DATA (mat);
  BDataPrivate *priv = b_data_get_instance_private (data);
  BMatrixPrivate *mpriv = b_matrix_get_instance_private (mat);
  if (!(priv->flags & B_DATA_SIZE_CACHED))
    {
      BMatrixClass const *klass = B_MATRIX_GET_CLASS (mat);

      g_return_val_if_fail (klass != NULL, 0);

      mpriv->size = (*klass->load_size) (mat);
      priv->flags |= B_DATA_SIZE_CACHED;
    }

  return mpriv->size.columns;
}

/**
 * b_matrix_get_values :
 * @mat: #BMatrix
 *
 * Get the array of values of @mat.
 *
 * Returns: an array.
 **/
const double *
b_matrix_get_values (BMatrix * mat)
{
  g_assert (B_IS_MATRIX (mat));
  BData *data = B_DATA (mat);
  BDataPrivate *priv = b_data_get_instance_private (data);
  BMatrixPrivate *mpriv = b_matrix_get_instance_private (mat);
  if (!(priv->flags & B_DATA_CACHE_IS_VALID))
    {
      BMatrixClass const *klass = B_MATRIX_GET_CLASS (mat);

      g_return_val_if_fail (klass != NULL, NULL);

      mpriv->values = (*klass->load_values) (mat);

      priv->flags |= B_DATA_CACHE_IS_VALID;
    }

  return mpriv->values;
}

/**
 * b_matrix_get_value :
 * @mat: #BMatrix
 * @i: row
 * @j: column
 *
 * Get a value in @mat.
 *
 * Returns: the value
 **/
double
b_matrix_get_value (BMatrix * mat, unsigned i, unsigned j)
{
  g_assert (B_IS_MATRIX (mat));
  BMatrixPrivate *mpriv = b_matrix_get_instance_private (mat);
  g_return_val_if_fail ((i < mpriv->size.rows)
                        && (j < mpriv->size.columns), NAN);
  BData *data = B_DATA (mat);
  BDataPrivate *priv = b_data_get_instance_private (data);
  if (!(priv->flags & B_DATA_CACHE_IS_VALID))
    {
      BMatrixClass const *klass = B_MATRIX_GET_CLASS (mat);
      g_return_val_if_fail (klass != NULL, NAN);
      return (*klass->get_value) (mat, i, j);
    }

  return mpriv->values[i * mpriv->size.columns + j];
}

/**
 * b_matrix_get_str :
 * @mat: #BMatrix
 * @i: row
 * @j: column
 * @format: a format string
 *
 * Get a string representation of a value in @mat.
 *
 * Returns: the string
 **/
char *
b_matrix_get_str (BMatrix * mat, unsigned i, unsigned j, const gchar * format)
{
  double val = b_matrix_get_value (mat, i, j);
  return format_val (val, format);
}

/**
 * b_matrix_get_minmax :
 * @mat: #BMatrix
 * @min: (out)(nullable): return location for minimum value, or @NULL
 * @max: (out)(nullable): return location for maximum value, or @NULL
 *
 * Get the minimum and maximum values in @mat.
 **/
void
b_matrix_get_minmax (BMatrix * mat, double *min, double *max)
{
  BData *data = B_DATA (mat);
  BDataPrivate *priv = b_data_get_instance_private (data);
  BMatrixPrivate *mpriv = b_matrix_get_instance_private (mat);
  if (!(priv->flags & B_DATA_MINMAX_CACHED))
    {
      const double *v = b_matrix_get_values (mat);

      double minimum = DBL_MAX, maximum = -DBL_MAX;

      BMatrixSize s = b_matrix_get_size (mat);
      unsigned int i = s.rows * s.columns;

      while (i-- > 0)
        {
          if (!isfinite (v[i]))
            continue;
          if (minimum > v[i])
            minimum = v[i];
          if (maximum < v[i])
            maximum = v[i];
          }
      mpriv->minimum = minimum;
      mpriv->maximum = maximum;
      priv->flags |= B_DATA_MINMAX_CACHED;
    }

  if (min != NULL)
    *min = mpriv->minimum;
  if (max != NULL)
    *max = mpriv->maximum;
}

/**
 * b_matrix_replace_cache :
 * @mat: #BMatrix
 * @len: new length of cache
 *
 * Frees old cache and replaces it with newly allocated memory, of length @len.
 * Used by subclasses of #BMatrix.
 *
 * Returns: Pointer to the new cache.
 **/
double *
b_matrix_replace_cache (BMatrix * mat, unsigned len)
{
  BData *data = B_DATA (mat);
  BDataPrivate *priv = b_data_get_instance_private (data);
  BMatrixPrivate *mpriv = b_matrix_get_instance_private (mat);

  BMatrixClass const *klass = B_MATRIX_GET_CLASS (mat);
  g_return_val_if_fail (klass != NULL, NULL);

  BMatrixSize s = b_matrix_get_size (mat);

  if (mpriv->values != NULL && s.rows * s.columns == len)
    {
      return mpriv->values;
    }

  /* if subclass has a replace_cache function, it is handling this */
  if (klass->replace_cache)
    {
      priv->flags &=
        ~(B_DATA_CACHE_IS_VALID | B_DATA_SIZE_CACHED | B_DATA_HAS_VALUE |
          B_DATA_MINMAX_CACHED);
      return (*klass->replace_cache) (mat, len);
    }

  g_clear_pointer (&mpriv->values, g_free);
  mpriv->values = g_new0 (double, len);

  priv->flags &=
    ~(B_DATA_CACHE_IS_VALID | B_DATA_SIZE_CACHED | B_DATA_HAS_VALUE |
      B_DATA_MINMAX_CACHED);

  return mpriv->values;
}

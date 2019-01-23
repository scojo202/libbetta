/*
 * b-data-class.h :
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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

#ifndef B_DATA_CLASS_H
#define B_DATA_CLASS_H

#include <glib-object.h>

G_BEGIN_DECLS

G_DECLARE_DERIVABLE_TYPE(BData, b_data, B, DATA, GInitiallyUnowned)

#define B_TYPE_DATA	(b_data_get_type ())

/**
 * BMatrixSize:
 * @rows: rows number, includes missing values.
 * @columns: columns number, includes missing values.
 *
 * Holds the size of a matrix.
 **/

typedef struct {
  unsigned int rows;
  unsigned int columns;
} BMatrixSize;

/**
 * BThreeDArraySize:
 * @layers: number of layers, includes missing values.
 * @rows: rows number, includes missing values.
 * @columns: columns number, includes missing values.
 *
 * Holds the size of a matrix.
 **/

typedef struct {
  unsigned int layers;
  unsigned int rows;
  unsigned int columns;
} BThreeDArraySize;

/**
 * BDataClass:
 * @base: base class.
 * @dup: duplicates the #BData.
 * @serialize: serializes to text.
 * @get_sizes: gets the size of each dimension and returns the number of dimensions.
 * @has_value: returns whether data has a finite value.
 * @emit_changed: changed signal default handler
 *
 * Class for BData.
 **/

struct _BDataClass {
  GObjectClass base;

  BData *(*dup) (BData * src);

  char *(*serialize) (BData * dat, gpointer user);

  char (*get_sizes) (BData * data, unsigned int *sizes);
  gboolean (*has_value) (BData *data);

  /* signals */
  void (*emit_changed) (BData * data);
};

G_DECLARE_DERIVABLE_TYPE(BScalar, b_scalar, B, SCALAR, BData)

#define B_TYPE_SCALAR	(b_scalar_get_type ())

/**
 * BScalarClass:
 * @base:  base class.
 * @get_value: gets the value.
 *
 * Class for BScalar.
 **/

struct _BScalarClass {
	BDataClass base;
	double (*get_value) (BScalar * scalar);
};

G_DECLARE_FINAL_TYPE(BValScalar, b_val_scalar, B, VAL_SCALAR, BScalar)

#define B_TYPE_VAL_SCALAR	(b_val_scalar_get_type ())

BData *b_val_scalar_new(double val);
double *b_val_scalar_get_val(BValScalar * s);
void b_val_scalar_set_val(BValScalar *s, double val);

G_DECLARE_DERIVABLE_TYPE(BVector, b_vector, B, VECTOR, BData)

#define B_TYPE_VECTOR	(b_vector_get_type ())

/**
 * BVectorClass:
 * @base: base class.
 * @load_len: loads the vector length and returns it.
 * @load_values: loads the values and returns them.
 * @get_value: gets a value.
 * @replace_cache: replaces array cache
 *
 * Class for BVector.
 **/

struct _BVectorClass {
	BDataClass base;

	unsigned int (*load_len) (BVector * vec);
	double *(*load_values) (BVector * vec);
	double (*get_value) (BVector * vec, guint i);
	double *(*replace_cache) (BVector *vec, guint len);
};

G_DECLARE_DERIVABLE_TYPE(BMatrix, b_matrix, B, MATRIX, BData)

#define B_TYPE_MATRIX (b_matrix_get_type())

/**
 * BMatrixClass:
 * @base: base class.
 * @load_size: loads the matrix length.
 * @load_values: loads the values in the cache.
 * @get_value: gets a value.
 * @replace_cache: replaces array cache
 *
 * Class for BMatrix.
 **/

struct _BMatrixClass {
  BDataClass base;

  BMatrixSize(*load_size) (BMatrix * vec);
  double *(*load_values) (BMatrix * vec);
  double (*get_value) (BMatrix * mat, guint i, guint j);
  double *(*replace_cache) (BMatrix *mat, guint len);
};

G_DECLARE_DERIVABLE_TYPE(BThreeDArray, b_three_d_array, B, THREE_D_ARRAY, BData)

#define B_TYPE_THREE_D_ARRAY (b_three_d_array_get_type())

/**
 * BThreeDArrayClass:
 * @base: base class.
 * @load_size: loads the matrix length.
 * @load_values: loads the values in the cache.
 * @get_value: gets a value.
 *
 * Class for BThreeDArray.
 **/

struct _BThreeDArrayClass {
  BDataClass base;

  BThreeDArraySize(*load_size) (BThreeDArray * vec);
  double *(*load_values) (BThreeDArray * vec);
  double (*get_value) (BThreeDArray * mat, guint i, guint j,
			     guint k);
};

BData *b_data_dup(BData * src);
BData *b_data_dup_to_simple(BData * src);

char *b_data_serialize(BData * dat, gpointer user);

void b_data_emit_changed(BData * data);
gint64 b_data_get_timestamp(BData *data);

gboolean b_data_has_value(BData * data);

char b_data_get_n_dimensions(BData * data);
unsigned int b_data_get_n_values(BData * data);

/*************************************************************************/

double b_scalar_get_value(BScalar * scalar);
char *b_scalar_get_str(BScalar * scalar, const gchar * format);

/*************************************************************************/

unsigned int b_vector_get_len(BVector * vec);
const double *b_vector_get_values(BVector * vec);
double b_vector_get_value(BVector * vec, guint i);
char *b_vector_get_str(BVector * vec, unsigned int i, const gchar * format);
gboolean b_vector_is_varying_uniformly(BVector * data);
void b_vector_get_minmax(BVector * vec, double *min, double *max);

/* to be used only by subclasses */
double* b_vector_replace_cache(BVector *vec, guint len);

/*************************************************************************/

BMatrixSize b_matrix_get_size(BMatrix * mat);
unsigned int b_matrix_get_rows(BMatrix * mat);
unsigned int b_matrix_get_columns(BMatrix * mat);
const double *b_matrix_get_values(BMatrix * mat);
double b_matrix_get_value(BMatrix * mat, guint i, guint j);
char *b_matrix_get_str(BMatrix * mat, guint i, guint j,
		       const gchar * format);
void b_matrix_get_minmax(BMatrix * mat, double *min, double *max);

/* to be used only by subclasses */
double* b_matrix_replace_cache(BMatrix *mat, guint len);

/*************************************************************************/

BThreeDArraySize b_three_d_array_get_size(BThreeDArray * mat);
unsigned int b_three_d_array_get_rows(BThreeDArray * mat);
unsigned int b_three_d_array_get_columns(BThreeDArray * mat);
unsigned int b_three_d_array_get_layers(BThreeDArray * mat);
const double *b_three_d_array_get_values(BThreeDArray * mat);
double b_three_d_array_get_value(BThreeDArray * mat, guint i, guint j,
				 guint k);
char *b_three_d_array_get_str(BThreeDArray * mat, guint i, guint j,
			      guint k, const gchar * format);
void b_three_d_array_get_minmax(BThreeDArray * mat, double *min, double *max);

G_END_DECLS

#endif				/* B_DATA_H */

/*
 * y-data-simple.h :
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

#ifndef Y_DATA_SIMPLE_H
#define Y_DATA_SIMPLE_H

#include <glib-object.h>
#include "data/y-data-class.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(YValVector,y_val_vector,Y,VAL_VECTOR,YVector)

#define Y_TYPE_VAL_VECTOR  (y_val_vector_get_type ())

YData	*y_val_vector_new      (double *val, unsigned n, GDestroyNotify   notify);
YData	*y_val_vector_new_alloc (unsigned n);
YData	*y_val_vector_new_copy (const double *val, unsigned n);

double *y_val_vector_get_array (YValVector *s);
void y_val_vector_replace_array(YValVector *s, double *array, unsigned n, GDestroyNotify notify);

G_DECLARE_FINAL_TYPE(YValMatrix,y_val_matrix,Y,VAL_MATRIX,YMatrix)

#define Y_TYPE_VAL_MATRIX  (y_val_matrix_get_type ())

YData	*y_val_matrix_new      (double *val, unsigned rows, unsigned columns, GDestroyNotify   notify);
YData *y_val_matrix_new_copy (const double   *val,
                                     unsigned  rows, unsigned columns);
YData *y_val_matrix_new_alloc (unsigned rows, unsigned columns);

double *y_val_matrix_get_array (YValMatrix *s);
void y_val_matrix_replace_array(YValMatrix *s, double *array, unsigned rows, unsigned columns, GDestroyNotify notify);

G_DECLARE_FINAL_TYPE(YValThreeDArray,y_val_three_d_array,Y,VAL_THREE_D_ARRAY,YThreeDArray)

#define Y_TYPE_VAL_THREE_D_ARRAY  (y_val_three_d_array_get_type ())

YData	*y_val_three_d_array_new      (double *val, unsigned rows, unsigned columns, unsigned layers, GDestroyNotify   notify);
YData *y_val_three_d_array_new_copy (double   *val,
                                     unsigned  rows, unsigned columns, unsigned layers);
YData *y_val_three_d_array_new_alloc (unsigned rows, unsigned columns, unsigned layers);

double *y_val_three_d_array_get_array (YValThreeDArray *s);

G_END_DECLS

#endif /* Y_DATA_SIMPLE_H */

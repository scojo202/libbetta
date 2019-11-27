/*
 * b-data-simple.h :
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

#pragma once

#include "data/b-data-class.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(BValVector,b_val_vector,B,VAL_VECTOR,BVector)

#define B_TYPE_VAL_VECTOR  (b_val_vector_get_type ())

BData	*b_val_vector_new      (double *val, guint n, GDestroyNotify   notify);
BData	*b_val_vector_new_alloc (guint n);
BData	*b_val_vector_new_copy (const double *val, guint n);

double *b_val_vector_get_array (BValVector *s);
void b_val_vector_replace_array(BValVector *s, double *array, guint n, GDestroyNotify notify);

G_DECLARE_FINAL_TYPE(BValMatrix,b_val_matrix,B,VAL_MATRIX,BMatrix)

#define B_TYPE_VAL_MATRIX  (b_val_matrix_get_type ())

BData	*b_val_matrix_new      (double *val, guint rows, guint columns, GDestroyNotify   notify);
BData *b_val_matrix_new_copy (const double   *val,
                                     guint  rows, guint columns);
BData *b_val_matrix_new_alloc (guint rows, guint columns);

double *b_val_matrix_get_array (BValMatrix *s);
void b_val_matrix_replace_array(BValMatrix *s, double *array, guint rows, guint columns, GDestroyNotify notify);

G_END_DECLS

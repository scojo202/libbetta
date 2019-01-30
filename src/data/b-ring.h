/*
 * b-vector-ring.h :
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

#pragma once

#include <glib-object.h>
#include <data/b-data-class.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(BRingVector,b_ring_vector,B,RING_VECTOR,BVector)

#define B_TYPE_RING_VECTOR  (b_ring_vector_get_type ())

BData *b_ring_vector_new (unsigned nmax, unsigned n, gboolean track_timestamps);
void b_ring_vector_set_length(BRingVector *d, unsigned newlength);
void b_ring_vector_append(BRingVector *d, double val);
void b_ring_vector_append_array(BRingVector *d, double *arr, int len);

void b_ring_vector_set_source(BRingVector *d, BScalar *source);

BRingVector *b_ring_vector_get_timestamps(BRingVector *d);

G_DECLARE_FINAL_TYPE(BRingMatrix,b_ring_matrix,B,RING_MATRIX,BMatrix)

#define B_TYPE_RING_MATRIX  (b_ring_matrix_get_type ())

BData *b_ring_matrix_new (unsigned c, unsigned rmax, unsigned r, gboolean track_timestamps);
void b_ring_matrix_set_rows(BRingMatrix *d, unsigned r);
void b_ring_matrix_set_max_rows(BRingMatrix *d, unsigned rmax);
void b_ring_matrix_append(BRingMatrix *d, const double *values, unsigned len);
void b_ring_matrix_set_source(BRingMatrix *d, BVector *source);

BRingVector *b_ring_matrix_get_timestamps(BRingMatrix *d);

G_END_DECLS

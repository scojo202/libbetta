/*
 * b-linear-range.h :
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

#ifndef _LINEAR_RANGE_H
#define _LINEAR_RANGE_H

#include <data/b-data-class.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(BLinearRangeVector,b_linear_range_vector,B,LINEAR_RANGE_VECTOR,BVector)

#define B_TYPE_LINEAR_RANGE_VECTOR  (b_linear_range_vector_get_type ())

BData	*b_linear_range_vector_new  (double v0, double dv, unsigned n);

void b_linear_range_vector_set_length(BLinearRangeVector *d, unsigned int n);
void b_linear_range_vector_set_pars(BLinearRangeVector *d, double v0, double dv);
void b_linear_range_vector_set_v0(BLinearRangeVector *d, double v0);
void b_linear_range_vector_set_dv(BLinearRangeVector *d, double dv);

double b_linear_range_vector_get_v0(BLinearRangeVector *d);
double b_linear_range_vector_get_dv(BLinearRangeVector *d);

G_DECLARE_FINAL_TYPE(BFourierLinearRangeVector,b_fourier_linear_range_vector,B,FOURIER_LINEAR_RANGE_VECTOR,BVector)

#define B_TYPE_FOURIER_LINEAR_RANGE_VECTOR  (b_fourier_linear_range_vector_get_type ())

void b_fourier_linear_range_vector_set_inverse(BFourierLinearRangeVector *v, gboolean val);
BData *b_fourier_linear_range_vector_new( BLinearRangeVector *v);

G_END_DECLS

#endif

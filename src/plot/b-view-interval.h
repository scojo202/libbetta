/*
 * b-view-interval.h
 *
 * Copyright (C) 2000 EMC Capital Management, Inc.
 * Copyright (C) 2001 The Free Software Foundation
 * Copyright (C) 2016 Scott O. Johnson (scojo202@gmail.com)
 *
 * Developed by Jon Trowbridge <trow@gnu.org> and
 * Havoc Pennington <hp@pobox.com>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

enum {
  VIEW_NORMAL=0,
  VIEW_LOG=1,
  VIEW_LAST
};

G_DECLARE_FINAL_TYPE(BViewInterval,b_view_interval,B,VIEW_INTERVAL,GObject)

#define B_TYPE_VIEW_INTERVAL (b_view_interval_get_type())

BViewInterval *b_view_interval_new (void);

void b_view_interval_set (BViewInterval *v, double a, double b);
void b_view_interval_grow_to (BViewInterval *v, double a, double b);
void b_view_interval_range (BViewInterval *v, double *a, double *b);
double b_view_interval_get_width (BViewInterval *v);

void b_view_interval_set_bounds (BViewInterval *v, double a, double b);
void b_view_interval_clear_bounds (BViewInterval *v);
void b_view_interval_set_min_width (BViewInterval *v, double mw);

gboolean b_view_interval_valid (BViewInterval *v, double x);
double b_view_interval_conv (BViewInterval *v, double x);
double b_view_interval_unconv (BViewInterval *v, double x);

int b_view_interval_get_vi_type(BViewInterval *v);

#define b_view_interval_is_logarithmic(v) (b_view_interval_get_vi_type(v) == VIEW_LOG)

void b_view_interval_conv_bulk (BViewInterval * v,
				    const double *in_data, double *out_data, gsize N);
void b_view_interval_unconv_bulk (BViewInterval * v,
				      const double *in_data, double *out_data, gsize N);


void b_view_interval_rescale_around_point (BViewInterval *v, double x, double s);
void b_view_interval_recenter_around_point (BViewInterval *v, double x);
void b_view_interval_translate (BViewInterval *v, double dx);

void b_view_interval_conv_translate (BViewInterval *v, double dx);

void b_view_interval_request_preferred_range (BViewInterval *v);
void b_view_interval_set_ignore_preferred_range (BViewInterval *v, gboolean ignore);
gboolean b_view_interval_get_ignore_preferred_range (BViewInterval *v);

void b_view_interval_scale_linearly (BViewInterval *v);
void b_view_interval_scale_logarithmically (BViewInterval *v, double base);

G_END_DECLS

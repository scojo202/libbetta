/*
 * y-view-interval.h
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

#ifndef _INC_VIEW_INTERVAL_H
#define _INC_VIEW_INTERVAL_H

#include <glib-object.h>

G_BEGIN_DECLS

enum {
  VIEW_NORMAL=0,
  VIEW_LOG=1,
  VIEW_LAST
};

G_DECLARE_FINAL_TYPE(YViewInterval,y_view_interval,Y,VIEW_INTERVAL,GObject)

#define Y_TYPE_VIEW_INTERVAL (y_view_interval_get_type())

YViewInterval *y_view_interval_new (void);

void y_view_interval_set (YViewInterval *v, double a, double b);
void y_view_interval_grow_to (YViewInterval *v, double a, double b);
void y_view_interval_range (YViewInterval *v, double *a, double *b);

void y_view_interval_set_bounds (YViewInterval *v, double a, double b);
void y_view_interval_clear_bounds (YViewInterval *v);
void y_view_interval_set_min_width (YViewInterval *v, double mw);

gboolean y_view_interval_valid_fn (YViewInterval *v, double x);
double y_view_interval_conv (YViewInterval *v, double x);
double y_view_interval_unconv (YViewInterval *v, double x);

int y_view_interval_get_vi_type(YViewInterval *v);

#define y_view_interval_valid(v, x) (y_view_interval_get_vi_type(v) == VIEW_NORMAL ? TRUE : y_view_interval_valid_fn((v),(x)))

#define y_view_interval_is_logarithmic(v) (y_view_interval_get_vi_type(v) == VIEW_LOG)

void y_view_interval_conv_bulk (YViewInterval * v,
				    const double *in_data, double *out_data, gsize N);
void y_view_interval_unconv_bulk (YViewInterval * v,
				      const double *in_data, double *out_data, gsize N);


void y_view_interval_rescale_around_point (YViewInterval *v, double x, double s);
void y_view_interval_recenter_around_point (YViewInterval *v, double x);
void y_view_interval_translate (YViewInterval *v, double dx);

void y_view_interval_conv_translate (YViewInterval *v, double dx);

void y_view_interval_request_preferred_range (YViewInterval *v);
void y_view_interval_set_ignore_preferred_range (YViewInterval *v, gboolean ignore);
gboolean y_view_interval_get_ignore_preferred_range (YViewInterval *v);

void y_view_interval_scale_linearly (YViewInterval *v);
void y_view_interval_scale_logarithmically (YViewInterval *v, double base);

G_END_DECLS

#endif

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

typedef struct _YViewInterval YViewInterval;
typedef struct _YViewIntervalClass YViewIntervalClass;

struct _YViewInterval {
  GObject parent;

  gint type;
  double type_arg;

  double t0, t1;

  double min, max;
  double min_width;
  guint include_min : 1;
  guint include_max : 1;

  guint block_changed_signals : 1;
  
  guint ignore_preferred : 1;
};

struct _YViewIntervalClass {
  GObjectClass parent_class;

  /* signals */
  void (*changed) (YViewInterval *v);
  void (*preferred_range_request) (YViewInterval *v);
};

#define Y_TYPE_VIEW_INTERVAL (y_view_interval_get_type())
#define Y_VIEW_INTERVAL(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),Y_TYPE_VIEW_INTERVAL,YViewInterval))
#define Y_VIEW_INTERVAL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),Y_TYPE_VIEW_INTERVAL,YViewIntervalClass))
#define Y_IS_VIEW_INTERVAL(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), Y_TYPE_VIEW_INTERVAL))
#define Y_IS_VIEW_INTERVAL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), Y_TYPE_VIEW_INTERVAL))

GType y_view_interval_get_type (void);

YViewInterval *y_view_interval_new (void);

void y_view_interval_set (YViewInterval *v, double a, double b);
void y_view_interval_grow_to (YViewInterval *v, double a, double b);
void y_view_interval_range (YViewInterval *v, double *a, double *b);

void y_view_interval_set_bounds (YViewInterval *v, double a, double b);
void y_view_interval_clear_bounds (YViewInterval *v);
void y_view_interval_set_min_width (YViewInterval *v, double mw);

gboolean y_view_interval_valid_fn (YViewInterval *v, double x);
double y_view_interval_conv_fn (YViewInterval *v, double x);
double y_view_interval_unconv_fn (YViewInterval *v, double x);

#define y_view_interval_valid(v, x) ((v)->type == VIEW_NORMAL ? TRUE : y_view_interval_valid_fn((v),(x)))

#define y_view_interval_conv(v, x) ((v)->type == VIEW_NORMAL ? ((x)-(v)->t0)/((v)->t1-(v)->t0) : y_view_interval_conv_fn((v),(x)))

#define y_view_interval_unconv(v, x) ((v)->type == VIEW_NORMAL ? (v)->t0 + (x)*((v)->t1-(v)->t0) : y_view_interval_unconv_fn((v),(x)))

#define y_view_interval_is_logarithmic(v) (v->type == VIEW_LOG)

#define y_view_interval_logarithm_base(v) (v->type_arg)

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

void y_view_interval_scale_linearly (YViewInterval *v);
void y_view_interval_scale_logarithmically (YViewInterval *v, double base);

G_END_DECLS

#endif

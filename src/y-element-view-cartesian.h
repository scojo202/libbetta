/*
 * y-element-view.h
 *
 * Copyright (C) 2000 EMC Capital Management, Inc.
 * Copyright (C) 2001-2002 The Free Software Foundation
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

#ifndef _INC_ELEMENT_VIEW_CARTESIAN_H
#define _INC_ELEMENT_VIEW_CARTESIAN_H

#include "y-element-view.h"
#include "y-view-interval.h"
#include "y-axis-markers.h"

G_BEGIN_DECLS

typedef enum {
  META_AXIS    = 0,
  X_AXIS       = 1,
  Y_AXIS       = 2,
  Z_AXIS       = 3,
  T_AXIS       = 4,
  LAST_AXIS    = 5,
  INVALID_AXIS = 6
} axis_t;

G_DECLARE_DERIVABLE_TYPE(YElementViewCartesian, y_element_view_cartesian, Y, ELEMENT_VIEW_CARTESIAN, YElementView)

#define Y_TYPE_ELEMENT_VIEW_CARTESIAN (y_element_view_cartesian_get_type())

struct _YElementViewCartesianClass {
  YElementViewClass parent_class;

  void (*update_axis_markers) (YElementViewCartesian *cart,
			       axis_t               axis,
			       YAxisMarkers          *markers,
			       double                     range_min,
			       double                     range_max);

  gboolean (*preferred_range) (YElementViewCartesian *cart,
			       axis_t               axis,
			       double                    *range_min,
			       double                    *range_max);

};

/* View Intervals */

void               y_element_view_cartesian_add_view_interval (YElementViewCartesian *cart,
								   axis_t axis);

YViewInterval *y_element_view_cartesian_get_view_interval (YElementViewCartesian *cart,
								   axis_t axis);

void               y_element_view_cartesian_connect_view_intervals (YElementViewCartesian *cart1,
									axis_t axis1,
									YElementViewCartesian *cart2,
									axis_t axis2);

void y_element_view_cartesian_set_preferred_view (YElementViewCartesian *cart,
						      axis_t axis);

void y_element_view_cartesian_set_preferred_view_all (YElementViewCartesian *cart);

void y_element_view_cartesian_force_preferred_view (YElementViewCartesian *cart,
							axis_t axis,
							gboolean);


/* Axis Markers */

void              y_element_view_cartesian_add_axis_markers     (YElementViewCartesian *cart,
								     axis_t               axis);

gint              y_element_view_cartesian_get_axis_marker_type (YElementViewCartesian *cart,
								     axis_t               axis);

void              y_element_view_cartesian_set_axis_marker_type (YElementViewCartesian *cart,
								     axis_t               axis,
								     gint                       code);

YAxisMarkers *y_element_view_cartesian_get_axis_markers     (YElementViewCartesian *cart,
								     axis_t               axis);

void              y_element_view_cartesian_connect_axis_markers (YElementViewCartesian *cart1,
								     axis_t               axis1,
								     YElementViewCartesian *view2,
								     axis_t               axis2);

GtkWidget * create_autoscale_menu_check_item (const gchar *label, YElementViewCartesian *view, axis_t ax);

G_END_DECLS

#endif /* _INC_ELEMENT_VIEW_CARTESIAN_H */

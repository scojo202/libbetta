/*
 * b-element-view.h
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

#include "plot/b-element-view.h"
#include "plot/b-view-interval.h"
#include "plot/b-axis-markers.h"

G_BEGIN_DECLS

/* should be Y_AXIS_TYPE_META, Y_AXIS_TYPE_X, etc. */
typedef enum {
  META_AXIS    = 0,
  X_AXIS       = 1,
  Y_AXIS       = 2,
  Z_AXIS       = 3,
  T_AXIS       = 4,
  LAST_AXIS    = 5,
  INVALID_AXIS = 6
} BAxisType;

G_DECLARE_DERIVABLE_TYPE(BElementViewCartesian, b_element_view_cartesian, B, ELEMENT_VIEW_CARTESIAN, BElementView)

#define B_TYPE_ELEMENT_VIEW_CARTESIAN (b_element_view_cartesian_get_type())

/**
 * BElementViewCartesianClass:
 * @parent_class: base class
 * @update_axis_markers: method that updates axis markers for a particular axis
 * @preferred_range: method that returns the view's preferred range, which for example could be a range big enough to show all data
 *
 * Abstract base class for cartesian views.
 **/
struct _BElementViewCartesianClass {
  BElementViewClass parent_class;

  void (*update_axis_markers) (BElementViewCartesian *cart,
			       BAxisType               axis,
			       BAxisMarkers          *markers,
			       double                     range_min,
			       double                     range_max);

  gboolean (*preferred_range) (BElementViewCartesian *cart,
			       BAxisType               axis,
			       double                    *range_min,
			       double                    *range_max);

};

/* View Intervals */

void               b_element_view_cartesian_add_view_interval (BElementViewCartesian *cart,
								   BAxisType ax);

BViewInterval *b_element_view_cartesian_get_view_interval (BElementViewCartesian *cart,
								   BAxisType ax);

void               b_element_view_cartesian_connect_view_intervals (BElementViewCartesian *cart1,
									BAxisType axis1,
									BElementViewCartesian *cart2,
									BAxisType axis2);

void b_element_view_cartesian_set_preferred_view (BElementViewCartesian *cart,
						      BAxisType axis);

void b_element_view_cartesian_set_preferred_view_all (BElementViewCartesian *cart);

void b_element_view_cartesian_force_preferred_view (BElementViewCartesian *cart,
							BAxisType axis,
							gboolean force);


/* Axis Markers */

void              b_element_view_cartesian_add_axis_markers     (BElementViewCartesian *cart,
								     BAxisType               axis);

gint              b_element_view_cartesian_get_axis_marker_type (BElementViewCartesian *cart,
								     BAxisType               axis);

void              b_element_view_cartesian_set_axis_marker_type (BElementViewCartesian *cart,
								     BAxisType               axis,
								     gint                       code);

BAxisMarkers *b_element_view_cartesian_get_axis_markers     (BElementViewCartesian *cart,
								     BAxisType               ax);

void              b_element_view_cartesian_connect_axis_markers (BElementViewCartesian *cart1,
								     BAxisType               axis1,
								     BElementViewCartesian *cart2,
								     BAxisType               axis2);

void b_rescale_around_val(BViewInterval *vi, double x, GdkEventButton *event);

void _format_double_scinot (gchar *buffer, double x);
GtkWidget * _y_create_autoscale_menu_check_item (BElementViewCartesian * view, BAxisType ax, const gchar * label);

G_END_DECLS

#endif /* _INC_ELEMENT_VIEW_CARTESIAN_H */

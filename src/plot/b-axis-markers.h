/*
 * b-axis-markers.h
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

#ifndef _INC_B_AXIS_MARKERS_H
#define _INC_B_AXIS_MARKERS_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

enum {
  B_TICK_NONE = 0,
  B_TICK_MAJOR,
  B_TICK_MINOR,
  B_TICK_MICRO,
  B_TICK_MAJOR_RULE,
  B_TICK_MINOR_RULE,
  B_TICK_MICRO_RULE
};

enum {
  B_AXIS_NONE=0,
  B_AXIS_SCALAR=1,
  B_AXIS_SCALAR_LOG2=2,
  B_AXIS_SCALAR_LOG10=3,
  B_AXIS_PERCENTAGE=4,
  B_AXIS_DATE=5,
  B_AXIS_LAST=6
};

/**
 * BTick:
 * @position: the position of the tick along the axis
 * @type: the type of the tick (major, minor, etc.)
 * @label: the label to show next to the tick
 * @critical_label: whether the label must be shown at all costs
 *
 * Abstract base class for cartesian views.
 **/

typedef struct _BTick BTick;

struct _BTick {
  double position;
  gint type;
  gchar *label;
  gboolean critical_label;
};

/**
 * b_tick_position:
 * @x: a #BTick
 *
 * Get the position of a tick
 *
 * Returns: the position in plot coordinates
 **/
#define b_tick_position(x) ((x)->position)

/**
 * b_tick_type:
 * @x: a #BTick
 *
 * Get the type of a tick
 *
 * Returns: the type (major, minor, etc.)
 **/
#define b_tick_type(x) ((x)->type)

/**
 * b_tick_has_label_only:
 * @x: a #BTick
 *
 * Get whether the tick shows a label but no line
 *
 * Returns: %TRUE or %FALSE
 **/
#define b_tick_has_label_only(x) ((x)->type == B_TICK_NONE)

/**
 * b_tick_is_major:
 * @x: a #BTick
 *
 * Get whether the tick is a major tick
 *
 * Returns: %TRUE or %FALSE
 **/
#define b_tick_is_major(x) \
  ((x)->type == B_TICK_MAJOR || (x)->type == B_TICK_MAJOR_RULE)

/**
 * b_tick_is_minor:
 * @x: a #BTick
 *
 * Get whether the tick is a minor tick
 *
 * Returns: %TRUE or %FALSE
 **/
#define b_tick_is_minor(x) \
  ((x)->type == B_TICK_MINOR || (x)->type == B_TICK_MINOR_RULE)

/**
 * b_tick_is_micro:
 * @x: a #BTick
 *
 * Get whether the tick is a micro tick
 *
 * Returns: %TRUE or %FALSE
 **/
#define b_tick_is_micro(x) \
  ((x)->type == B_TICK_MICRO || (x)->type == B_TICK_MICRO_RULE)

#define b_tick_is_rule(x) \
  ((x)->type == B_TICK_MAJOR_RULE || (x)->type == B_TICK_MINOR_RULE ||\
   (x)->type == B_TICK_MICRO_RULE)

/**
 * b_tick_is_labelled:
 * @x: a #BTick
 *
 * Get whether the tick has a label
 *
 * Returns: %TRUE or %FALSE
 **/
#define b_tick_is_labelled(x) ((x)->label != NULL)

/**
 * b_tick_label:
 * @x: a #BTick
 *
 * Get the label
 *
 * Returns: a string
 **/
#define b_tick_label(x) ((x)->label)

/**********************/

G_DECLARE_FINAL_TYPE(BAxisMarkers,b_axis_markers,B,AXIS_MARKERS,GObject)

#define B_TYPE_AXIS_MARKERS (b_axis_markers_get_type())

BAxisMarkers *b_axis_markers_new (void);

void b_axis_markers_freeze (BAxisMarkers *am);
void b_axis_markers_thaw (BAxisMarkers *am);

gint b_axis_markers_size (BAxisMarkers *am);

const BTick *b_axis_markers_get (BAxisMarkers *am, gint i);

void b_axis_markers_clear (BAxisMarkers *am);

void b_axis_markers_add (BAxisMarkers *am, double pos, gint type, const gchar *label);
void b_axis_markers_add_critical (BAxisMarkers *am, double pos, gint type, const gchar *label);
void b_axis_markers_sort (BAxisMarkers *am);

void b_axis_markers_populate_scalar (BAxisMarkers *am,
					 double pos_min, double pos_max,
					 gint goal, gint radix,
					 gboolean percentage);

void b_axis_markers_populate_scalar_log (BAxisMarkers *am,
					     double min, double max,
					     gint goal, double base);

void b_axis_markers_populate_dates (BAxisMarkers *am,
					GDate *min, GDate *max);

void b_axis_markers_populate_generic (BAxisMarkers *am,
					  gint type,
					  double min, double max);

G_END_DECLS

#endif /* _INC_B_AXIS_MARKERS_H */

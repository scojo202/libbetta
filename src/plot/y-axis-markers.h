/*
 * y-axis-markers.h
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

#ifndef _INC_Y_AXIS_MARKERS_H
#define _INC_Y_AXIS_MARKERS_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

enum {
  Y_TICK_NONE,
  Y_TICK_MAJOR,
  Y_TICK_MINOR,
  Y_TICK_MICRO,
  Y_TICK_MAJOR_RULE,
  Y_TICK_MINOR_RULE,
  Y_TICK_MICRO_RULE
};

enum {
  Y_AXIS_NONE=0,
  Y_AXIS_SCALAR=1,
  Y_AXIS_SCALAR_LOG2=2,
  Y_AXIS_SCALAR_LOG10=3,
  Y_AXIS_PERCENTAGE=4,
  Y_AXIS_DATE=5,
  Y_AXIS_LAST=6
};

/**
 * YTick:
 * @position: the position of the tick along the axis
 * @type: the type of the tick (major, minor, etc.)
 * @label: the label to show next to the tick
 * @critical_label: whether the label must be shown at all costs
 *
 * Abstract base class for cartesian views.
 **/

typedef struct _YTick YTick;

struct _YTick {
  double position;
  gint type;
  gchar *label;
  gboolean critical_label;
};

/**
 * y_tick_position:
 * @x: a #YTick
 *
 * Get the position of a tick
 *
 * Returns: the position in plot coordinates
 **/
#define y_tick_position(x) ((x)->position)

/**
 * y_tick_type:
 * @x: a #YTick
 *
 * Get the type of a tick
 *
 * Returns: the type (major, minor, etc.)
 **/
#define y_tick_type(x) ((x)->type)

/**
 * y_tick_has_label_only:
 * @x: a #YTick
 *
 * Get whether the tick shows a label but no line
 *
 * Returns: %TRUE or %FALSE
 **/
#define y_tick_has_label_only(x) ((x)->type == Y_TICK_NONE)

/**
 * y_tick_is_major:
 * @x: a #YTick
 *
 * Get whether the tick is a major tick
 *
 * Returns: %TRUE or %FALSE
 **/
#define y_tick_is_major(x) \
  ((x)->type == Y_TICK_MAJOR || (x)->type == Y_TICK_MAJOR_RULE)

/**
 * y_tick_is_minor:
 * @x: a #YTick
 *
 * Get whether the tick is a minor tick
 *
 * Returns: %TRUE or %FALSE
 **/
#define y_tick_is_minor(x) \
  ((x)->type == Y_TICK_MINOR || (x)->type == Y_TICK_MINOR_RULE)

/**
 * y_tick_is_micro:
 * @x: a #YTick
 *
 * Get whether the tick is a micro tick
 *
 * Returns: %TRUE or %FALSE
 **/
#define y_tick_is_micro(x) \
  ((x)->type == Y_TICK_MICRO || (x)->type == Y_TICK_MICRO_RULE)

#define y_tick_is_rule(x) \
  ((x)->type == Y_TICK_MAJOR_RULE || (x)->type == Y_TICK_MINOR_RULE ||\
   (x)->type == Y_TICK_MICRO_RULE)

/**
 * y_tick_is_labelled:
 * @x: a #YTick
 *
 * Get whether the tick has a label
 *
 * Returns: %TRUE or %FALSE
 **/
#define y_tick_is_labelled(x) ((x)->label != NULL)

/**
 * y_tick_label:
 * @x: a #YTick
 *
 * Get the label
 *
 * Returns: a string
 **/
#define y_tick_label(x) ((x)->label)

/**********************/

G_DECLARE_FINAL_TYPE(YAxisMarkers,y_axis_markers,Y,AXIS_MARKERS,GObject)

#define Y_TYPE_AXIS_MARKERS (y_axis_markers_get_type())

YAxisMarkers *y_axis_markers_new (void);

void y_axis_markers_freeze (YAxisMarkers *am);
void y_axis_markers_thaw (YAxisMarkers *am);

gint y_axis_markers_size (YAxisMarkers *am);

const YTick *y_axis_markers_get (YAxisMarkers *am, gint i);

void y_axis_markers_clear (YAxisMarkers *am);

void y_axis_markers_add (YAxisMarkers *am, double pos, gint type, const gchar *label);
void y_axis_markers_add_critical (YAxisMarkers *am, double pos, gint type, const gchar *label);
void y_axis_markers_sort (YAxisMarkers *am);

void y_axis_markers_populate_scalar (YAxisMarkers *am,
					 double pos_min, double pos_max,
					 gint goal, gint radix,
					 gboolean percentage);

void y_axis_markers_populate_scalar_log (YAxisMarkers *am,
					     double min, double max,
					     gint goal, double base);

/*void y_axis_markers_populate_dates (YAxisMarkers *gam,
					GDate *min, GDate *max);*/

void y_axis_markers_populate_generic (YAxisMarkers *am,
					  gint type,
					  double min, double max);

G_END_DECLS

#endif /* _INC_Y_AXIS_MARKERS_H */

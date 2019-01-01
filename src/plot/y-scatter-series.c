/*
 * y-scatter-series.c
 *
 * Copyright (C) 2000 EMC Capital Management, Inc.
 * Copyright (C) 2018 Scott O. Johnson (scojo202@gmail.com)
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

#include "plot/y-scatter-series.h"
#include <math.h>
#include "data/y-data-class.h"
#include "data/y-data-simple.h"

/**
 * SECTION: y-scatter-series
 * @short_description: Object holding X and Y data for a scatter/line plot.
 *
 * Controls for a pair of X and Y data shown in a line/scatter plot. Holds X
 * and Y vectors and style information.
 *
 *
 */

static GObjectClass *parent_class = NULL;

#define PROFILE 0

enum
{
  SCATTER_SERIES_X_DATA = 1,
  SCATTER_SERIES_Y_DATA,
  SCATTER_SERIES_DRAW_LINE,
  SCATTER_SERIES_LINE_COLOR,
  SCATTER_SERIES_LINE_WIDTH,
  SCATTER_SERIES_LINE_DASHING,
  SCATTER_SERIES_DRAW_MARKERS,
  SCATTER_SERIES_MARKER,
  SCATTER_SERIES_MARKER_COLOR,
  SCATTER_SERIES_MARKER_SIZE,
};

struct _YScatterSeries
{
  GObject base;
  YVector *xdata;
  YVector *ydata;

  gboolean draw_line, draw_markers;
  GdkRGBA line_color, marker_color;
  double line_width, marker_size;
  YMarkerType marker;
};

G_DEFINE_TYPE (YScatterSeries, y_scatter_series, G_TYPE_OBJECT);

static void
y_scatter_series_finalize (GObject * obj)
{
  if (parent_class->finalize)
    parent_class->finalize (obj);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

/**
 * y_scatter_series_set_line_color_from_string:
 * @ss: a #YScatterSeries
 * @colorstring: a string specifying a color, e.g. "#ff0000"
 *
 * Set the color to use to draw the line in a scatter plot for the data in @ss.
 **/
void
y_scatter_series_set_line_color_from_string (YScatterSeries * ss,
					     gchar * colorstring)
{
  g_return_if_fail(Y_IS_SCATTER_SERIES(ss));
  GdkRGBA c;
  gboolean success = gdk_rgba_parse (&c, colorstring);
  if (success)
    g_object_set (ss, "line-color", &c, NULL);
  else
    g_warning ("Failed to parse line color string %s", colorstring);
}

/**
 * y_scatter_series_set_marker_color_from_string:
 * @ss: a #YScatterSeries
 * @colorstring: a string specifying a color, e.g. "#ff0000"
 *
 * Set the color to use to draw markers in a scatter plot for the data in @ss.
 **/
void
y_scatter_series_set_marker_color_from_string (YScatterSeries * ss,
					       gchar * colorstring)
{
  g_return_if_fail(Y_IS_SCATTER_SERIES(ss));
  GdkRGBA c;
  gboolean success = gdk_rgba_parse (&c, colorstring);
  if (success)
    g_object_set (ss, "marker-color", &c, NULL);
  else
    g_warning ("Failed to parse marker color string %s", colorstring);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
y_scatter_series_set_property (GObject * object,
                               guint property_id,
                               const GValue * value,
                               GParamSpec * pspec)
{
  YScatterSeries *self = (YScatterSeries *) object;
  g_debug ("set_property: %d", property_id);

  switch (property_id)
    {
    case SCATTER_SERIES_X_DATA:
      {
        self->xdata = g_value_dup_object (value);
      }
      break;
    case SCATTER_SERIES_Y_DATA:
      {
        self->ydata = g_value_dup_object (value);
      }
      break;
    case SCATTER_SERIES_DRAW_LINE:
      {
        self->draw_line = g_value_get_boolean (value);
      }
      break;
    case SCATTER_SERIES_LINE_COLOR:
      {
        GdkRGBA *c = g_value_get_pointer (value);
        self->line_color = *c;
      }
      break;
    case SCATTER_SERIES_LINE_WIDTH:
      {
        self->line_width = g_value_get_double (value);
      }
      break;
    case SCATTER_SERIES_DRAW_MARKERS:
      {
        self->draw_markers = g_value_get_boolean (value);
      }
      break;
    case SCATTER_SERIES_MARKER:
      {
        self->marker = g_value_get_int (value);
      }
      break;
    case SCATTER_SERIES_MARKER_COLOR:
      {
        GdkRGBA *c = g_value_get_pointer (value);
        self->marker_color = *c;
      }
      break;
    case SCATTER_SERIES_MARKER_SIZE:
      {
        self->marker_size = g_value_get_double (value);
      }
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
y_scatter_series_get_property (GObject * object,
                               guint property_id,
                               GValue * value,
                               GParamSpec * pspec)
{
  YScatterSeries *self = (YScatterSeries *) object;
  switch (property_id)
    {
    case SCATTER_SERIES_X_DATA:
      {
        g_value_set_object (value, self->xdata);
      }
      break;
    case SCATTER_SERIES_Y_DATA:
      {
        g_value_set_object (value, self->ydata);
      }
      break;
    case SCATTER_SERIES_DRAW_LINE:
      {
        g_value_set_boolean (value, self->draw_line);
      }
      break;
    case SCATTER_SERIES_LINE_COLOR:
      {
        g_value_set_pointer (value, &self->line_color);
      }
      break;
    case SCATTER_SERIES_LINE_WIDTH:
      {
        g_value_set_double (value, self->line_width);
      }
      break;
    case SCATTER_SERIES_DRAW_MARKERS:
      {
        g_value_set_boolean (value, self->draw_markers);
      }
      break;
    case SCATTER_SERIES_MARKER:
      {
        g_value_set_int (value, self->marker);
      }
      break;
    case SCATTER_SERIES_MARKER_COLOR:
      {
        g_value_set_pointer (value, &self->marker_color);
      }
      break;
    case SCATTER_SERIES_MARKER_SIZE:
      {
        g_value_set_double (value, self->marker_size);
      }
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

#define DEFAULT_DRAW_LINE (TRUE)
#define DEFAULT_DRAW_MARKERS (FALSE)
#define DEFAULT_LINE_WIDTH 1.0
#define DEFAULT_MARKER_SIZE 4.0

static void
y_scatter_series_class_init (YScatterSeriesClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->set_property = y_scatter_series_set_property;
  object_class->get_property = y_scatter_series_get_property;

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = y_scatter_series_finalize;

  /* properties */
  g_object_class_install_property (object_class, SCATTER_SERIES_X_DATA,
				   g_param_spec_object ("x-data",
							 "X Data",
							 "Vector for horizontal axis",
               Y_TYPE_VECTOR,
							 G_PARAM_READWRITE |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, SCATTER_SERIES_Y_DATA,
             				   g_param_spec_object ("y-data",
             							 "Y Data",
             							 "Vector for vertical axis",
                            Y_TYPE_VECTOR,
             							 G_PARAM_READWRITE |
             							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, SCATTER_SERIES_DRAW_LINE,
				   g_param_spec_boolean ("draw-line",
							 "Draw Line",
							 "Whether to draw a line between points",
							 DEFAULT_DRAW_LINE,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, SCATTER_SERIES_LINE_COLOR,
				   g_param_spec_pointer ("line-color",
							 "Line Color",
							 "The line color",
							 G_PARAM_READWRITE |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, SCATTER_SERIES_LINE_WIDTH,
				   g_param_spec_double ("line-width",
							"Line Width",
							"The line width in pixels",
							0.0, 100.0,
							DEFAULT_LINE_WIDTH,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  // dashing

  g_object_class_install_property (object_class, SCATTER_SERIES_LINE_DASHING,
				   g_param_spec_value_array ("line-dashing",
							     "Line Dashing",
							     "Array for dashing",
							     g_param_spec_double
							     ("dash", "", "",
							      0.0, 100.0, 1.0,
							      G_PARAM_READWRITE),
							     G_PARAM_READWRITE
							     |
							     G_PARAM_STATIC_STRINGS));

  // marker-related

  g_object_class_install_property (object_class, SCATTER_SERIES_DRAW_MARKERS,
				   g_param_spec_boolean ("draw-markers",
							 "Draw Markers",
							 "Whether to draw markers at points",
							 DEFAULT_DRAW_MARKERS,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, SCATTER_SERIES_MARKER,
				   g_param_spec_int ("marker", "Marker",
						     "The marker",
						     MARKER_NONE,
						     MARKER_UNKNOWN,
						     MARKER_CIRCLE,
						     G_PARAM_READWRITE |
						     G_PARAM_CONSTRUCT |
						     G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, SCATTER_SERIES_MARKER_COLOR,
				   g_param_spec_pointer ("marker-color",
							 "Marker Color",
							 "The marker color",
							 G_PARAM_READWRITE |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, SCATTER_SERIES_MARKER_SIZE,
				   g_param_spec_double ("marker-size",
							"Marker Size",
							"The marker size in pixels",
							0.0, 100.0,
							DEFAULT_MARKER_SIZE,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));
}

static void
y_scatter_series_init (YScatterSeries * obj)
{
  obj->line_color.alpha = 1.0;
  obj->marker_color.alpha = 1.0;

  g_debug ("y_scatter_series_init");
}

/**
 * y_scatter_series_set_x_array:
 * @ss: a #YScatterSeries
 * @arr: (array length=n): array of doubles
 * @n: length of array
 *
 * Creates a #YValVector and adds to the series as its X vector.
 *
 * Returns: (transfer none): the #YValVector as a #YData
 **/
YData *y_scatter_series_set_x_array(YScatterSeries *ss, const double *arr, guint n)
{
  g_return_val_if_fail(Y_IS_SCATTER_SERIES(ss),NULL);
  g_return_val_if_fail(arr!=NULL, NULL);
  YData *v = y_val_vector_new_copy(arr,n);
  ss->xdata = g_object_ref_sink(v);
  /* notify */
  return v;
}

/**
 * y_scatter_series_set_y_array:
 * @ss: a #YScatterSeries
 * @arr: (array length=n): array of doubles
 * @n: length of array
 *
 * Creates a #YValVector and adds to the series as its Y vector.
 *
 * Returns: (transfer none): the #YValVector as a #YData
 **/
YData *y_scatter_series_set_y_array(YScatterSeries *ss, const double *arr, guint n)
{
  g_return_val_if_fail(Y_IS_SCATTER_SERIES(ss),NULL);
  g_return_val_if_fail(arr!=NULL, NULL);
  YData *v = y_val_vector_new_copy(arr,n);
  ss->ydata = g_object_ref_sink(v);
  /* notify */
  return v;
}

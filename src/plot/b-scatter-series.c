/*
 * b-scatter-series.c
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

#include "b-plot-enums.h"
#include "plot/b-scatter-series.h"
#include <math.h>
#include "data/b-data-class.h"
#include "data/b-data-simple.h"

/**
 * SECTION: b-scatter-series
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
  SCATTER_SERIES_X_ERR,
  SCATTER_SERIES_Y_ERR,
	SCATTER_SERIES_SHOW,
  SCATTER_SERIES_DRAW_LINE,
  SCATTER_SERIES_LINE_COLOR,
  SCATTER_SERIES_LINE_WIDTH,
  SCATTER_SERIES_LINE_DASHING,
  SCATTER_SERIES_MARKER,
  SCATTER_SERIES_MARKER_COLOR,
  SCATTER_SERIES_MARKER_SIZE,
  SCATTER_SERIES_LABEL,
  SCATTER_SERIES_TOOLTIP
};

struct _BScatterSeries
{
  GInitiallyUnowned base;
  BVector *xdata, *ydata;
  BData *xerr, *yerr;

  unsigned int show : 1;
  unsigned int draw_line : 1;
  GdkRGBA line_color, marker_color;
  double line_width, marker_size;
  BMarker marker;
  gchar *label, *tooltip;
  BDashing dashing;
};

G_DEFINE_TYPE (BScatterSeries, b_scatter_series, G_TYPE_INITIALLY_UNOWNED);

static void
b_scatter_series_finalize (GObject * obj)
{
  BScatterSeries *s = (BScatterSeries *) obj;
  g_clear_object(&s->xdata);
  g_clear_object(&s->ydata);
  g_clear_object(&s->xerr);
  g_clear_object(&s->yerr);
  g_clear_pointer(&s->label,g_free);

  if (parent_class->finalize)
    parent_class->finalize (obj);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

/**
 * b_scatter_series_set_line_color_from_string:
 * @ss: a #BScatterSeries
 * @colorstring: a string specifying a color, e.g. "#ff0000"
 *
 * Set the color to use to draw the line in a scatter plot for the data in @ss.
 **/
void
b_scatter_series_set_line_color_from_string (BScatterSeries * ss,
					     gchar * colorstring)
{
  g_return_if_fail(B_IS_SCATTER_SERIES(ss));
  GdkRGBA c;
  gboolean success = gdk_rgba_parse (&c, colorstring);
  if (success)
    g_object_set (ss, "line-color", &c, NULL);
  else
    g_warning ("Failed to parse line color string %s", colorstring);
}

/**
 * b_scatter_series_set_marker_color_from_string:
 * @ss: a #BScatterSeries
 * @colorstring: a string specifying a color, e.g. "#ff0000"
 *
 * Set the color to use to draw markers in a scatter plot for the data in @ss.
 **/
void
b_scatter_series_set_marker_color_from_string (BScatterSeries * ss,
					       gchar * colorstring)
{
  g_return_if_fail(B_IS_SCATTER_SERIES(ss));
  GdkRGBA c;
  gboolean success = gdk_rgba_parse (&c, colorstring);
  if (success)
    g_object_set (ss, "marker-color", &c, NULL);
  else
    g_warning ("Failed to parse marker color string %s", colorstring);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
b_scatter_series_set_property (GObject * object,
                               guint property_id,
                               const GValue * value,
                               GParamSpec * pspec)
{
  BScatterSeries *self = (BScatterSeries *) object;

  switch (property_id)
    {
    case SCATTER_SERIES_X_DATA:
      {
        self->xdata = g_value_get_object (value);
        if(self->xdata) {
          g_object_ref_sink(self->xdata);
          b_data_emit_changed(B_DATA(self->xdata));
        }
      }
      break;
    case SCATTER_SERIES_Y_DATA:
      {
        self->ydata = g_value_get_object (value);
        if(self->ydata) {
          g_object_ref_sink(self->ydata);
          b_data_emit_changed(B_DATA(self->ydata));
        }
      }
      break;
    case SCATTER_SERIES_X_ERR:
      {
        GObject *d = g_value_get_object (value);
        if(d == NULL || B_IS_SCALAR(d) || B_IS_VECTOR(d))
          self->xerr = g_value_get_object (value);
        if(self->xerr) {
          g_object_ref_sink(self->xerr);
          b_data_emit_changed(B_DATA(self->xerr));
        }
      }
      break;
    case SCATTER_SERIES_Y_ERR:
      {
        GObject *d = g_value_get_object (value);
        if(d == NULL || B_IS_SCALAR(d) || B_IS_VECTOR(d))
          self->yerr = g_value_get_object (value);
        if(self->yerr) {
          g_object_ref_sink(self->yerr);
          b_data_emit_changed(B_DATA(self->yerr));
        }
      }
      break;
    case SCATTER_SERIES_SHOW:
        {
          self->show = g_value_get_boolean (value);
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
    case SCATTER_SERIES_LINE_DASHING:
      {
        self->dashing = g_value_get_enum(value);
      }
      break;
    case SCATTER_SERIES_MARKER:
      {
        self->marker = g_value_get_enum (value);
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
    case SCATTER_SERIES_LABEL:
      {
        self->label = g_value_dup_string (value);
        break;
      }
    case SCATTER_SERIES_TOOLTIP:
      {
        self->tooltip = g_value_dup_string (value);
        break;
      }
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
b_scatter_series_get_property (GObject * object,
                               guint property_id,
                               GValue * value,
                               GParamSpec * pspec)
{
  BScatterSeries *self = (BScatterSeries *) object;
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
    case SCATTER_SERIES_X_ERR:
      {
        g_value_set_object (value, self->xerr);
      }
      break;
    case SCATTER_SERIES_Y_ERR:
      {
        g_value_set_object (value, self->yerr);
      }
      break;
    case SCATTER_SERIES_SHOW:
        {
          g_value_set_boolean (value, self->show);
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
    case SCATTER_SERIES_LINE_DASHING:
      {
        g_value_set_enum (value, self->dashing);
      }
      break;
    case SCATTER_SERIES_MARKER:
      {
        g_value_set_enum (value, self->marker);
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
    case SCATTER_SERIES_LABEL:
      {
        g_value_set_string (value, self->label);
      }
      break;
    case SCATTER_SERIES_TOOLTIP:
      {
        g_value_set_string (value, self->tooltip);
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
#define DEFAULT_MARKER_SIZE 5.0

static void
b_scatter_series_class_init (BScatterSeriesClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->set_property = b_scatter_series_set_property;
  object_class->get_property = b_scatter_series_get_property;

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = b_scatter_series_finalize;

  /* properties */
  g_object_class_install_property (object_class, SCATTER_SERIES_X_DATA,
				   g_param_spec_object ("x-data",
							 "X Data",
							 "Vector for horizontal axis",
               B_TYPE_VECTOR,
							 G_PARAM_READWRITE |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, SCATTER_SERIES_Y_DATA,
             				   g_param_spec_object ("y-data",
             							 "Y Data",
             							 "Vector for vertical axis",
                            B_TYPE_VECTOR,
             							 G_PARAM_READWRITE |
             							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, SCATTER_SERIES_X_ERR,
             				   g_param_spec_object ("x-err",
             							 "X Error",
             							 "Error bar data for horizontal axis. Can be a #YScalar or #YVector",
                            B_TYPE_DATA,
             							 G_PARAM_READWRITE |
             							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, SCATTER_SERIES_Y_ERR,
            				   g_param_spec_object ("y-err",
            							 "Y Error",
            							 "Error bar data for vertical axis. Can be a #YScalar or #YVector",
                           B_TYPE_DATA,
            							 G_PARAM_READWRITE |
            							 G_PARAM_STATIC_STRINGS));

   g_object_class_install_property (object_class, SCATTER_SERIES_SHOW,
 				   g_param_spec_boolean ("show",
 							 "Show series",
 							 "Whether to draw the series",
 							 TRUE,
 							 G_PARAM_READWRITE |
 							 G_PARAM_CONSTRUCT |
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
				   g_param_spec_enum ("dashing",
							     "Line Dashing",
							     "Preset line dashing",
							     B_TYPE_DASHING,
                   B_DASHING_SOLID,
							     G_PARAM_READWRITE |
                   G_PARAM_CONSTRUCT |
							     G_PARAM_STATIC_STRINGS));

  // marker-related

  g_object_class_install_property (object_class, SCATTER_SERIES_MARKER,
				   g_param_spec_enum ("marker", "Marker",
						     "The marker",
						     B_TYPE_MARKER,
						     B_MARKER_NONE,
						     G_PARAM_READWRITE |
						     G_PARAM_CONSTRUCT |
						     G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, SCATTER_SERIES_MARKER_COLOR,
				   g_param_spec_pointer ("marker-color",
							 "Marker Color",
							 "The marker color",
							 G_PARAM_READWRITE |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, SCATTER_SERIES_LABEL,
    g_param_spec_string("label","Label","The string associated with the series. Used, for example, in the legend.",
                        "Untitled", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, SCATTER_SERIES_TOOLTIP,
    g_param_spec_string("tooltip","Tooltip","The string to be used in a tooltip, typically to show more information than the label.",
                        "", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, SCATTER_SERIES_MARKER_SIZE,
				   g_param_spec_double ("marker-size",
							"Marker Size",
							"The marker size in pixels",
							1.0, 100.0,
							DEFAULT_MARKER_SIZE,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));
}

static void
b_scatter_series_init (BScatterSeries * obj)
{
  obj->line_color.alpha = 1.0;
  obj->marker_color.alpha = 1.0;
}

/**
 * b_scatter_series_set_x_array:
 * @ss: a #BScatterSeries
 * @arr: (array length=n): array of doubles
 * @n: length of array
 *
 * Creates a #BValVector and adds to the series as its X vector.
 *
 * Returns: (transfer none): the #BValVector as a #BData
 **/
BData *b_scatter_series_set_x_array(BScatterSeries *ss, const double *arr, guint n)
{
  g_return_val_if_fail(B_IS_SCATTER_SERIES(ss),NULL);
  g_return_val_if_fail(arr!=NULL, NULL);
  BData *v = b_val_vector_new_copy(arr,n);
  ss->xdata = B_VECTOR(g_object_ref_sink(v));
  /* notify */
  return v;
}

/**
 * b_scatter_series_set_y_array:
 * @ss: a #BScatterSeries
 * @arr: (array length=n): array of doubles
 * @n: length of array
 *
 * Creates a #BValVector and adds to the series as its Y vector.
 *
 * Returns: (transfer none): the #BValVector as a #BData
 **/
BData *b_scatter_series_set_y_array(BScatterSeries *ss, const double *arr, guint n)
{
  g_return_val_if_fail(B_IS_SCATTER_SERIES(ss),NULL);
  g_return_val_if_fail(arr!=NULL, NULL);
  BData *v = b_val_vector_new_copy(arr,n);
  ss->ydata = B_VECTOR(g_object_ref_sink(v));
  /* notify */
  return v;
}

/**
 * b_scatter_series_get_show:
 * @ss: a #BScatterSeries
 *
 * Get whether to actually draw the series.
 *
 * Returns %TRUE if the series should be drawn.
 **/
gboolean b_scatter_series_get_show(BScatterSeries *ss)
{
  g_return_val_if_fail(B_IS_SCATTER_SERIES(ss),FALSE);
  return ss->show;
}

cairo_surface_t *_b_scatter_series_create_legend_image(BScatterSeries *series)
{
  const int width = 30;
  const int height = 20;
  cairo_surface_t *surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,width,height);

  cairo_t *cr = cairo_create(surf);
  cairo_set_source_rgb(cr,1.0,1.0,1.0);
  cairo_rectangle(cr,0,0,width,height);
  cairo_fill(cr);

  gboolean draw_line;
  double line_width;
  GdkRGBA *line_color;
  BDashing dash;

  g_object_get (series, "draw-line", &draw_line, "line-width", &line_width,
		"line-color", &line_color, "dashing", &dash, NULL);

  if(draw_line) {
    cairo_set_line_width (cr, line_width);

    cairo_set_source_rgba (cr, line_color->red, line_color->green,
         line_color->blue, line_color->alpha);

    _b_dashing_set(dash, line_width, cr);

    cairo_move_to(cr,4,height/2+0.5);
    cairo_line_to(cr,width-4,height/2+0.5);
    cairo_stroke(cr);
  }

  GdkRGBA *marker_color;
  double marker_size;
  BMarker marker_type;

  g_object_get (series, "marker-color", &marker_color,
                        "marker-size", &marker_size,
                        "marker", &marker_type, NULL);

  if (marker_type != B_MARKER_NONE)
    {
      cairo_set_source_rgba (cr, marker_color->red, marker_color->green,
                                 marker_color->blue, marker_color->alpha);

      BPoint pos;
      pos.x = width/2;
      pos.y = height/2+0.5;

      switch (marker_type)
      {
        case B_MARKER_CIRCLE:
          _draw_marker_circle (cr, pos, marker_size, TRUE);
          break;
        case B_MARKER_OPEN_CIRCLE:
          _draw_marker_circle (cr, pos, marker_size, FALSE);
          break;
        case B_MARKER_SQUARE:
          _draw_marker_square (cr, pos, marker_size, TRUE);
          break;
        case B_MARKER_OPEN_SQUARE:
          _draw_marker_square (cr, pos, marker_size, FALSE);
          break;
        case B_MARKER_DIAMOND:
          _draw_marker_diamond (cr, pos, marker_size, TRUE);
          break;
        case B_MARKER_OPEN_DIAMOND:
          _draw_marker_diamond (cr, pos, marker_size, FALSE);
          break;
        case B_MARKER_X:
          _draw_marker_x (cr, pos, marker_size);
          break;
        case B_MARKER_PLUS:
          _draw_marker_plus (cr, pos, marker_size);
          break;
        default:
          break;
      }
    }

  cairo_destroy(cr);

  return surf;
}

void _b_dashing_set(BDashing d, double line_width, cairo_t *cr)
{
  double dash[4];
  switch(d)
    {
      case B_DASHING_SOLID:
        cairo_set_dash(cr, dash, 0, 0.0);
        break;
      case B_DASHING_DOTTED:
        dash[0]=0.0;
        dash[1]=3*line_width;
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
        cairo_set_dash(cr, dash, 2, 0.0);
        break;
      case B_DASHING_DASHED:
        dash[0]=6*line_width;
        cairo_set_dash(cr, dash, 1, 0.0);
        break;
      case B_DASHING_DOT_DASH:
        dash[0]=0.0;
        dash[1]=3*line_width;
        dash[2]=6*line_width;
        dash[3]=3*line_width;
        cairo_set_dash(cr,dash,4,0.0);
      default:
        break;
    }
}

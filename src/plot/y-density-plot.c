/*
 * y-density-plot.c
 *
 * Copyright (C) 2016 Scott O. Johnson (scojo202@gmail.com)
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

#include "plot/y-density-plot.h"
#include <math.h>

/* TODO */
/*

Allow autoscaling symmetrically (for phase) or asymetrically (for spot)
Color bar - separate class, use Z view interval

*/

#define CREATE_SURF 1

enum
{
  DENSITY_PLOT_DATA = 1,
  DENSITY_PLOT_XMIN,
  DENSITY_PLOT_DX,
  DENSITY_PLOT_YMIN,
  DENSITY_PLOT_DY,
  DENSITY_PLOT_ZMAX,
  DENSITY_PLOT_AUTO_Z,
  DENSITY_PLOT_DRAW_LINE,
  DENSITY_PLOT_LINE_DIR,
  DENSITY_PLOT_LINE_POS,
  DENSITY_PLOT_LINE_WIDTH,
  DENSITY_PLOT_DRAW_DOT,
  DENSITY_PLOT_DOT_X,
  DENSITY_PLOT_DOT_Y,
  DENSITY_PLOT_PRESERVE_ASPECT,
  N_PROPERTIES
};

static GObjectClass *parent_class;

static gboolean
preferred_range (YElementViewCartesian * cart, YAxisType ax, double *a,
		 double *b)
{
  YDensityPlot *widget = Y_DENSITY_PLOT (cart);
  *a = 0;
  *b = 1;
  if (widget->tdata != NULL)
    {
      YMatrixSize size = y_matrix_get_size (widget->tdata);
      if (ax == X_AXIS)
	{
	  *a = widget->xmin;
	  *b = widget->xmin + size.columns * widget->dx;
	}
      else if (ax == Y_AXIS)
	{
	  *a = widget->ymin;
	  *b = widget->ymin + size.rows * widget->dy;
	}
      else
	{
	  *b = 1;
	}
    }

  return TRUE;
}

static GtkSizeRequestMode
get_request_mode (GtkWidget * w)
{
  return GTK_SIZE_REQUEST_WIDTH_FOR_HEIGHT;
  //return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
get_preferred_width (GtkWidget * w, gint * minimum, gint * natural)
{
  YDensityPlot *widget = Y_DENSITY_PLOT (w);
  *minimum = 10;
  if (widget->tdata == NULL)
    *natural = 2000;
  else
    *natural = y_matrix_get_columns (widget->tdata);
}

static void
get_preferred_height (GtkWidget * w, gint * minimum, gint * natural)
{
  YDensityPlot *widget = Y_DENSITY_PLOT (w);
  *minimum = 10;
  if (widget->tdata == NULL)
    *natural = 2000;
  else
    *natural = y_matrix_get_rows (widget->tdata);
}

static void
get_preferred_height_for_width (GtkWidget * w, gint for_width, gint * minimum,
				gint * natural)
{
  YDensityPlot *widget = Y_DENSITY_PLOT (w);
  if (widget->aspect_ratio > 0)
    {
      *natural = ((float) for_width) / widget->aspect_ratio;
    }
  else
    {
      if (widget->tdata == NULL)
	*natural = 200;
      else
	*natural = y_matrix_get_rows (widget->tdata);
    }
  *minimum = (*natural >= 50 ? 50 : *natural);

  g_debug ("density plot requesting height %d for width %d", *natural,
	   for_width);
}

static void
get_preferred_width_for_height (GtkWidget * w, gint for_height,
				gint * minimum, gint * natural)
{
  YDensityPlot *widget = Y_DENSITY_PLOT (w);
  if (widget->aspect_ratio > 0)
    {
      *natural = ((float) for_height) * widget->aspect_ratio;
    }
  else
    {
      if (widget->tdata == NULL)
	*natural = 200;
      else
	*natural = y_matrix_get_columns (widget->tdata);
    }
  *minimum = (*natural >= 50 ? 50 : *natural);

  g_debug ("density plot requesting width %d for height %d, minimum %d",
	   *natural, for_height, *minimum);
}

/* called when data size changes */

static void
y_density_plot_update_surface (YDensityPlot * widget)
{
  int width = 0;
  int height = 0;

  gboolean need_resize = FALSE;

  if (widget->pixbuf != NULL)
    {
      g_object_get (widget->pixbuf, "height", &height, "width", &width, NULL);
    }
  else
    need_resize = TRUE;

  YMatrixSize size = y_matrix_get_size (widget->tdata);

  if (size.rows == height || size.columns == width)
    return;

  g_debug ("update_surface: %d -> %d, %d -> %d", size.rows, height,
	   size.columns, width);

  if (widget->pixbuf != NULL)
    {
      g_object_unref (widget->pixbuf);
      widget->pixbuf = NULL;
    }
  need_resize = TRUE;

  //if(widget->tdata->size.rows < 1 || widget->tdata->size.columns < 1) return;

  if (need_resize)
    {
      widget->pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
				       size.columns, size.rows);
    }
}

static void
y_density_plot_rescale (YDensityPlot * widget)
{
  int width, height;
  width = gtk_widget_get_allocated_width (GTK_WIDGET (widget));
  height = gtk_widget_get_allocated_height (GTK_WIDGET (widget));

  YMatrixSize size = y_matrix_get_size (widget->tdata);

  size_t nrow = size.rows;
  size_t ncol = size.columns;

  widget->scalex = ((double) width) / ncol;
  widget->scaley = ((double) height) / nrow;

  double aspect = (widget->scalex / widget->scaley);
  if (fabs (aspect - 1.0) < 1e-2)
    {
      if (widget->scaley < widget->scalex)
	widget->scalex = widget->scaley;
      else
	widget->scaley = widget->scalex;
    }
}

static unsigned char
b_red (float scaled)
{
  if (scaled <= 0.5f)
    return 0;
  if (scaled < 0.75f)
    return 255 * (scaled - 0.5f) * 4;
  if (scaled <= 1.0f)
    return 255;
  return 0;
}

static unsigned char
b_blue (float scaled)
{
  if (scaled >= 0.5f)
    return 0;
  if (scaled > 0.25f)
    return 255 * (0.25f - scaled) * 4;
  if (scaled >= 0.0f)
    return 255;
  return 0;
}

static unsigned char
b_green (float scaled)
{
  if (scaled <= 0.75f && scaled >= 0.25f)
    return 0;
  if (scaled < 0.25f)
    return 255 * (0.25f - scaled) * 4;
  if (scaled > 0.75f)
    return 255 * (scaled - 0.75f) * 4;
  return 0;
}

static void
on_data_changed (YData * dat, gpointer user_data)
{
  YElementView *mev = (YElementView *) user_data;
  g_assert (mev);

  YDensityPlot *widget = Y_DENSITY_PLOT (user_data);

  if (widget->tdata == NULL)
    {
      return;
    }

  const double *data = y_matrix_get_values (widget->tdata);
  YMatrixSize size = y_matrix_get_size (widget->tdata);

  size_t nrow = size.rows;
  size_t ncol = size.columns;

  y_density_plot_update_surface (widget);

  int i, j;

  //GTimer *t = g_timer_new();

  if (widget->auto_z)
    {
      double mn, mx;
      y_matrix_get_minmax (widget->tdata, &mn, &mx);

      widget->zmax = MAX (fabs (mx), fabs (mn));
    }
  double dmax = widget->zmax;

  int n_channels = gdk_pixbuf_get_n_channels (widget->pixbuf);
  int rowstride = gdk_pixbuf_get_rowstride (widget->pixbuf);
  guchar *pixels = gdk_pixbuf_get_pixels (widget->pixbuf);

  unsigned char lut[256 * 4];

  float dl = 1 / 256.0f;
  for (i = 0; i < 256; i++)
    {
      lut[4 * i] = b_blue (i * dl);
      lut[4 * i + 1] = b_green (i * dl);
      lut[4 * i + 2] = b_red (i * dl);
    }

  for (i = 0; i < nrow; i++)
    {
      for (j = 0; j < ncol; j++)
	{
	  if (__builtin_isnan (data[i * ncol + j]))
	    {			/* math.h isnan() is really slow */
	      pixels[n_channels * j + (nrow - 1 - i) * rowstride] = 0;
	      pixels[n_channels * j + (nrow - 1 - i) * rowstride + 1] = 0;
	      pixels[n_channels * j + (nrow - 1 - i) * rowstride + 2] = 0;
	    }
	  else
	    {
	      int ss = (int) ((data[i * ncol + j] + dmax) / (2 * dmax) * 255);
	      if (ss >= 0 && ss < 256)
		{
		  pixels[n_channels * j + (nrow - 1 - i) * rowstride] =
		    lut[4 * ss + 2];
		  pixels[n_channels * j + (nrow - 1 - i) * rowstride + 1] =
		    lut[4 * ss + 1];
		  pixels[n_channels * j + (nrow - 1 - i) * rowstride + 2] =
		    lut[4 * ss];
		}
	      else
		{
		  //cbuffer[i]=0;
		}
	    }
	}
    }

  //double te = g_timer_elapsed(t,NULL);
  //g_message("fill buffer: %f ms",te*1000);

  if (widget->preserve_aspect)
    widget->aspect_ratio = ((float) size.columns / ((float) size.rows));
  else
    widget->aspect_ratio = -1;

  gtk_widget_queue_resize (GTK_WIDGET (widget));

  y_element_view_changed (mev);
}

static void
y_density_plot_do_popup_menu (GtkWidget * my_widget, GdkEventButton * event)
{
  YElementViewCartesian *view = Y_ELEMENT_VIEW_CARTESIAN (my_widget);

  GtkWidget *menu;
  int button, event_time;

  menu = gtk_menu_new ();

  GtkWidget *autoscale_x =
    _y_create_autoscale_menu_check_item (view, X_AXIS,"Autoscale X axis");
  gtk_widget_show (autoscale_x);
  GtkWidget *autoscale_y =
    _y_create_autoscale_menu_check_item (view, Y_AXIS, "Autoscale Y axis");
  gtk_widget_show (autoscale_y);

  gtk_menu_shell_append (GTK_MENU_SHELL (menu), autoscale_x);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), autoscale_y);

  if (event)
    {
      button = event->button;
      event_time = event->time;
    }
  else
    {
      button = 0;
      event_time = gtk_get_current_event_time ();
    }

  gtk_menu_attach_to_widget (GTK_MENU (menu), my_widget, NULL);
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
		  button, event_time);
}

static gboolean
y_density_plot_scroll_event (GtkWidget * widget, GdkEventScroll * event)
{
  YElementViewCartesian *view = Y_ELEMENT_VIEW_CARTESIAN (widget);

  gboolean scroll = FALSE;
  gboolean direction;
  if (event->direction == GDK_SCROLL_UP)
    {
      scroll = TRUE;
      direction = TRUE;
    }
  else if (event->direction == GDK_SCROLL_DOWN)
    {
      scroll = TRUE;
      direction = FALSE;
    }
  if (!scroll)
    return FALSE;

  YViewInterval *viy = y_element_view_cartesian_get_view_interval (view,
								   Y_AXIS);
  YViewInterval *vix = y_element_view_cartesian_get_view_interval (view,
								   X_AXIS);

  double scale = direction ? 0.8 : 1.0 / 0.8;

  /* find the cursor position */

  YPoint ip;
  YPoint *evp = (YPoint *) & (event->x);

  _view_invconv (widget, evp, &ip);
  y_view_interval_rescale_around_point (vix,
					y_view_interval_unconv_fn (vix, ip.x),
					scale);
  y_view_interval_rescale_around_point (viy,
					y_view_interval_unconv_fn (viy, ip.y),
					scale);

  return FALSE;
}

static gboolean
y_density_plot_button_press_event (GtkWidget * widget, GdkEventButton * event)
{
  YElementViewCartesian *view = Y_ELEMENT_VIEW_CARTESIAN (widget);

  /* Ignore double-clicks and triple-clicks */
  if (gdk_event_triggers_context_menu ((GdkEvent *) event) &&
      event->type == GDK_BUTTON_PRESS)
    {
      y_density_plot_do_popup_menu (widget, event);
      return TRUE;
    }

  if (event->button == 1 && (event->state & GDK_SHIFT_MASK))
    {
      YViewInterval *viy = y_element_view_cartesian_get_view_interval (view,
								       Y_AXIS);
      YViewInterval *vix = y_element_view_cartesian_get_view_interval (view,
								       X_AXIS);

      YPoint ip;
      YPoint *evp = (YPoint *) & (event->x);

      _view_invconv (widget, evp, &ip);

      y_view_interval_recenter_around_point (vix,
					     y_view_interval_unconv_fn (vix,
									ip.
									x));
      y_view_interval_recenter_around_point (viy,
					     y_view_interval_unconv_fn (viy,
									ip.
									y));
    }

  return FALSE;
}

static gboolean
y_density_plot_draw (GtkWidget * w, cairo_t * cr)
{
  YDensityPlot *widget = Y_DENSITY_PLOT (w);

  if (widget->pixbuf == NULL || widget->tdata == NULL)
    {
      g_debug ("density plot draw2: %d %d", widget->scaled_pixbuf == NULL,
	       widget->tdata == NULL);
      return FALSE;
    }

  YMatrixSize size = y_matrix_get_size (widget->tdata);

  size_t nrow = size.rows;
  size_t ncol = size.columns;

  if (nrow == 0 || ncol == 0)
    return FALSE;

  int used_width, used_height;

  used_width = ncol * widget->scalex;
  used_height = nrow * widget->scaley;

  g_debug ("density plot using %d by %d", used_width, used_height);
  /*cairo_move_to(cr, 0,0);
     cairo_line_to(cr,0,view->used_height);
     cairo_line_to(cr,view->used_width,view->used_height);
     cairo_line_to(cr,view->used_width,0);
     cairo_line_to(cr,0,0);
     cairo_stroke(cr); */

  int width, height;

  width = gtk_widget_get_allocated_width (w);
  height = gtk_widget_get_allocated_height (w);

  /* calculate new scales based on how view intervals match our parameters */
  double scalex = 1.0;
  double scaley = 1.0;
  double offsetx = 0.0;
  double offsety = 0.0;

  YViewInterval *vix =
    y_element_view_cartesian_get_view_interval (Y_ELEMENT_VIEW_CARTESIAN
						(widget), X_AXIS);
  if (vix != NULL)
    {
      double dx2 = (vix->t1 - vix->t0) / ncol;
      double xmin = vix->t0;

      scalex = widget->scalex * widget->dx / dx2;
      offsetx = (widget->xmin - xmin) / widget->dx * scalex;
    }
  YViewInterval *viy =
    y_element_view_cartesian_get_view_interval (Y_ELEMENT_VIEW_CARTESIAN
						(widget), Y_AXIS);
  if (viy != NULL)
    {
      double dy2 = (viy->t1 - viy->t0) / nrow;
      double ymax = viy->t1;
      double wymax = widget->ymin + widget->dy * nrow;

      scaley = widget->scaley * widget->dy / dy2;
      offsety = -(wymax - ymax) / widget->dy * scaley;
    }

  //g_message("scale: %f %f, offset %f %f",scalex,scaley,offsetx,offsety);

  /* resize scaled pixbuf if necessary */

  int pwidth = 0;
  int pheight = 0;
  if (widget->scaled_pixbuf != NULL)
    g_object_get (widget->scaled_pixbuf, "height", &pheight, "width", &pwidth,
		  NULL);

  if (pheight != height || pwidth != width)
    {
      if (widget->scaled_pixbuf != NULL)
	g_object_unref (widget->scaled_pixbuf);
      widget->scaled_pixbuf =
	gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, width, height);
    }

  gdk_pixbuf_scale (widget->pixbuf, widget->scaled_pixbuf,
		    0, 0,
		    used_width, used_height,
		    offsetx, offsety, scalex, scaley, GDK_INTERP_TILES);
#if CREATE_SURF
  cairo_surface_t *surf =
    gdk_cairo_surface_create_from_pixbuf (widget->scaled_pixbuf, 0,
					  gtk_widget_get_window (GTK_WIDGET
								 (widget)));

  cairo_set_source_surface (cr, surf, 0.0, 0.0);
#else
  gdk_cairo_set_source_pixbuf (cr, widget->scaled_pixbuf, 0, 0);
#endif

  cairo_paint (cr);
#if CREATE_SURF
  cairo_surface_destroy (surf);
#endif

  if (widget->draw_line)
    {
      double pos = widget->line_pos;
      double wid = widget->line_width;

      YPoint p1a, p2a, p1b, p2b;

      cairo_set_source_rgba (cr, 0, 1, 0, 0.5);
      if (widget->line_dir == GTK_ORIENTATION_HORIZONTAL)
	{
	  p1a.x = 0.;
	  p1a.y = pos - wid / 2;
	  p2a.x = 1.;
	  p2a.y = pos - wid / 2;
	  p1b.x = 0.;
	  p1b.y = pos + wid / 2;
	  p2b.x = 1.;
	  p2b.y = pos + wid / 2;
	  if (viy != NULL)
	    {
	      p1a.y = y_view_interval_conv_fn (viy, p1a.y);
	      p2a.y = y_view_interval_conv_fn (viy, p2a.y);
	      p1b.y = y_view_interval_conv_fn (viy, p1b.y);
	      p2b.y = y_view_interval_conv_fn (viy, p2b.y);
	    }
	}
      else
	{
	  p1a.y = 0.;
	  p1a.x = pos - wid / 2;
	  p2a.y = 1.;
	  p2a.x = pos - wid / 2;
	  p1b.y = 0.;
	  p1b.x = pos + wid / 2;
	  p2b.y = 1.;
	  p2b.x = pos + wid / 2;
	  if (vix != NULL)
	    {
	      p1a.x = y_view_interval_conv_fn (vix, p1a.x);
	      p2a.x = y_view_interval_conv_fn (vix, p2a.x);
	      p1b.x = y_view_interval_conv_fn (vix, p1b.x);
	      p2b.x = y_view_interval_conv_fn (vix, p2b.x);
	    }
	}
      _view_conv (w, &p1a, &p1a);
      _view_conv (w, &p2a, &p2a);
      _view_conv (w, &p1b, &p1b);
      _view_conv (w, &p2b, &p2b);
      cairo_move_to (cr, p1a.x, p1a.y);
      cairo_line_to (cr, p2a.x, p2a.y);
      cairo_stroke (cr);
      cairo_move_to (cr, p1b.x, p1b.y);
      cairo_line_to (cr, p2b.x, p2b.y);
      cairo_stroke (cr);
    }

  if (widget->draw_dot)
    {
      double ccx = 0, ccy = 0;
      if ((vix != NULL) && (viy != NULL))
	{
	  ccx = y_view_interval_conv_fn (vix, widget->dot_pos_x);
	  ccy = y_view_interval_conv_fn (viy, widget->dot_pos_y);
	}
      YPoint p = { ccx, ccy };
      YPoint p2;
      _view_conv (w, &p, &p2);

      cairo_arc (cr, p2.x, p2.y, 4, 0, 2 * G_PI);
      cairo_close_path (cr);
      cairo_set_source_rgba (cr, 0, 0, 1, 1);
      cairo_fill_preserve (cr);
      cairo_set_source_rgba (cr, 0, 0, 0, 1);
      cairo_stroke (cr);
    }

  return FALSE;
}

static gboolean
y_density_plot_configure_event (GtkWidget * w, GdkEventConfigure * ev)
{
  YDensityPlot *widget = Y_DENSITY_PLOT (w);

  int width, height;
  width = ev->width;
  height = ev->height;

  if (width < 1 || height < 1)
    return FALSE;

  if (widget->tdata == NULL)
    return FALSE;

  /* set scale from new widget size */

  y_density_plot_rescale (widget);

  g_debug ("density plot allocated %d by %d", ev->width, ev->height);

  return FALSE;
}

static void
changed (YElementView * gev)
{
  y_element_view_cartesian_set_preferred_view ((YElementViewCartesian *) gev,
					       X_AXIS);
  y_element_view_cartesian_set_preferred_view ((YElementViewCartesian *) gev,
					       Y_AXIS);

  YElementViewCartesian *cart = (YElementViewCartesian *) gev;

  YViewInterval *vix =
    y_element_view_cartesian_get_view_interval (cart, X_AXIS);
  YViewInterval *viy =
    y_element_view_cartesian_get_view_interval (cart, Y_AXIS);

  if (vix)
    y_view_interval_request_preferred_range (vix);
  if (viy)
    y_view_interval_request_preferred_range (viy);

  if (Y_ELEMENT_VIEW_CLASS (parent_class)->changed)
    Y_ELEMENT_VIEW_CLASS (parent_class)->changed (gev);
}

static void
y_density_plot_finalize (GObject * obj)
{
  YDensityPlot *self = (YDensityPlot *) obj;
  if (self->tdata != NULL)
    {
      g_signal_handler_disconnect (self->tdata, self->tdata_changed_id);
      g_object_unref (self->tdata);
    }

  if (parent_class->finalize)
    parent_class->finalize (obj);
}

static void
y_density_plot_set_property (GObject * object,
			     guint property_id,
			     const GValue * value, GParamSpec * pspec)
{
  YDensityPlot *self = (YDensityPlot *) object;

  g_debug ("set_property: %d", property_id);

  switch (property_id)
    {
    case DENSITY_PLOT_DATA:
      {
	if (self->tdata)
	  {
	    g_signal_handler_disconnect (self->tdata, self->tdata_changed_id);
	    g_object_unref (self->tdata);
	  }
	//disconnect old data, if present
	self->tdata = g_value_dup_object (value);
	if (self->tdata)
	  {
	    y_density_plot_update_surface (self);
	    /* connect to changed signal */
	    self->tdata_changed_id =
	      g_signal_connect_after (self->tdata, "changed",
				      G_CALLBACK (on_data_changed), self);
	  }
	break;
      }
    case DENSITY_PLOT_XMIN:
      {
	self->xmin = g_value_get_double (value);
      }
      break;
    case DENSITY_PLOT_DX:
      {
	self->dx = g_value_get_double (value);
      }
      break;
    case DENSITY_PLOT_YMIN:
      {
	self->ymin = g_value_get_double (value);
      }
      break;
    case DENSITY_PLOT_DY:
      {
	self->dy = g_value_get_double (value);
      }
      break;
    case DENSITY_PLOT_ZMAX:
      {
	self->zmax = g_value_get_double (value);
	if (self->tdata)
	  y_data_emit_changed (Y_DATA (self->tdata));
      }
      break;
    case DENSITY_PLOT_AUTO_Z:
      {
	self->auto_z = g_value_get_boolean (value);
      }
      break;
    case DENSITY_PLOT_DRAW_LINE:
      {
	self->draw_line = g_value_get_boolean (value);
      }
      break;
    case DENSITY_PLOT_LINE_DIR:
      {
	self->line_dir = g_value_get_int (value);
      }
      break;
    case DENSITY_PLOT_LINE_POS:
      {
	self->line_pos = g_value_get_double (value);
      }
      break;
    case DENSITY_PLOT_LINE_WIDTH:
      {
	self->line_width = g_value_get_double (value);
      }
      break;
    case DENSITY_PLOT_DRAW_DOT:
      {
	self->draw_dot = g_value_get_boolean (value);
      }
      break;
    case DENSITY_PLOT_PRESERVE_ASPECT:
      {
	self->preserve_aspect = g_value_get_boolean (value);
      }
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
y_density_plot_get_property (GObject * object,
			     guint property_id,
			     GValue * value, GParamSpec * pspec)
{
  YDensityPlot *self = (YDensityPlot *) object;
  switch (property_id)
    {
    case DENSITY_PLOT_DATA:
      {
	g_value_set_object (value, self->tdata);
      }
      break;
    case DENSITY_PLOT_XMIN:
      {
	g_value_set_double (value, self->xmin);
      }
      break;
    case DENSITY_PLOT_DX:
      {
	g_value_set_double (value, self->dx);
      }
      break;
    case DENSITY_PLOT_YMIN:
      {
	g_value_set_double (value, self->ymin);
      }
      break;
    case DENSITY_PLOT_DY:
      {
	g_value_set_double (value, self->dy);
      }
      break;
    case DENSITY_PLOT_ZMAX:
      {
	g_value_set_double (value, self->zmax);
      }
      break;
    case DENSITY_PLOT_AUTO_Z:
      {
	g_value_set_boolean (value, self->auto_z);
      }
      break;
    case DENSITY_PLOT_DRAW_LINE:
      {
	g_value_set_boolean (value, self->draw_line);
      }
      break;
    case DENSITY_PLOT_LINE_DIR:
      {
	g_value_set_int (value, self->line_dir);
      }
      break;
    case DENSITY_PLOT_LINE_POS:
      {
	g_value_set_double (value, self->line_pos);
      }
      break;
    case DENSITY_PLOT_LINE_WIDTH:
      {
	g_value_set_double (value, self->line_width);
      }
      break;
    case DENSITY_PLOT_DRAW_DOT:
      {
	g_value_set_boolean (value, self->draw_dot);
      }
      break;
    case DENSITY_PLOT_PRESERVE_ASPECT:
      {
	g_value_set_boolean (value, self->preserve_aspect);
      }
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
y_density_plot_init (YDensityPlot * plot)
{
  plot->tdata = NULL;

  plot->aspect_ratio = 0;

  plot->scaled_pixbuf = NULL;
  plot->pixbuf = NULL;

  gtk_widget_add_events (GTK_WIDGET (plot),
			 GDK_SCROLL_MASK | GDK_BUTTON_PRESS_MASK);

  g_object_set (plot, "expand", FALSE, "valign", GTK_ALIGN_START, "halign",
		GTK_ALIGN_START, NULL);
}

static void
y_density_plot_class_init (YDensityPlotClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  object_class->set_property = y_density_plot_set_property;
  object_class->get_property = y_density_plot_get_property;
  object_class->finalize = y_density_plot_finalize;

  /* properties */

  g_object_class_install_property (object_class, DENSITY_PLOT_DATA,
				   g_param_spec_object ("data", "Data",
							"data", Y_TYPE_MATRIX,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_PLOT_XMIN,
				   g_param_spec_double ("xmin",
							"Minimum X value",
							"Minimum value of X axis",
							-HUGE_VAL, HUGE_VAL,
							0.0,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_PLOT_DX,
				   g_param_spec_double ("dx", "X resolution",
							"step size, X axis",
							-HUGE_VAL, HUGE_VAL,
							1.0,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_PLOT_YMIN,
				   g_param_spec_double ("ymin",
							"Minimum Y value",
							"Minimum value of Y axis",
							-HUGE_VAL, HUGE_VAL,
							0.0,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_PLOT_DY,
				   g_param_spec_double ("dy", "Y resolution",
							"step size, Y axis",
							-HUGE_VAL, HUGE_VAL,
							1.0,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_PLOT_ZMAX,
				   g_param_spec_double ("zmax",
							"Maximum Z value",
							"maximum abs(Z)", 0,
							HUGE_VAL, 1.0,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_PLOT_AUTO_Z,
				   g_param_spec_boolean ("auto-z",
							 "Automatic Z max setting",
							 "", TRUE,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_PLOT_DRAW_LINE,
				   g_param_spec_boolean ("draw-line",
							 "Draw Line",
							 "Whether to draw line",
							 FALSE,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_PLOT_LINE_DIR,
				   g_param_spec_int ("line-dir",
						     "Line direction",
						     "Line direction",
						     GTK_ORIENTATION_HORIZONTAL,
						     GTK_ORIENTATION_VERTICAL,
						     GTK_ORIENTATION_HORIZONTAL,
						     G_PARAM_READWRITE |
						     G_PARAM_CONSTRUCT |
						     G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_PLOT_LINE_POS,
				   g_param_spec_double ("line-pos",
							"Line position",
							"Line position (center)",
							-1e16, 1e16, 0.0,
							G_PARAM_READWRITE |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_PLOT_LINE_WIDTH,
				   g_param_spec_double ("line-width",
							"Line width",
							"Line width", -1e16,
							1e16, 1.0,
							G_PARAM_READWRITE |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_PLOT_DRAW_DOT,
				   g_param_spec_boolean ("draw-dot",
							 "Draw Dot",
							 "Whether to draw a dot",
							 FALSE,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_PLOT_PRESERVE_ASPECT,
				   g_param_spec_boolean ("preserve-aspect",
							 "Preserve Aspect Ratio of data",
							 "Preserve aspect ratio of data",
							 TRUE,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  YElementViewClass *view_class = Y_ELEMENT_VIEW_CLASS (klass);
  YElementViewCartesianClass *cart_class =
    Y_ELEMENT_VIEW_CARTESIAN_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  view_class->changed = changed;

  cart_class->preferred_range = preferred_range;

  widget_class->configure_event = y_density_plot_configure_event;
  widget_class->draw = y_density_plot_draw;

  widget_class->scroll_event = y_density_plot_scroll_event;
  widget_class->button_press_event = y_density_plot_button_press_event;

  widget_class->get_request_mode = get_request_mode;
  widget_class->get_preferred_width = get_preferred_width;
  widget_class->get_preferred_height = get_preferred_height;
  widget_class->get_preferred_height_for_width =
    get_preferred_height_for_width;
  widget_class->get_preferred_width_for_height =
    get_preferred_width_for_height;
}

G_DEFINE_TYPE (YDensityPlot, y_density_plot, Y_TYPE_ELEMENT_VIEW_CARTESIAN);

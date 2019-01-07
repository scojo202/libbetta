/*
 * y-density-view.c
 *
 * Copyright (C) 2016, 2018 Scott O. Johnson (scojo202@gmail.com)
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

#include "plot/y-density-view.h"
#include <math.h>

/* TODO */
/*

Allow autoscaling symmetrically or asymetrically
Color bar - separate class, use Z view interval

*/

#define CREATE_SURF 0

enum
{
  DENSITY_VIEW_DATA = 1,
  DENSITY_VIEW_XMIN,
  DENSITY_VIEW_DX,
  DENSITY_VIEW_YMIN,
  DENSITY_VIEW_DY,
  DENSITY_VIEW_ZMAX,
  DENSITY_VIEW_AUTO_Z,
  DENSITY_VIEW_DRAW_LINE,
  DENSITY_VIEW_LINE_DIR,
  DENSITY_VIEW_LINE_POS,
  DENSITY_VIEW_LINE_WIDTH,
  DENSITY_VIEW_DRAW_DOT,
  DENSITY_VIEW_DOT_X,
  DENSITY_VIEW_DOT_Y,
  DENSITY_VIEW_PRESERVE_ASPECT,
  N_PROPERTIES
};

static GObjectClass *parent_class;

struct _YDensityView {
  YElementViewCartesian parent;

  GdkPixbuf *pixbuf, *scaled_pixbuf;

  YMatrix * tdata;
  gulong tdata_changed_id;

  GtkLabel *pos_label;		/* replace with a signal? */
  YPoint op_start;
  YPoint cursor_pos;
  gboolean zoom_in_progress;
  gboolean pan_in_progress;

  double xmin,dx;
  double ymin,dy;

  double zmax;
  gboolean auto_z;

  double scalex, scaley;
  float aspect_ratio;
  gboolean preserve_aspect;

  gboolean draw_line;
  GtkOrientation line_dir;
  double line_pos, line_width;

  gboolean draw_dot;
  double dot_pos_x, dot_pos_y;
};

static gboolean
preferred_range (YElementViewCartesian * cart, YAxisType ax, double *a,
		 double *b)
{
  YDensityView *widget = Y_DENSITY_VIEW (cart);
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
  YDensityView *widget = Y_DENSITY_VIEW (w);
  *minimum = 10;
  if (widget->tdata == NULL)
    *natural = 2000;
  else
    *natural = y_matrix_get_columns (widget->tdata);
}

static void
get_preferred_height (GtkWidget * w, gint * minimum, gint * natural)
{
  YDensityView *widget = Y_DENSITY_VIEW (w);
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
  YDensityView *widget = Y_DENSITY_VIEW (w);
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

  g_debug ("density view requesting height %d for width %d", *natural,
	   for_width);
}

static void
get_preferred_width_for_height (GtkWidget * w, gint for_height,
				gint * minimum, gint * natural)
{
  YDensityView *widget = Y_DENSITY_VIEW (w);
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

  g_debug ("density view requesting width %d for height %d, minimum %d",
           *natural, for_height, *minimum);
}

/* called when data size changes */

static void
y_density_view_update_surface (YDensityView * widget)
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
y_density_view_rescale (YDensityView * widget)
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

  YDensityView *widget = Y_DENSITY_VIEW (user_data);

  if (widget->tdata == NULL)
    {
      return;
    }

  const double *data = y_matrix_get_values (widget->tdata);
  YMatrixSize size = y_matrix_get_size (widget->tdata);

  size_t nrow = size.rows;
  size_t ncol = size.columns;

  y_density_view_update_surface (widget);

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
y_density_view_do_popup_menu (GtkWidget * my_widget, GdkEventButton * event)
{
  YElementViewCartesian *view = Y_ELEMENT_VIEW_CARTESIAN (my_widget);

  GtkWidget *menu;

  menu = gtk_menu_new ();

  GtkWidget *autoscale_x =
    _y_create_autoscale_menu_check_item (view, X_AXIS,"Autoscale X axis");
  gtk_widget_show (autoscale_x);
  GtkWidget *autoscale_y =
    _y_create_autoscale_menu_check_item (view, Y_AXIS, "Autoscale Y axis");
  gtk_widget_show (autoscale_y);

  gtk_menu_shell_append (GTK_MENU_SHELL (menu), autoscale_x);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), autoscale_y);

  gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}

static gboolean
y_density_view_scroll_event (GtkWidget * widget, GdkEventScroll * event)
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
					y_view_interval_unconv (vix, ip.x),
					scale);
  y_view_interval_rescale_around_point (viy,
					y_view_interval_unconv (viy, ip.y),
					scale);

  return FALSE;
}

static gboolean
y_density_view_button_press_event (GtkWidget * widget, GdkEventButton * event)
{
  YElementViewCartesian *view = Y_ELEMENT_VIEW_CARTESIAN (widget);
  YDensityView *dens_view = Y_DENSITY_VIEW (widget);

  /* Ignore double-clicks and triple-clicks */
  if (gdk_event_triggers_context_menu ((GdkEvent *) event) &&
      event->type == GDK_BUTTON_PRESS)
    {
      y_density_view_do_popup_menu (widget, event);
      return TRUE;
    }

    if (y_element_view_get_zooming (Y_ELEMENT_VIEW (view))
        && event->button == 1)
      {
        YViewInterval *viy = y_element_view_cartesian_get_view_interval (view,
  								       Y_AXIS);
        YViewInterval *vix = y_element_view_cartesian_get_view_interval (view,
  								       X_AXIS);

        YPoint ip;
        YPoint *evp = (YPoint *) & (event->x);

        _view_invconv (widget, evp, &ip);

        dens_view->op_start.x = y_view_interval_unconv (vix, ip.x);
        dens_view->op_start.y = y_view_interval_unconv (viy, ip.y);
        dens_view->zoom_in_progress = TRUE;
      }
    else if (event->button == 1 && (event->state & GDK_SHIFT_MASK) && y_element_view_get_panning (Y_ELEMENT_VIEW (view)))
    {
      YViewInterval *viy = y_element_view_cartesian_get_view_interval (view,
								       Y_AXIS);
      YViewInterval *vix = y_element_view_cartesian_get_view_interval (view,
								       X_AXIS);

      YPoint ip;
      YPoint *evp = (YPoint *) & (event->x);

      _view_invconv (widget, evp, &ip);

      y_view_interval_set_ignore_preferred_range (vix, TRUE);
      y_view_interval_set_ignore_preferred_range (viy, TRUE);

      y_view_interval_recenter_around_point (vix,
					     y_view_interval_unconv (vix,
									ip.
									x));
      y_view_interval_recenter_around_point (viy,
					     y_view_interval_unconv (viy,
									ip.
									y));
    }
    else if (y_element_view_get_panning (Y_ELEMENT_VIEW (view))
  		   && event->button == 1)
      {
        YViewInterval *vix =
  	        y_element_view_cartesian_get_view_interval ((YElementViewCartesian *)
  							    view,
  							    X_AXIS);

        YViewInterval *viy =
            y_element_view_cartesian_get_view_interval ((YElementViewCartesian *)
                    view,
  									Y_AXIS);

        y_view_interval_set_ignore_preferred_range (vix, TRUE);
        y_view_interval_set_ignore_preferred_range (viy, TRUE);

        YPoint ip;
        YPoint *evp = (YPoint *) & (event->x);

        _view_invconv (widget, evp, &ip);

        dens_view->op_start.x = y_view_interval_unconv (vix, ip.x);
        dens_view->op_start.y = y_view_interval_unconv (viy, ip.y);

        /* this is the position where the pan started */

        dens_view->pan_in_progress = TRUE;
      }

  return FALSE;
}

static gboolean
y_density_view_motion_notify_event (GtkWidget * widget,
					 GdkEventMotion * event)
{
  YElementViewCartesian *view = Y_ELEMENT_VIEW_CARTESIAN (widget);
  YDensityView *dens_view = Y_DENSITY_VIEW (widget);

  if (dens_view->zoom_in_progress)
    {
      YViewInterval *viy = y_element_view_cartesian_get_view_interval (view,
								       Y_AXIS);
      YViewInterval *vix = y_element_view_cartesian_get_view_interval (view,
								       X_AXIS);

      YPoint ip;
      YPoint *evp = (YPoint *) & (event->x);

      _view_invconv (widget, evp, &ip);

      YPoint pos;
      pos.x = y_view_interval_unconv (vix, ip.x);
      pos.y = y_view_interval_unconv (viy, ip.y);

      if (pos.x != dens_view->cursor_pos.x
        && pos.y != dens_view->cursor_pos.y)
        {
          dens_view->cursor_pos = pos;
          gtk_widget_queue_draw (widget);	/* for the zoom box */
        }
    }
    else if (dens_view->pan_in_progress)
      {
        YViewInterval *vix =
          y_element_view_cartesian_get_view_interval ((YElementViewCartesian *)
  						    view,
  						    X_AXIS);
        YViewInterval *viy =
                    y_element_view_cartesian_get_view_interval ((YElementViewCartesian *)
                    view,
                    Y_AXIS);
        YPoint ip;
        YPoint *evp = (YPoint *) & (event->x);

        _view_invconv (widget, evp, &ip);

        /* Calculate the translation required to put the cursor at the
         * start position. */

        double vx = y_view_interval_unconv (vix, ip.x);
        double dvx = vx - dens_view->op_start.x;

        double vy = y_view_interval_unconv (viy, ip.y);
        double dvy = vy - dens_view->op_start.y;

        y_view_interval_translate (vix, -dvx);
        y_view_interval_translate (viy, -dvy);
      }

  if (dens_view->pos_label)
    {
      YViewInterval *viy = y_element_view_cartesian_get_view_interval (view,
								       Y_AXIS);
      YViewInterval *vix = y_element_view_cartesian_get_view_interval (view,
								       X_AXIS);

      YPoint ip;
      YPoint *evp = (YPoint *) & (event->x);

      _view_invconv (widget, evp, &ip);

      double x = y_view_interval_unconv (vix, ip.x);
      double y = y_view_interval_unconv (viy, ip.y);

      gchar buffer[64];
      sprintf (buffer, "(%1.2e,%1.2e)", x, y);
      gtk_label_set_text (dens_view->pos_label, buffer);
    }

  return FALSE;
}

static gboolean
y_density_view_button_release_event (GtkWidget * widget,
					  GdkEventButton * event)
{
  YElementViewCartesian *view = Y_ELEMENT_VIEW_CARTESIAN (widget);
  YDensityView *dens_view = Y_DENSITY_VIEW (widget);

  if (dens_view->zoom_in_progress)
    {
      YViewInterval *viy = y_element_view_cartesian_get_view_interval (view,
								       Y_AXIS);
      YViewInterval *vix = y_element_view_cartesian_get_view_interval (view,
								       X_AXIS);

      YPoint ip;
      YPoint *evp = (YPoint *) & (event->x);

      _view_invconv (widget, evp, &ip);
      YPoint zoom_end;
      zoom_end.x = y_view_interval_unconv (vix, ip.x);
      zoom_end.y = y_view_interval_unconv (viy, ip.y);
      y_view_interval_set_ignore_preferred_range (vix, TRUE);
      y_view_interval_set_ignore_preferred_range (viy, TRUE);
      y_element_view_freeze (Y_ELEMENT_VIEW (widget));
      if (dens_view->op_start.x != zoom_end.x
        || dens_view->op_start.y != zoom_end.y)
        {
          y_view_interval_set (vix, dens_view->op_start.x, zoom_end.x);
          y_view_interval_set (viy, dens_view->op_start.y, zoom_end.y);
        }
      else
      {
        if (event->state & GDK_MOD1_MASK)
        {
          y_view_interval_rescale_around_point (vix, zoom_end.x,
						    1.0 / 0.8);
          y_view_interval_rescale_around_point (viy, zoom_end.y,
						    1.0 / 0.8);
        }
        else
        {
          y_view_interval_rescale_around_point (vix, zoom_end.x, 0.8);
          y_view_interval_rescale_around_point (viy, zoom_end.y, 0.8);
        }
      }
      y_element_view_thaw (Y_ELEMENT_VIEW (widget));

      dens_view->zoom_in_progress = FALSE;
    }
    else if (dens_view->pan_in_progress)
      {
        dens_view->pan_in_progress = FALSE;
      }
  return FALSE;
}

static gboolean
y_density_view_draw (GtkWidget * w, cairo_t * cr)
{
  YDensityView *widget = Y_DENSITY_VIEW (w);

  if (widget->pixbuf == NULL || widget->tdata == NULL)
    {
      g_debug ("density view draw2: %d %d", widget->scaled_pixbuf == NULL,
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

  /*g_debug ("density view using %d by %d", used_width, used_height);
  cairo_move_to(cr, 0,0);
  cairo_line_to(cr,0,used_height);
  cairo_line_to(cr,used_width,used_height);
  cairo_line_to(cr,used_width,0);
  cairo_line_to(cr,0,0);
  cairo_stroke(cr);*/

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
      double t0, t1;
      y_view_interval_range(vix,&t0,&t1);
			double dx2 = (t1 - t0) / ncol;
      double xmin = t0;

      scalex = widget->scalex * widget->dx / dx2;
      offsetx = (widget->xmin - xmin) / widget->dx * scalex;
    }
  YViewInterval *viy =
    y_element_view_cartesian_get_view_interval (Y_ELEMENT_VIEW_CARTESIAN
						(widget), Y_AXIS);

  double wxmax = widget->xmin + widget->dx * ncol;
  double wymax = widget->ymin + widget->dy * nrow;

  if (viy != NULL)
    {
      double t0, t1;
      y_view_interval_range(viy,&t0,&t1);
      double dy2 = (t1 - t0) / nrow;
      double ymax = t1;

      scaley = widget->scaley * widget->dy / dy2;
      offsety = -(wymax - ymax) / widget->dy * scaley;
    }

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
                    0, 0, used_width, used_height,
		    offsetx, offsety, scalex, scaley, GDK_INTERP_TILES);

  /* clip to size of matrix */

  /* need to handle case when vix, viy are NULL? */
  g_assert(vix);
  g_assert(viy);
  YPoint c1, c2;
  c1.x = y_view_interval_conv(vix,widget->xmin);
  c1.y = y_view_interval_conv(viy,widget->ymin);
  c2.x = y_view_interval_conv(vix,wxmax);
  c2.y = y_view_interval_conv(viy,wymax);
  _view_conv (w, &c1, &c1);
  _view_conv (w, &c2, &c2);
  cairo_move_to(cr,c1.x,c1.y);
  cairo_line_to(cr,c1.x,c2.y);
  cairo_line_to(cr,c2.x,c2.y);
  cairo_line_to(cr,c2.x,c1.y);
  cairo_line_to(cr,c1.x,c1.y);
  cairo_clip(cr);

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
          p1a.y = y_view_interval_conv (viy, p1a.y);
          p2a.y = y_view_interval_conv (viy, p2a.y);
          p1b.y = y_view_interval_conv (viy, p1b.y);
          p2b.y = y_view_interval_conv (viy, p2b.y);
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
          p1a.x = y_view_interval_conv (vix, p1a.x);
          p2a.x = y_view_interval_conv (vix, p2a.x);
          p1b.x = y_view_interval_conv (vix, p1b.x);
          p2b.x = y_view_interval_conv (vix, p2b.x);
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
        ccx = y_view_interval_conv (vix, widget->dot_pos_x);
        ccy = y_view_interval_conv (viy, widget->dot_pos_y);
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

    if (widget->zoom_in_progress)
      {
        YViewInterval *vi_x =
          y_element_view_cartesian_get_view_interval (Y_ELEMENT_VIEW_CARTESIAN
  						    (w),
  						    X_AXIS);

        YViewInterval *vi_y =
          y_element_view_cartesian_get_view_interval (Y_ELEMENT_VIEW_CARTESIAN
  						    (w),
  						    Y_AXIS);

        YPoint pstart, pend;

        pstart.x = y_view_interval_conv (vi_x, widget->op_start.x);
        pend.x = y_view_interval_conv (vi_x, widget->cursor_pos.x);
        pstart.y = y_view_interval_conv (vi_y, widget->op_start.y);
        pend.y = y_view_interval_conv (vi_y, widget->cursor_pos.y);

        _view_conv (w, &pstart, &pstart);
        _view_conv (w, &pend, &pend);

        cairo_set_source_rgba (cr, 0.0, 0.0, 1.0, 0.25);

        cairo_move_to (cr, pstart.x, pstart.y);
        cairo_line_to (cr, pstart.x, pend.y);
        cairo_line_to (cr, pend.x, pend.y);
        cairo_line_to (cr, pend.x, pstart.y);
        cairo_line_to (cr, pstart.x, pstart.y);

        cairo_fill (cr);
      }

  return FALSE;
}

static gboolean
y_density_view_configure_event (GtkWidget * w, GdkEventConfigure * ev)
{
  YDensityView *widget = Y_DENSITY_VIEW (w);

  int width, height;
  width = ev->width;
  height = ev->height;

  if (width < 1 || height < 1)
    return FALSE;

  if (widget->tdata == NULL)
    return FALSE;

  /* set scale from new widget size */

  y_density_view_rescale (widget);

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
y_density_view_finalize (GObject * obj)
{
  YDensityView *self = (YDensityView *) obj;
  if (self->tdata != NULL)
    {
      g_signal_handler_disconnect (self->tdata, self->tdata_changed_id);
      g_object_unref (self->tdata);
    }

  if (parent_class->finalize)
    parent_class->finalize (obj);
}

static void
y_density_view_set_property (GObject * object,
			     guint property_id,
			     const GValue * value, GParamSpec * pspec)
{
  YDensityView *self = (YDensityView *) object;

  g_debug ("set_property: %d", property_id);

  switch (property_id)
    {
    case DENSITY_VIEW_DATA:
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
          y_density_view_update_surface (self);
          /* connect to changed signal */
          self->tdata_changed_id =
          g_signal_connect_after (self->tdata, "changed",
                                  G_CALLBACK (on_data_changed), self);
        }
        break;
      }
    case DENSITY_VIEW_XMIN:
      {
        self->xmin = g_value_get_double (value);
      }
      break;
    case DENSITY_VIEW_DX:
      {
        self->dx = g_value_get_double (value);
      }
      break;
    case DENSITY_VIEW_YMIN:
      {
        self->ymin = g_value_get_double (value);
      }
      break;
    case DENSITY_VIEW_DY:
      {
        self->dy = g_value_get_double (value);
      }
      break;
    case DENSITY_VIEW_ZMAX:
      {
        self->zmax = g_value_get_double (value);
        if (self->tdata)
          y_data_emit_changed (Y_DATA (self->tdata));
      }
      break;
    case DENSITY_VIEW_AUTO_Z:
      {
        self->auto_z = g_value_get_boolean (value);
      }
      break;
    case DENSITY_VIEW_DRAW_LINE:
      {
        self->draw_line = g_value_get_boolean (value);
      }
      break;
    case DENSITY_VIEW_LINE_DIR:
      {
        self->line_dir = g_value_get_int (value);
      }
      break;
    case DENSITY_VIEW_LINE_POS:
      {
        self->line_pos = g_value_get_double (value);
      }
      break;
    case DENSITY_VIEW_LINE_WIDTH:
      {
        self->line_width = g_value_get_double (value);
      }
      break;
    case DENSITY_VIEW_DRAW_DOT:
      {
        self->draw_dot = g_value_get_boolean (value);
      }
      break;
    case DENSITY_VIEW_DOT_X:
        {
          self->dot_pos_x = g_value_get_double (value);
        }
      break;
    case DENSITY_VIEW_DOT_Y:
        {
          self->dot_pos_y = g_value_get_double (value);
        }
      break;
    case DENSITY_VIEW_PRESERVE_ASPECT:
      {
        self->preserve_aspect = g_value_get_boolean (value);
      }
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
  y_element_view_changed(Y_ELEMENT_VIEW(self));
}

static void
y_density_view_get_property (GObject * object,
			     guint property_id,
			     GValue * value, GParamSpec * pspec)
{
  YDensityView *self = (YDensityView *) object;
  switch (property_id)
    {
    case DENSITY_VIEW_DATA:
      {
        g_value_set_object (value, self->tdata);
      }
      break;
    case DENSITY_VIEW_XMIN:
      {
        g_value_set_double (value, self->xmin);
      }
      break;
    case DENSITY_VIEW_DX:
      {
        g_value_set_double (value, self->dx);
      }
      break;
    case DENSITY_VIEW_YMIN:
      {
        g_value_set_double (value, self->ymin);
      }
      break;
    case DENSITY_VIEW_DY:
      {
        g_value_set_double (value, self->dy);
      }
      break;
    case DENSITY_VIEW_ZMAX:
      {
        g_value_set_double (value, self->zmax);
      }
      break;
    case DENSITY_VIEW_AUTO_Z:
      {
        g_value_set_boolean (value, self->auto_z);
      }
      break;
    case DENSITY_VIEW_DRAW_LINE:
      {
        g_value_set_boolean (value, self->draw_line);
      }
      break;
    case DENSITY_VIEW_LINE_DIR:
      {
        g_value_set_int (value, self->line_dir);
      }
      break;
    case DENSITY_VIEW_LINE_POS:
      {
        g_value_set_double (value, self->line_pos);
      }
      break;
    case DENSITY_VIEW_LINE_WIDTH:
      {
        g_value_set_double (value, self->line_width);
      }
      break;
    case DENSITY_VIEW_DRAW_DOT:
      {
        g_value_set_boolean (value, self->draw_dot);
      }
      break;
    case DENSITY_VIEW_DOT_X:
        {
          g_value_set_double (value, self->dot_pos_x);
        }
      break;
    case DENSITY_VIEW_DOT_Y:
        {
          g_value_set_double (value, self->dot_pos_y);
        }
      break;
    case DENSITY_VIEW_PRESERVE_ASPECT:
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
y_density_view_init (YDensityView * view)
{
  view->tdata = NULL;

  view->aspect_ratio = 0;

  view->scaled_pixbuf = NULL;
  view->pixbuf = NULL;

  gtk_widget_add_events (GTK_WIDGET (view),
                         GDK_SCROLL_MASK | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);

  g_object_set (view, "expand", FALSE, "valign", GTK_ALIGN_START, "halign",
                GTK_ALIGN_START, NULL);

  y_element_view_cartesian_add_view_interval (Y_ELEMENT_VIEW_CARTESIAN (view),
                				      X_AXIS);
  y_element_view_cartesian_add_view_interval (Y_ELEMENT_VIEW_CARTESIAN (view),
                				      Y_AXIS);
}

static void
y_density_view_class_init (YDensityViewClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  object_class->set_property = y_density_view_set_property;
  object_class->get_property = y_density_view_get_property;
  object_class->finalize = y_density_view_finalize;

  /* properties */

  g_object_class_install_property (object_class, DENSITY_VIEW_DATA,
				   g_param_spec_object ("data", "Data",
							"data", Y_TYPE_MATRIX,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_VIEW_XMIN,
				   g_param_spec_double ("xmin",
							"Minimum X value",
							"Minimum value of X axis",
							-HUGE_VAL, HUGE_VAL,
							0.0,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_VIEW_DX,
				   g_param_spec_double ("dx", "X resolution",
							"step size, X axis",
							-HUGE_VAL, HUGE_VAL,
							1.0,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_VIEW_YMIN,
				   g_param_spec_double ("ymin",
							"Minimum Y value",
							"Minimum value of Y axis",
							-HUGE_VAL, HUGE_VAL,
							0.0,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_VIEW_DY,
				   g_param_spec_double ("dy", "Y resolution",
							"step size, Y axis",
							-HUGE_VAL, HUGE_VAL,
							1.0,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_VIEW_ZMAX,
				   g_param_spec_double ("zmax",
							"Maximum Z value",
							"maximum abs(Z)", 0,
							HUGE_VAL, 1.0,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_VIEW_AUTO_Z,
				   g_param_spec_boolean ("auto-z",
							 "Automatic Z max setting",
							 "", TRUE,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_VIEW_DRAW_LINE,
				   g_param_spec_boolean ("draw-line",
							 "Draw Line",
							 "Whether to draw line",
							 FALSE,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_VIEW_LINE_DIR,
				   g_param_spec_int ("line-dir",
						     "Line direction",
						     "Line direction",
						     GTK_ORIENTATION_HORIZONTAL,
						     GTK_ORIENTATION_VERTICAL,
						     GTK_ORIENTATION_HORIZONTAL,
						     G_PARAM_READWRITE |
						     G_PARAM_CONSTRUCT |
						     G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_VIEW_LINE_POS,
				   g_param_spec_double ("line-pos",
							"Line position",
							"Line position (center)",
							-1e16, 1e16, 0.0,
							G_PARAM_READWRITE |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_VIEW_LINE_WIDTH,
				   g_param_spec_double ("line-width",
							"Line width",
							"Line width", -1e16,
							1e16, 1.0,
							G_PARAM_READWRITE |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_VIEW_DRAW_DOT,
				   g_param_spec_boolean ("draw-dot",
							 "Draw Dot",
							 "Whether to draw a dot",
							 FALSE,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_VIEW_DOT_X,
				   g_param_spec_double ("dot-pos-x",
							"Dot x coordinate",
							"X position of dot", -1e16,
							1e16, 0.0,
							G_PARAM_READWRITE |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_VIEW_DOT_Y,
				   g_param_spec_double ("dot-pos-y",
							"Dot y coordinate",
							"Y position of dot", -1e16,
							1e16, 0.0,
							G_PARAM_READWRITE |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, DENSITY_VIEW_PRESERVE_ASPECT,
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

  widget_class->configure_event = y_density_view_configure_event;
  widget_class->draw = y_density_view_draw;

  widget_class->scroll_event = y_density_view_scroll_event;
  widget_class->button_press_event = y_density_view_button_press_event;
  widget_class->motion_notify_event = y_density_view_motion_notify_event;
  widget_class->button_release_event =
    y_density_view_button_release_event;

  widget_class->get_request_mode = get_request_mode;
  widget_class->get_preferred_width = get_preferred_width;
  widget_class->get_preferred_height = get_preferred_height;
  widget_class->get_preferred_height_for_width =
    get_preferred_height_for_width;
  widget_class->get_preferred_width_for_height =
    get_preferred_width_for_height;
}

G_DEFINE_TYPE (YDensityView, y_density_view, Y_TYPE_ELEMENT_VIEW_CARTESIAN);

void y_density_view_set_pos_label(YDensityView *v, GtkLabel *pos_label)
{
  v->pos_label = g_object_ref (pos_label);
}

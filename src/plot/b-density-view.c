/*
 * b-density-view.c
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

#include "plot/b-density-view.h"
#include "plot/b-color-map.h"
#include <math.h>

/* TODO */
/*

Allow autoscaling symmetrically or asymetrically

*/

/**
 * SECTION: b-density-view
 * @short_description: View for a density plot.
 *
 * Displays a color image showing the value of a matrix as a function of its
 * two axes.
 *
 * To get the horizontal axis view interval and markers use axis type X_AXIS,
 * and for the vertical axis use Y_AXIS. The axis type to use to get the density
 * axis is Z_AXIS.
 */

#define CREATE_SURF 0

enum
{
  DENSITY_VIEW_DATA = 1,
  DENSITY_VIEW_XMIN,
  DENSITY_VIEW_DX,
  DENSITY_VIEW_YMIN,
  DENSITY_VIEW_DY,
  DENSITY_VIEW_SYM_Z,
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

struct _BDensityView {
  BElementViewCartesian parent;

  GdkPixbuf *pixbuf, *scaled_pixbuf;

  BColorMap *map;

  BMatrix * tdata;
  gulong tdata_changed_id;

  BPoint op_start;
  BPoint cursor_pos;
  gboolean zoom_in_progress;
  gboolean pan_in_progress;

  double xmin,dx;
  double ymin,dy;

  gboolean sym_z;

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
preferred_range (BElementViewCartesian * cart, BAxisType ax, double *a,
		 double *b)
{
  BDensityView *widget = B_DENSITY_VIEW (cart);
  *a = 0;
  *b = 1;
  if (widget->tdata != NULL)
    {
      BMatrixSize size = b_matrix_get_size (widget->tdata);
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
      else if (ax == Z_AXIS)
      {
        b_matrix_get_minmax(widget->tdata, a, b);
        if(widget->sym_z) {
          double mx = MAX(fabs(*a),fabs(*b));
          *a = -mx;
          *b = mx;
        }
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
  BDensityView *widget = B_DENSITY_VIEW (w);
  *minimum = 10;
  if (widget->tdata == NULL)
    *natural = 2000;
  else
    *natural = b_matrix_get_columns (widget->tdata);
}

static void
get_preferred_height (GtkWidget * w, gint * minimum, gint * natural)
{
  BDensityView *widget = B_DENSITY_VIEW (w);
  *minimum = 10;
  if (widget->tdata == NULL)
    *natural = 2000;
  else
    *natural = b_matrix_get_rows (widget->tdata);
}

static void
get_preferred_height_for_width (GtkWidget * w, gint for_width, gint * minimum,
				gint * natural)
{
  BDensityView *widget = B_DENSITY_VIEW (w);
  if (widget->aspect_ratio > 0)
    {
      *natural = ((float) for_width) / widget->aspect_ratio;
    }
  else
    {
      if (widget->tdata == NULL)
        *natural = 200;
      else
        *natural = b_matrix_get_rows (widget->tdata);
    }
  *minimum = (*natural >= 50 ? 50 : *natural);

  g_debug ("density view requesting height %d for width %d", *natural,
	   for_width);
}

static void
get_preferred_width_for_height (GtkWidget * w, gint for_height,
				gint * minimum, gint * natural)
{
  BDensityView *widget = B_DENSITY_VIEW (w);
  if (widget->aspect_ratio > 0)
    {
      *natural = ((float) for_height) * widget->aspect_ratio;
    }
  else
    {
      if (widget->tdata == NULL)
        *natural = 200;
      else
        *natural = b_matrix_get_columns (widget->tdata);
    }
  *minimum = (*natural >= 50 ? 50 : *natural);

  g_debug ("density view requesting width %d for height %d, minimum %d",
           *natural, for_height, *minimum);
}

/* called when data size changes */

static void
b_density_view_update_surface (BDensityView * widget)
{
  int width = 0;
  int height = 0;

  if (widget->pixbuf != NULL)
    {
      g_object_get (widget->pixbuf, "height", &height, "width", &width, NULL);
    }

  BMatrixSize size = b_matrix_get_size (widget->tdata);

  if (size.rows == height || size.columns == width)
    return;

  g_debug ("update_surface: %d -> %d, %d -> %d", size.rows, height,
	   size.columns, width);

  if (widget->pixbuf != NULL)
    {
      g_clear_object (&widget->pixbuf);
    }

  widget->pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, size.columns,
                                   size.rows);
}

static void
b_density_view_rescale (BDensityView * widget)
{
  int width, height;
  width = gtk_widget_get_allocated_width (GTK_WIDGET (widget));
  height = gtk_widget_get_allocated_height (GTK_WIDGET (widget));

  BMatrixSize size = b_matrix_get_size (widget->tdata);

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

static void
redraw_surface(BDensityView *widget)
{
  int i, j;

  if(widget->tdata==NULL)
    return;

  const double *data = b_matrix_get_values (widget->tdata);
  BMatrixSize size = b_matrix_get_size (widget->tdata);

  size_t nrow = size.rows;
  size_t ncol = size.columns;

  b_density_view_update_surface (widget);

  //GTimer *t = g_timer_new();

  double mn, mx;
  BViewInterval *viz = b_element_view_cartesian_get_view_interval(B_ELEMENT_VIEW_CARTESIAN(widget),Z_AXIS);
  b_view_interval_range(viz,&mn,&mx);

  int n_channels = gdk_pixbuf_get_n_channels (widget->pixbuf);
  g_return_if_fail(n_channels != 4);
  int rowstride = gdk_pixbuf_get_rowstride (widget->pixbuf);
  guchar *pixels = gdk_pixbuf_get_pixels (widget->pixbuf);
  //guint32 *mpixels = (guint32*) pixels;

  unsigned char lut[256 * 4];
  guint32 *mlut = (guint32 *) lut;

  double dl = 1.0 / 256.0;
  for (i = 0; i < 256; i++)
    {
      mlut[i] = b_color_map_get_map(widget->map,i*dl);
      //g_message("%d: %d %d %d %d",i,lut[4*i],lut[4*i+1],lut[4*i+2],lut[4*i+3]);
    }

  for (i = 0; i < nrow; i++)
    {
      for (j = 0; j < ncol; j++)
      {
        if (isnan (data[i * ncol + j]))
        {
          //mpixels[j+(nrow-1-i)*rowstride/4] = 0;
          pixels[n_channels * j + (nrow - 1 - i) * rowstride] = 0;
          pixels[n_channels * j + (nrow - 1 - i) * rowstride + 1] = 0;
          pixels[n_channels * j + (nrow - 1 - i) * rowstride + 2] = 0;
        }
        else
        {
          double ds = b_view_interval_conv(viz,data[i * ncol + j]);
          if (ds <= 0.0) {
            pixels[n_channels * j + (nrow - 1 - i) * rowstride] =
            lut[0];
            pixels[n_channels * j + (nrow - 1 - i) * rowstride + 1] =
            lut[1];
            pixels[n_channels * j + (nrow - 1 - i) * rowstride + 2] =
            lut[2];
            /*pixels[n_channels * j + (nrow - 1 - i) * rowstride + 3] =
            lut[3];*/
          }
          else if (ds>=1.0) {
            pixels[n_channels * j + (nrow - 1 - i) * rowstride] =
            lut[4*255];
            pixels[n_channels * j + (nrow - 1 - i) * rowstride + 1] =
            lut[4*255+1];
            pixels[n_channels * j + (nrow - 1 - i) * rowstride + 2] =
            lut[4*255+1];
            /*pixels[n_channels * j + (nrow - 1 - i) * rowstride + 3] =
            lut[4*255+1];*/
          }
          else {
            //g_message("%d %d %d, %d %d, %d",i,j,ss,nrow,ncol,rowstride);
            /* should be able to do better than below */
            int ss = (int) (ds * 255);
            pixels[n_channels * j + (nrow - 1 - i) * rowstride] =
            lut[4 * ss];
            pixels[n_channels * j + (nrow - 1 - i) * rowstride + 1] =
            lut[4 * ss + 1];
            pixels[n_channels * j + (nrow - 1 - i) * rowstride + 2] =
            lut[4 * ss + 2];
            /*pixels[n_channels * j + (nrow - 1 - i) * rowstride + 3] =
            lut[4 * ss + 3];*/
          }
        }
      }
    }

    if (widget->preserve_aspect)
      widget->aspect_ratio = ((float) size.columns / ((float) size.rows));
    else
      widget->aspect_ratio = -1;
}

static void
on_data_changed (BData * dat, gpointer user_data)
{
  BElementView *mev = (BElementView *) user_data;
  g_assert (mev);

  BDensityView *widget = B_DENSITY_VIEW (user_data);

  if (widget->tdata == NULL)
    {
      return;
    }

  redraw_surface(widget);

  //double te = g_timer_elapsed(t,NULL);
  //g_message("fill buffer: %f ms",te*1000);

  gtk_widget_queue_resize (GTK_WIDGET (widget));

  b_element_view_changed (mev);
}

static void
b_density_view_do_popup_menu (GtkWidget * my_widget, GdkEventButton * event)
{
  BElementViewCartesian *view = B_ELEMENT_VIEW_CARTESIAN (my_widget);

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
b_density_view_scroll_event (GtkWidget * widget, GdkEventScroll * event)
{
  BElementViewCartesian *view = B_ELEMENT_VIEW_CARTESIAN (widget);

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

  BViewInterval *viy = b_element_view_cartesian_get_view_interval (view,
								   Y_AXIS);
  BViewInterval *vix = b_element_view_cartesian_get_view_interval (view,
								   X_AXIS);

  double scale = direction ? 0.8 : 1.0 / 0.8;

  /* find the cursor position */

  BPoint ip = _view_event_point(widget,(GdkEvent *)event);
  b_view_interval_rescale_around_point (vix,
					b_view_interval_unconv (vix, ip.x),
					scale);
  b_view_interval_rescale_around_point (viy,
					b_view_interval_unconv (viy, ip.y),
					scale);

  return FALSE;
}

static gboolean
b_density_view_button_press_event (GtkWidget * widget, GdkEventButton * event)
{
  BElementViewCartesian *view = B_ELEMENT_VIEW_CARTESIAN (widget);
  BDensityView *dens_view = B_DENSITY_VIEW (widget);

  /* Ignore double-clicks and triple-clicks */
  if (gdk_event_triggers_context_menu ((GdkEvent *) event) &&
      event->type == GDK_BUTTON_PRESS)
    {
      b_density_view_do_popup_menu (widget, event);
      return TRUE;
    }

    if (b_element_view_get_zooming (B_ELEMENT_VIEW (view))
        && event->button == 1)
      {
        BViewInterval *viy = b_element_view_cartesian_get_view_interval (view,
  								       Y_AXIS);
        BViewInterval *vix = b_element_view_cartesian_get_view_interval (view,
  								       X_AXIS);

        BPoint ip = _view_event_point(widget,(GdkEvent *)event);

        dens_view->op_start.x = b_view_interval_unconv (vix, ip.x);
        dens_view->op_start.y = b_view_interval_unconv (viy, ip.y);
        dens_view->zoom_in_progress = TRUE;
      }
    else if (event->button == 1 && (event->state & GDK_SHIFT_MASK) && b_element_view_get_panning (B_ELEMENT_VIEW (view)))
    {
      BViewInterval *viy = b_element_view_cartesian_get_view_interval (view,
								       Y_AXIS);
      BViewInterval *vix = b_element_view_cartesian_get_view_interval (view,
								       X_AXIS);

      BPoint ip = _view_event_point(widget,(GdkEvent *)event);

      b_view_interval_set_ignore_preferred_range (vix, TRUE);
      b_view_interval_set_ignore_preferred_range (viy, TRUE);

      b_view_interval_recenter_around_point (vix,
					     b_view_interval_unconv (vix,
									ip.
									x));
      b_view_interval_recenter_around_point (viy,
					     b_view_interval_unconv (viy,
									ip.
									y));
    }
    else if (b_element_view_get_panning (B_ELEMENT_VIEW (view))
  		   && event->button == 1)
      {
        BViewInterval *vix =
  	        b_element_view_cartesian_get_view_interval ( view, X_AXIS);

        BViewInterval *viy =
            b_element_view_cartesian_get_view_interval ( view, Y_AXIS);

        b_view_interval_set_ignore_preferred_range (vix, TRUE);
        b_view_interval_set_ignore_preferred_range (viy, TRUE);

        BPoint ip = _view_event_point(widget,(GdkEvent *)event);

        dens_view->op_start.x = b_view_interval_unconv (vix, ip.x);
        dens_view->op_start.y = b_view_interval_unconv (viy, ip.y);

        /* this is the position where the pan started */

        dens_view->pan_in_progress = TRUE;
      }

  return FALSE;
}

static gboolean
b_density_view_motion_notify_event (GtkWidget * widget,
					 GdkEventMotion * event)
{
  BElementViewCartesian *view = B_ELEMENT_VIEW_CARTESIAN (widget);
  BDensityView *dens_view = B_DENSITY_VIEW (widget);

  g_return_val_if_fail(B_IS_MATRIX(dens_view->tdata),FALSE);

  if (dens_view->zoom_in_progress)
    {
      BViewInterval *viy = b_element_view_cartesian_get_view_interval (view,
								       Y_AXIS);
      BViewInterval *vix = b_element_view_cartesian_get_view_interval (view,
								       X_AXIS);

      BPoint ip = _view_event_point(widget,(GdkEvent *)event);

      BPoint pos;
      pos.x = b_view_interval_unconv (vix, ip.x);
      pos.y = b_view_interval_unconv (viy, ip.y);

      if (pos.x != dens_view->cursor_pos.x
        && pos.y != dens_view->cursor_pos.y)
        {
          dens_view->cursor_pos = pos;
          gtk_widget_queue_draw (widget);	/* for the zoom box */
        }
    }
    else if (dens_view->pan_in_progress)
      {
        BViewInterval *vix =
          b_element_view_cartesian_get_view_interval ((BElementViewCartesian *)
  						    view,
  						    X_AXIS);
        BViewInterval *viy =
                    b_element_view_cartesian_get_view_interval ((BElementViewCartesian *)
                    view,
                    Y_AXIS);
        BPoint ip = _view_event_point(widget,(GdkEvent *)event);

        /* Calculate the translation required to put the cursor at the
         * start position. */

        double vx = b_view_interval_unconv (vix, ip.x);
        double dvx = vx - dens_view->op_start.x;

        double vy = b_view_interval_unconv (viy, ip.y);
        double dvy = vy - dens_view->op_start.y;

        b_view_interval_translate (vix, -dvx);
        b_view_interval_translate (viy, -dvy);
      }

  if (b_element_view_get_status_label((BElementView *)dens_view))
    {
      BViewInterval *viy = b_element_view_cartesian_get_view_interval (view,
								       Y_AXIS);
      BViewInterval *vix = b_element_view_cartesian_get_view_interval (view,
								       X_AXIS);

      BPoint ip = _view_event_point(widget,(GdkEvent *)event);

      double x = b_view_interval_unconv (vix, ip.x);
      double y = b_view_interval_unconv (viy, ip.y);

      /* get index from coordinate */
      int i=(int)((x-dens_view->xmin)/dens_view->dx);
      int j=(int)((y-dens_view->ymin)/dens_view->dy);
      BMatrixSize size = b_matrix_get_size(dens_view->tdata);
      double z = NAN;
      if(i>=0 && j>=0 && i<size.columns && j<size.rows)
        z = b_matrix_get_value(dens_view->tdata,j,i);

      GString *str = g_string_new("(");
      _append_format_double_scinot(str,x);
      g_string_append(str,", ");
      _append_format_double_scinot(str,y);
      g_string_append(str,", ");
      _append_format_double_scinot(str,z);
      g_string_append(str,")");
      b_element_view_set_status (B_ELEMENT_VIEW(view), str->str);
      g_string_free(str,TRUE);
    }

  return FALSE;
}

static gboolean
b_density_view_button_release_event (GtkWidget * widget,
					  GdkEventButton * event)
{
  BElementViewCartesian *view = B_ELEMENT_VIEW_CARTESIAN (widget);
  BDensityView *dens_view = B_DENSITY_VIEW (widget);

  if (dens_view->zoom_in_progress)
    {
      BViewInterval *viy = b_element_view_cartesian_get_view_interval (view,
								       Y_AXIS);
      BViewInterval *vix = b_element_view_cartesian_get_view_interval (view,
								       X_AXIS);

      BPoint ip = _view_event_point(widget,(GdkEvent *)event);
      BPoint zoom_end;
      zoom_end.x = b_view_interval_unconv (vix, ip.x);
      zoom_end.y = b_view_interval_unconv (viy, ip.y);
      b_view_interval_set_ignore_preferred_range (vix, TRUE);
      b_view_interval_set_ignore_preferred_range (viy, TRUE);
      b_element_view_freeze (B_ELEMENT_VIEW (widget));
      if (dens_view->op_start.x != zoom_end.x
        || dens_view->op_start.y != zoom_end.y)
        {
          b_view_interval_set (vix, dens_view->op_start.x, zoom_end.x);
          b_view_interval_set (viy, dens_view->op_start.y, zoom_end.y);
        }
      else
      {
        b_rescale_around_val(vix,zoom_end.x, event);
        b_rescale_around_val(viy,zoom_end.y, event);
      }
      b_element_view_thaw (B_ELEMENT_VIEW (widget));

      dens_view->zoom_in_progress = FALSE;
    }
    else if (dens_view->pan_in_progress)
      {
        dens_view->pan_in_progress = FALSE;
      }
  return FALSE;
}

static gboolean
b_density_view_draw (GtkWidget * w, cairo_t * cr)
{
  BDensityView *widget = B_DENSITY_VIEW (w);
  BElementViewCartesian *cart = B_ELEMENT_VIEW_CARTESIAN(w);

  if (widget->pixbuf == NULL || widget->tdata == NULL)
    {
      g_debug ("density view draw2: %d %d", widget->scaled_pixbuf == NULL,
	       widget->tdata == NULL);
      return FALSE;
    }

  BMatrixSize size = b_matrix_get_size (widget->tdata);

  size_t nrow = size.rows;
  size_t ncol = size.columns;

  if (nrow == 0 || ncol == 0)
    return FALSE;

  int used_width, used_height;

  used_width = ncol * widget->scalex;
  used_height = nrow * widget->scaley;

  /*cairo_move_to(cr, 0,0);
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

  BViewInterval *vix =
    b_element_view_cartesian_get_view_interval (cart, X_AXIS);

  double wxmax = widget->xmin + widget->dx * ncol;
  double wymax = widget->ymin + widget->dy * nrow; /* other end of interval */

  if (vix != NULL)
    {
      double t0, t1;
      b_view_interval_range(vix,&t0,&t1);
      double dx2 = (t1 - t0) / ncol;
      double xmin = t0;

      scalex = widget->scalex * widget->dx / dx2;
      double wx0 = widget->xmin;
      if(widget->dx<0) {
        wx0 = wxmax;
      }
      offsetx = (wx0 - xmin) / widget->dx * scalex;
    }
  BViewInterval *viy =
    b_element_view_cartesian_get_view_interval (cart, Y_AXIS);

  if (viy != NULL)
    {
      double t0, t1;
      b_view_interval_range(viy,&t0,&t1);
      double dy2 = (t1 - t0) / nrow;
      double ymax = t1;

      scaley = widget->scaley * widget->dy / dy2;
      double wy0 = wymax;
      if(widget->dy<0) {
        wy0 = widget->ymin;
      }
      offsety = -(wy0 - ymax) / widget->dy * scaley;
    }

  //g_message("scaley is %f, offsety is %f",scaley,offsety);

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

  GdkPixbuf *buf = g_object_ref(widget->pixbuf);
  if(scalex<0) {
    scalex = fabs(scalex);
    buf = gdk_pixbuf_flip(widget->pixbuf,TRUE);
  }
  if(scaley<0) {
    scaley = fabs(scaley);
    GdkPixbuf *buf2 = gdk_pixbuf_flip(buf,FALSE);
    g_object_unref(buf);
    buf=buf2;
  }

  gdk_pixbuf_scale (buf, widget->scaled_pixbuf,
                    0, 0, used_width, used_height,
		    offsetx, offsety, scalex, scaley, GDK_INTERP_TILES);

  g_object_unref(buf);

  /* clip to size of matrix */

  /* need to handle case when vix, viy are NULL? */
  g_assert(vix);
  g_assert(viy);
  BPoint c1, c2;
  c1.x = b_view_interval_conv(vix,widget->xmin);
  c1.y = b_view_interval_conv(viy,widget->ymin);
  c2.x = b_view_interval_conv(vix,wxmax);
  c2.y = b_view_interval_conv(viy,wymax);
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

      BPoint p1a, p2a, p1b, p2b;

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
          p1a.y = b_view_interval_conv (viy, p1a.y);
          p2a.y = b_view_interval_conv (viy, p2a.y);
          p1b.y = b_view_interval_conv (viy, p1b.y);
          p2b.y = b_view_interval_conv (viy, p2b.y);
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
          p1a.x = b_view_interval_conv (vix, p1a.x);
          p2a.x = b_view_interval_conv (vix, p2a.x);
          p1b.x = b_view_interval_conv (vix, p1b.x);
          p2b.x = b_view_interval_conv (vix, p2b.x);
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
        ccx = b_view_interval_conv (vix, widget->dot_pos_x);
        ccy = b_view_interval_conv (viy, widget->dot_pos_y);
      }
      BPoint p = { ccx, ccy };
      BPoint p2;
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
        BViewInterval *vi_x =
          b_element_view_cartesian_get_view_interval (cart, X_AXIS);

        BViewInterval *vi_y =
          b_element_view_cartesian_get_view_interval (cart, Y_AXIS);

        BPoint pstart, pend;

        pstart.x = b_view_interval_conv (vi_x, widget->op_start.x);
        pend.x = b_view_interval_conv (vi_x, widget->cursor_pos.x);
        pstart.y = b_view_interval_conv (vi_y, widget->op_start.y);
        pend.y = b_view_interval_conv (vi_y, widget->cursor_pos.y);

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
b_density_view_configure_event (GtkWidget * w, GdkEventConfigure * ev)
{
  BDensityView *widget = B_DENSITY_VIEW (w);

  int width, height;
  width = ev->width;
  height = ev->height;

  if (width < 1 || height < 1)
    return FALSE;

  if (widget->tdata == NULL)
    return FALSE;

  /* set scale from new widget size */

  b_density_view_rescale (widget);

  return FALSE;
}

static void
changed (BElementView * gev)
{
  b_element_view_cartesian_set_preferred_view ((BElementViewCartesian *) gev,
					       X_AXIS);
  b_element_view_cartesian_set_preferred_view ((BElementViewCartesian *) gev,
					       Y_AXIS);
  b_element_view_cartesian_set_preferred_view ((BElementViewCartesian *) gev,
               					       Z_AXIS);

  BElementViewCartesian *cart = (BElementViewCartesian *) gev;

  BViewInterval *vix =
    b_element_view_cartesian_get_view_interval (cart, X_AXIS);
  BViewInterval *viy =
    b_element_view_cartesian_get_view_interval (cart, Y_AXIS);
  BViewInterval *viz =
    b_element_view_cartesian_get_view_interval (cart, Z_AXIS);

  if (vix)
    b_view_interval_request_preferred_range (vix);
  if (viy)
    b_view_interval_request_preferred_range (viy);
  if (viz)
    b_view_interval_request_preferred_range (viz);

  redraw_surface((BDensityView *)gev);

  if (B_ELEMENT_VIEW_CLASS (parent_class)->changed)
    B_ELEMENT_VIEW_CLASS (parent_class)->changed (gev);
}

static void
b_density_view_finalize (GObject * obj)
{
  BDensityView *self = (BDensityView *) obj;
  if (self->tdata != NULL)
    {
      g_signal_handler_disconnect (self->tdata, self->tdata_changed_id);
      g_object_unref (self->tdata);
    }

  if (parent_class->finalize)
    parent_class->finalize (obj);
}

static void
b_density_view_set_property (GObject * object,
			     guint property_id,
			     const GValue * value, GParamSpec * pspec)
{
  BDensityView *self = (BDensityView *) object;

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
          b_density_view_update_surface (self);
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
    case DENSITY_VIEW_SYM_Z:
      {
        self->sym_z = g_value_get_boolean (value);
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
  b_element_view_changed(B_ELEMENT_VIEW(self));
}

static void
b_density_view_get_property (GObject * object,
			     guint property_id,
			     GValue * value, GParamSpec * pspec)
{
  BDensityView *self = (BDensityView *) object;
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
    case DENSITY_VIEW_SYM_Z:
      {
        g_value_set_boolean (value, self->sym_z);
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
b_density_view_init (BDensityView * view)
{
  gtk_widget_add_events (GTK_WIDGET (view),
                         GDK_SCROLL_MASK | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);

  g_object_set (view, "expand", FALSE, "valign", GTK_ALIGN_START, "halign",
                GTK_ALIGN_START, NULL);

  b_element_view_cartesian_add_view_interval (B_ELEMENT_VIEW_CARTESIAN (view),
                                              X_AXIS);
  b_element_view_cartesian_add_view_interval (B_ELEMENT_VIEW_CARTESIAN (view),
                                              Y_AXIS);
  b_element_view_cartesian_add_view_interval (B_ELEMENT_VIEW_CARTESIAN (view),
                                              Z_AXIS);

  view->map = b_color_map_new();
  //b_color_map_set_transition(view->map,RGBA_BLACK,RGBA_CYAN);
  b_color_map_set_thermal(view->map);
}

static void
b_density_view_class_init (BDensityViewClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  object_class->set_property = b_density_view_set_property;
  object_class->get_property = b_density_view_get_property;
  object_class->finalize = b_density_view_finalize;

  /* properties */

  g_object_class_install_property (object_class, DENSITY_VIEW_DATA,
				   g_param_spec_object ("data", "Data",
							"data", B_TYPE_MATRIX,
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

  g_object_class_install_property (object_class, DENSITY_VIEW_SYM_Z,
				   g_param_spec_boolean ("symmetric-z",
							 "Force Z scale to always be symmetric about 0",
							 "", FALSE,
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

  BElementViewClass *view_class = B_ELEMENT_VIEW_CLASS (klass);
  BElementViewCartesianClass *cart_class =
    B_ELEMENT_VIEW_CARTESIAN_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  view_class->changed = changed;

  cart_class->preferred_range = preferred_range;

  widget_class->configure_event = b_density_view_configure_event;
  widget_class->draw = b_density_view_draw;

  widget_class->scroll_event = b_density_view_scroll_event;
  widget_class->button_press_event = b_density_view_button_press_event;
  widget_class->motion_notify_event = b_density_view_motion_notify_event;
  widget_class->button_release_event =
    b_density_view_button_release_event;

  widget_class->get_request_mode = get_request_mode;
  widget_class->get_preferred_width = get_preferred_width;
  widget_class->get_preferred_height = get_preferred_width;
  widget_class->get_preferred_height_for_width =
    get_preferred_height_for_width;
  widget_class->get_preferred_width_for_height =
    get_preferred_width_for_height;
}

G_DEFINE_TYPE (BDensityView, b_density_view, B_TYPE_ELEMENT_VIEW_CARTESIAN);

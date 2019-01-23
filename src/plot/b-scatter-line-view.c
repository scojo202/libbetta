/*
 * y-scatter-view.c
 *
 * Copyright (C) 2000 EMC Capital Management, Inc.
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

#include "plot/y-scatter-line-view.h"
#include <math.h>
#include "data/y-data-class.h"

/**
 * SECTION: y-scatter-line-view
 * @short_description: View for a scatter and/or line plot.
 *
 * Displays a line and/or scatter plot, e.g. y as a function of x.
 */

static GObjectClass *parent_class = NULL;

#define PROFILE 0

struct _YScatterLineView
{
  YElementViewCartesian base;
  GList *series;
  YPoint op_start;
  YPoint cursor_pos;
  gboolean zoom_in_progress;
  gboolean pan_in_progress;
};

G_DEFINE_TYPE (YScatterLineView, y_scatter_line_view,
	       Y_TYPE_ELEMENT_VIEW_CARTESIAN);

static void
handlers_disconnect (gpointer data, gpointer user_data)
{
  YScatterSeries *series = Y_SCATTER_SERIES (data);
  YScatterLineView *v = Y_SCATTER_LINE_VIEW (user_data);

  YData *xdata, *ydata;
  g_object_get(series, "x-data",&xdata, "y-data", &ydata, NULL);

  if (xdata != NULL)
    {
      g_signal_handlers_disconnect_by_data (xdata, v);
    }

  if (ydata != NULL)
    {
      g_signal_handlers_disconnect_by_data (ydata, v);
    }
}

static void
y_scatter_line_view_finalize (GObject * obj)
{
  YScatterLineView *v = Y_SCATTER_LINE_VIEW (obj);
  g_list_foreach (v->series, handlers_disconnect, v);

  if (parent_class->finalize)
    parent_class->finalize (obj);
}

#if 0
static gboolean
rectangle_contains_point (cairo_rectangle_t rect, YPoint * point)
{
  return ((point->x > rect.x) && (point->x < rect.x + rect.width)
	  && (point->y > rect.y) && (point->y < rect.y + rect.height));
}
#endif

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
changed (YElementView * gev)
{
  YElementViewCartesian *cart = Y_ELEMENT_VIEW_CARTESIAN (gev);

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

static gboolean
y_scatter_line_view_scroll_event (GtkWidget * widget, GdkEventScroll * event)
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

  YPoint ip = _view_event_point(widget,(GdkEvent *)event);
  y_view_interval_rescale_around_point (vix,
					y_view_interval_unconv (vix, ip.x),
					scale);
  y_view_interval_rescale_around_point (viy,
					y_view_interval_unconv (viy, ip.y),
					scale);

  return FALSE;
}

static void
do_popup_menu (GtkWidget * my_widget, GdkEventButton * event)
{
  YElementViewCartesian *view = Y_ELEMENT_VIEW_CARTESIAN (my_widget);

  GtkWidget *menu;

  menu = gtk_menu_new ();

  GtkWidget *autoscale_x =
    _y_create_autoscale_menu_check_item (view, X_AXIS, "Autoscale X axis");
  gtk_widget_show (autoscale_x);
  GtkWidget *autoscale_y =
    _y_create_autoscale_menu_check_item (view, Y_AXIS, "Autoscale Y axis");
  gtk_widget_show (autoscale_y);

  gtk_menu_shell_append (GTK_MENU_SHELL (menu), autoscale_x);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), autoscale_y);

  gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}

static gboolean
y_scatter_line_view_motion_notify_event (GtkWidget * widget,
					 GdkEventMotion * event)
{
  YElementViewCartesian *view = Y_ELEMENT_VIEW_CARTESIAN (widget);
  YScatterLineView *line_view = Y_SCATTER_LINE_VIEW (widget);

  if (line_view->zoom_in_progress)
    {
      YViewInterval *viy = y_element_view_cartesian_get_view_interval (view,
								       Y_AXIS);
      YViewInterval *vix = y_element_view_cartesian_get_view_interval (view,
								       X_AXIS);

      YPoint ip = _view_event_point(widget,(GdkEvent *)event);

      YPoint pos;
      pos.x = y_view_interval_unconv (vix, ip.x);
      pos.y = y_view_interval_unconv (viy, ip.y);

      if (pos.x != line_view->cursor_pos.x
        && pos.y != line_view->cursor_pos.y)
        {
          line_view->cursor_pos = pos;
          gtk_widget_queue_draw (widget);	/* for the zoom box */
        }
    }
    else if (line_view->pan_in_progress)
      {
        YViewInterval *vix =
          y_element_view_cartesian_get_view_interval ((YElementViewCartesian *)
  						    view,
  						    X_AXIS);
        YViewInterval *viy =
                    y_element_view_cartesian_get_view_interval ((YElementViewCartesian *)
                    view,
                    Y_AXIS);
        YPoint ip = _view_event_point(widget,(GdkEvent *)event);

        /* Calculate the translation required to put the cursor at the
         * start position. */

        double vx = y_view_interval_unconv (vix, ip.x);
        double dvx = vx - line_view->op_start.x;

        double vy = y_view_interval_unconv (viy, ip.y);
        double dvy = vy - line_view->op_start.y;

        y_view_interval_translate (vix, -dvx);
        y_view_interval_translate (viy, -dvy);
      }

  if (y_element_view_get_status_label(Y_ELEMENT_VIEW(view)))
    {
      YViewInterval *viy = y_element_view_cartesian_get_view_interval (view,
								       Y_AXIS);
      YViewInterval *vix = y_element_view_cartesian_get_view_interval (view,
								       X_AXIS);

      YPoint ip = _view_event_point(widget,(GdkEvent *)event);

      double x = y_view_interval_unconv (vix, ip.x);
      double y = y_view_interval_unconv (viy, ip.y);

      gchar buffer[64];
      sprintf (buffer, "(%1.2e,%1.2e)", x, y);
      y_element_view_set_status (Y_ELEMENT_VIEW(view), buffer);
    }

  return FALSE;
}

static gboolean
y_scatter_line_view_button_press_event (GtkWidget * widget,
					GdkEventButton * event)
{
  YElementViewCartesian *view = Y_ELEMENT_VIEW_CARTESIAN (widget);
  YScatterLineView *line_view = Y_SCATTER_LINE_VIEW (widget);

  /* Ignore double-clicks and triple-clicks */
  if (gdk_event_triggers_context_menu ((GdkEvent *) event) &&
      event->type == GDK_BUTTON_PRESS)
    {
      do_popup_menu (widget, event);
      return TRUE;
    }

  if (y_element_view_get_zooming (Y_ELEMENT_VIEW (view))
      && event->button == 1)
    {
      YViewInterval *viy = y_element_view_cartesian_get_view_interval (view,
								       Y_AXIS);
      YViewInterval *vix = y_element_view_cartesian_get_view_interval (view,
								       X_AXIS);

      YPoint ip = _view_event_point(widget,(GdkEvent *)event);

      line_view->op_start.x = y_view_interval_unconv (vix, ip.x);
      line_view->op_start.y = y_view_interval_unconv (viy, ip.y);
      line_view->zoom_in_progress = TRUE;
    }
  else if (event->button == 1 && (event->state & GDK_SHIFT_MASK)
    && y_element_view_get_panning (Y_ELEMENT_VIEW (view)))
    {
      YViewInterval *viy = y_element_view_cartesian_get_view_interval (view,
								       Y_AXIS);
      YViewInterval *vix = y_element_view_cartesian_get_view_interval (view,
								       X_AXIS);

      YPoint ip = _view_event_point(widget,(GdkEvent *)event);

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

      YPoint ip = _view_event_point(widget,(GdkEvent *)event);

      line_view->op_start.x = y_view_interval_unconv (vix, ip.x);
      line_view->op_start.y = y_view_interval_unconv (viy, ip.y);

      /* this is the position where the pan started */

      line_view->pan_in_progress = TRUE;
    }

  return FALSE;
}

static gboolean
y_scatter_line_view_button_release_event (GtkWidget * widget,
					  GdkEventButton * event)
{
  YElementViewCartesian *view = Y_ELEMENT_VIEW_CARTESIAN (widget);
  YScatterLineView *line_view = Y_SCATTER_LINE_VIEW (widget);

  if (line_view->zoom_in_progress)
    {
      YViewInterval *viy = y_element_view_cartesian_get_view_interval (view,
								       Y_AXIS);
      YViewInterval *vix = y_element_view_cartesian_get_view_interval (view,
								       X_AXIS);

      YPoint ip = _view_event_point(widget,(GdkEvent *)event);
      YPoint zoom_end;
      zoom_end.x = y_view_interval_unconv (vix, ip.x);
      zoom_end.y = y_view_interval_unconv (viy, ip.y);
      y_view_interval_set_ignore_preferred_range (vix, TRUE);
      y_view_interval_set_ignore_preferred_range (viy, TRUE);
      y_element_view_freeze (Y_ELEMENT_VIEW (widget));
      if (line_view->op_start.x != zoom_end.x
        || line_view->op_start.y != zoom_end.y)
        {
          y_view_interval_set (vix, line_view->op_start.x, zoom_end.x);
          y_view_interval_set (viy, line_view->op_start.y, zoom_end.y);
        }
      else
      {
        y_rescale_around_val(vix,zoom_end.x, event);
        y_rescale_around_val(viy,zoom_end.y, event);
      }
      y_element_view_thaw (Y_ELEMENT_VIEW (widget));

      line_view->zoom_in_progress = FALSE;
    }
    else if (line_view->pan_in_progress)
      {
        line_view->pan_in_progress = FALSE;
      }
  return FALSE;
}

/* calculate the interval that just fits the data inside, with a little extra margin, and put it in a and b

return TRUE ("valid") if:
The min and max calculated actually correspond to "valid" points in the sequence, in the sense that negative numbers are bad on a logarithmic interval.

*/
static gboolean
valid_range (YViewInterval * vi, YVector * data, double *a, double *b)
{
  gint i, i0, i1;
  double min = 0.0;
  double max = 1.0;
  double w;
  gboolean first_min = TRUE, first_max = TRUE;

  if (y_vector_get_len (data) == 0)
    {
      *a = 0.0;
      *b = 1.0;
      return TRUE;
    }

  y_vector_get_minmax (data, &min, &max);

  g_debug ("seq range: %e %e\n", min, max);

  if (!(y_view_interval_valid (vi, min) && y_view_interval_valid (vi, max)))
    {
      i1 = y_vector_get_len (data) - 1;
      i0 = 0;

      for (i = i0; i <= i1; ++i)
      {
        double x = y_vector_get_value (data, i);

        if (y_view_interval_valid (vi, x))
        {
          if (first_min)
          {
            min = x;
            first_min = FALSE;
          }
          else
          {
            if (x < min)
              min = x;
          }

          if (first_max)
          {
            max = x;
            first_max = FALSE;
          }
          else
          {
            if (x > max)
              max = x;
          }
        }
      }

      if (first_min || first_max)
        return FALSE;
    }

  /* Add 5% in 'margins' */
  w = max - min;
  if (w == 0)
    w = (min != 0 ? min : 1.0);
  if(!y_view_interval_is_logarithmic(vi))
    min -= w * 0.025;
  max += w * 0.025;

  if (a)
    *a = min;
  if (b)
    *b = max;

  return TRUE;
}

static gboolean
preferred_range (YElementViewCartesian * cart, YAxisType ax, double *a,
		 double *b)
{
  YVector *seq = NULL;
  YScatterLineView *scat = Y_SCATTER_LINE_VIEW (cart);

  *a = NAN;
  *b = NAN;

  g_debug ("scatter view preferred range");

  /* should loop over all series, come up with a range that fits all */
  GList *l = scat->series;
  if (l == NULL)
    return FALSE;

  gboolean vr = FALSE;

  for(l = scat->series; l != NULL; l=l->next )
  {
    YScatterSeries *series = Y_SCATTER_SERIES (l->data);

    YVector *xdata, *ydata;
    g_object_get (series, "x-data", &xdata, "y-data", &ydata, NULL);

    if (ax == X_AXIS)
      seq = xdata;
    else if (ax == Y_AXIS)
      seq = ydata;
    else
      return FALSE;

    if (seq)
      {
        double ai,bi;
        gboolean vrp = valid_range (y_element_view_cartesian_get_view_interval (cart, ax),
		      seq, &ai, &bi);
        if(vrp) {
          if(isnan(*a) || *a>ai)
            *a = ai;
          if(isnan(*b) || *b<bi)
            *b = bi;
        }
        vr = vrp || vr;
      }
    else if (ax == X_AXIS && ydata != NULL)
      {
        int n = y_vector_get_len (ydata);
        if(isnan(*a) || isnan(*b)) {
          if(isnan(*a))
            *a = 0.0;
          if(isnan(*b))
            *b = (double) n;
        }
        else {
          *a = MIN(0.0,*a);
          *b = MAX((double) n,*b);
        }
        vr = TRUE;
      }
  }
  return vr;

  return FALSE;
}

static void
get_preferred_size (GtkWidget * w, gint * minimum, gint * natural)
{
  *minimum = 100;
  *natural = 2000;
}

static inline void
draw_marker_circle (cairo_t * cr, YPoint pos, double size, gboolean fill)
{
  cairo_arc (cr, pos.x, pos.y, size / 2, 0, 2 * G_PI);
  fill ? cairo_fill (cr) : cairo_stroke(cr);
}

static inline void
draw_marker_square (cairo_t * cr, YPoint pos, double size, gboolean fill)
{
  cairo_move_to (cr, pos.x - size / 2, pos.y - size / 2);
  cairo_line_to (cr, pos.x - size / 2, pos.y + size / 2);
  cairo_line_to (cr, pos.x + size / 2, pos.y + size / 2);
  cairo_line_to (cr, pos.x + size / 2, pos.y - size / 2);
  cairo_line_to (cr, pos.x - size / 2, pos.y - size / 2);
  fill ? cairo_fill (cr) : cairo_stroke(cr);
}

static inline void
draw_marker_diamond (cairo_t * cr, YPoint pos, double size, gboolean fill)
{
  cairo_move_to (cr, pos.x - M_SQRT1_2 * size, pos.y);
  cairo_line_to (cr, pos.x, pos.y + M_SQRT1_2 * size);
  cairo_line_to (cr, pos.x + M_SQRT1_2 * size, pos.y);
  cairo_line_to (cr, pos.x, pos.y - M_SQRT1_2 * size);
  cairo_line_to (cr, pos.x - M_SQRT1_2 * size, pos.y);
  fill ? cairo_fill (cr) : cairo_stroke(cr);
}

static inline void
draw_marker_x (cairo_t * cr, YPoint pos, double size)
{
  cairo_move_to (cr, pos.x - size / 2, pos.y - size / 2);
  cairo_line_to (cr, pos.x + size / 2, pos.y + size / 2);
  cairo_stroke (cr);
  cairo_move_to (cr, pos.x + size / 2, pos.y - size / 2);
  cairo_line_to (cr, pos.x - size / 2, pos.y + size / 2);
  cairo_stroke (cr);
}

static inline void
draw_marker_plus (cairo_t * cr, YPoint pos, double size)
{
  cairo_move_to (cr, pos.x - size / 2, pos.y);
  cairo_line_to (cr, pos.x + size / 2, pos.y);
  cairo_stroke (cr);
  cairo_move_to (cr, pos.x, pos.y - size / 2);
  cairo_line_to (cr, pos.x, pos.y + size / 2);
  cairo_stroke (cr);
}

struct draw_struct
{
  YScatterLineView *scat;
  cairo_t *cr;
};

static void
series_draw (gpointer data, gpointer user_data)
{
  YScatterSeries *series = Y_SCATTER_SERIES (data);
  struct draw_struct *s = user_data;
  YScatterLineView *scat = s->scat;
  cairo_t *cr = s->cr;
  GtkWidget *w = GTK_WIDGET (scat);

  YVector *xdata, *ydata;
  g_object_get (series, "x-data", &xdata, "y-data", &ydata, NULL);

  YViewInterval *vi_x, *vi_y;
  int i, N;

#if PROFILE
  GTimer *t = g_timer_new ();
#endif

  g_debug ("scatter view draw");

  if (ydata == NULL)
    {
      g_debug ("data was null...");
      return;
    }

  vi_x =
    y_element_view_cartesian_get_view_interval (Y_ELEMENT_VIEW_CARTESIAN (w),
						X_AXIS);

  vi_y =
    y_element_view_cartesian_get_view_interval (Y_ELEMENT_VIEW_CARTESIAN (w),
						Y_AXIS);

  if (xdata == NULL)
    {
      N = y_vector_get_len (ydata);
    }
  else
    {
      N = MIN (y_vector_get_len (xdata), y_vector_get_len (ydata));
    }

  //g_message("length is %d %d",y_vector_get_len (xdata),y_vector_get_len (ydata));

  if (N < 1)
    {
      return;
    }

  YPoint pos[N];
  double buffer[N];

  if (xdata != NULL)
    {
      const double *xraw = y_vector_get_values (xdata);
      y_view_interval_conv_bulk (vi_x, xraw, buffer, N);

      for (i = 0; i < N; i++)
      {
        pos[i].x = buffer[i];
      }
    }
  else
    {
      for (i = 0; i < N; i++)
      {
        pos[i].x = y_view_interval_conv (vi_x, (double) i);
      }
    }

  const double *yraw = y_vector_get_values (ydata);

  y_view_interval_conv_bulk (vi_y, yraw, buffer, N);
  for (i = 0; i < N; i++)
    {
      pos[i].y = buffer[i];
    }

  _view_conv_bulk (w, pos, pos, N);

#if PROFILE
  double te = g_timer_elapsed (t, NULL);
  g_message ("scatter view before draw: %f ms", N, te * 1000);
#endif

  gboolean draw_line;
  double line_width;
  GdkRGBA *line_color;

  g_object_get (series, "draw-line", &draw_line, "line-width", &line_width,
		"line-color", &line_color, NULL);

  gboolean found_nan = FALSE;

  if (draw_line && N > 1)
    {
      cairo_set_line_width (cr, line_width);
      //canvas_set_dashing (canvas, NULL, 0);

      cairo_set_source_rgba (cr, line_color->red, line_color->green,
			     line_color->blue, line_color->alpha);

      cairo_move_to (cr, pos[0].x, pos[0].y);
      for (i = 1; i < N; i++)
      {
        if(isnan(pos[i].x) || isnan(pos[i].y)) {
          found_nan = TRUE;
        }
        else {
          if(found_nan)
            cairo_move_to(cr, pos[i].x, pos[i].y);
          else
            cairo_line_to (cr, pos[i].x, pos[i].y);
          found_nan = FALSE;
        }
      }
      cairo_stroke (cr);
    }

  GdkRGBA *marker_color;
  double marker_size;
  YMarker marker_type;

  g_object_get (series, "marker-color",
		&marker_color, "marker-size", &marker_size, "marker",
		&marker_type, NULL);

  if (marker_type != Y_MARKER_NONE)
    {
      cairo_set_source_rgba (cr, marker_color->red, marker_color->green,
			     marker_color->blue, marker_color->alpha);

      switch (marker_type)
      {
        case Y_MARKER_CIRCLE:
        for (i = 0; i < N; i++)
        {
          if(!isnan(pos[i].x) & !isnan(pos[i].y))
            draw_marker_circle (cr, pos[i], marker_size, TRUE);
        }
        break;
        case Y_MARKER_OPEN_CIRCLE:
        for (i = 0; i < N; i++)
        {
          if(!isnan(pos[i].x) & !isnan(pos[i].y))
            draw_marker_circle (cr, pos[i], marker_size, FALSE);
        }
        break;
        case Y_MARKER_SQUARE:
        for (i = 0; i < N; i++)
        {
          if(!isnan(pos[i].x) & !isnan(pos[i].y))
            draw_marker_square (cr, pos[i], marker_size, TRUE);
        }
        break;
        case Y_MARKER_OPEN_SQUARE:
        for (i = 0; i < N; i++)
        {
          if(!isnan(pos[i].x) & !isnan(pos[i].y))
            draw_marker_square (cr, pos[i], marker_size, FALSE);
        }
        break;
        case Y_MARKER_DIAMOND:
        for (i = 0; i < N; i++)
        {
          if(!isnan(pos[i].x) & !isnan(pos[i].y))
            draw_marker_diamond (cr, pos[i], marker_size, TRUE);
        }
        break;
        case Y_MARKER_OPEN_DIAMOND:
        for (i = 0; i < N; i++)
        {
          if(!isnan(pos[i].x) & !isnan(pos[i].y))
            draw_marker_diamond (cr, pos[i], marker_size, FALSE);
        }
        break;
        case Y_MARKER_X:
        for (i = 0; i < N; i++)
        {
          if(!isnan(pos[i].x) & !isnan(pos[i].y))
            draw_marker_x (cr, pos[i], marker_size);
        }
        break;
        case Y_MARKER_PLUS:
        for (i = 0; i < N; i++)
        {
          if(!isnan(pos[i].x) & !isnan(pos[i].y))
            draw_marker_plus (cr, pos[i], marker_size);
        }
        break;
        default:
        break;
      }
    }

#if PROFILE
  gint64 now = g_get_real_time();
  gint64 then = y_data_get_timestamp(Y_DATA(xdata));
  te = g_timer_elapsed (t, NULL);
  g_message ("scatter view draw %d points: %f ms", N, te * 1000);
  g_message ("microseconds: %d",(int) (now-then));
  g_timer_destroy (t);
#endif
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
y_scatter_line_view_draw (GtkWidget * w, cairo_t * cr)
{
  YScatterLineView *scat = Y_SCATTER_LINE_VIEW (w);

  struct draw_struct *s = g_malloc (sizeof (struct draw_struct));
  s->scat = scat;
  s->cr = cr;

  g_list_foreach (scat->series, series_draw, s);

  g_free (s);

  if (scat->zoom_in_progress)
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

      pstart.x = y_view_interval_conv (vi_x, scat->op_start.x);
      pend.x = y_view_interval_conv (vi_x, scat->cursor_pos.x);
      pstart.y = y_view_interval_conv (vi_y, scat->op_start.y);
      pend.y = y_view_interval_conv (vi_y, scat->cursor_pos.y);

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

  return TRUE;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
on_data_changed (YData * data, gpointer user_data)
{
  YElementView *mev = (YElementView *) user_data;
  g_assert (mev);
  y_element_view_changed (mev);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

/**
 * y_scatter_line_view_add_series:
 * @v: a #YScatterLineView
 * @s: (transfer full): a #YScatterSeries
 *
 * Add a series to the plot @v.
 **/
void
y_scatter_line_view_add_series (YScatterLineView * v, YScatterSeries * s)
{
  v->series = g_list_append (v->series, g_object_ref_sink(s));
  /* connect changed signals */
  YVector *xdata, *ydata;
  g_object_get (s, "x-data", &xdata, "y-data", &ydata, NULL);

  /* TODO: connect to a "subdata changed" signal on series so that:
     - if x is set after series is added, we can connect to signals
     - if data is changed we connect and disconnect to signals
   */

  if (xdata != NULL)
    {
      g_signal_connect_after (xdata, "changed", G_CALLBACK (on_data_changed),
			      v);
    }
  if (ydata != NULL)
    {
      g_signal_connect_after (ydata, "changed", G_CALLBACK (on_data_changed),
			      v);
    }

  YElementViewCartesian *cart = (YElementViewCartesian *) v;
  YViewInterval *vix =
    y_element_view_cartesian_get_view_interval (cart, X_AXIS);
  YViewInterval *viy =
    y_element_view_cartesian_get_view_interval (cart, Y_AXIS);

  if (vix)
    y_view_interval_request_preferred_range (vix);
  if (viy)
    y_view_interval_request_preferred_range (viy);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
y_scatter_line_view_class_init (YScatterLineViewClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  YElementViewClass *view_class = Y_ELEMENT_VIEW_CLASS (klass);
  YElementViewCartesianClass *cart_class =
    Y_ELEMENT_VIEW_CARTESIAN_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = y_scatter_line_view_finalize;

  widget_class->get_preferred_width = get_preferred_size;
  widget_class->get_preferred_height = get_preferred_size;

  widget_class->scroll_event = y_scatter_line_view_scroll_event;
  widget_class->button_press_event = y_scatter_line_view_button_press_event;
  widget_class->motion_notify_event = y_scatter_line_view_motion_notify_event;
  widget_class->button_release_event =
    y_scatter_line_view_button_release_event;

  widget_class->draw = y_scatter_line_view_draw;

  view_class->changed = changed;

  cart_class->preferred_range = preferred_range;
}

static void
y_scatter_line_view_init (YScatterLineView * obj)
{
  g_object_set (obj, "expand", FALSE, "valign", GTK_ALIGN_START, "halign",
		GTK_ALIGN_START, NULL);

  gtk_widget_add_events (GTK_WIDGET (obj),
			 GDK_SCROLL_MASK | GDK_BUTTON_PRESS_MASK |
			 GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);

  y_element_view_cartesian_add_view_interval (Y_ELEMENT_VIEW_CARTESIAN (obj),
       				      X_AXIS);
  y_element_view_cartesian_add_view_interval (Y_ELEMENT_VIEW_CARTESIAN (obj),
       				      Y_AXIS);

  g_debug ("y_scatter_line_view_init");
}

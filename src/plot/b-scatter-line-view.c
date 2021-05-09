/*
 * b-scatter-view.c
 *
 * Copyright (C) 2000 EMC Capital Management, Inc.
 * Copyright (C) 2016, 2019 Scott O. Johnson (scojo202@gmail.com)
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

#include <string.h>
#include "plot/b-scatter-line-view.h"
#include "data/b-data-class.h"

/**
 * SECTION: b-scatter-line-view
 * @short_description: View for a scatter and/or line plot.
 *
 * Displays a line and/or scatter plot, e.g. y as a function of x. Collections
 * of (x,y) pairs are grouped into series, each of which has its own style
 * (type of marker, line color and dash type, etc.). Series are created using
 * #BScatterSeries and added using b_scatter_line_view_add_series().
 *
 * The axis type to use to get the horizontal axis is X_AXIS, and the axis type
 * to get the vertical axis is Y_AXIS.
 */

static GObjectClass *parent_class = NULL;

#define PROFILE 0

enum
{
  PROP_V_CURSOR_POS = 1,
  PROP_H_CURSOR_POS,
  PROP_SHOW_CURSORS,
  PROP_CURSOR_COLOR,
  PROP_CURSOR_WIDTH
};

struct _BScatterLineView
{
  BElementViewCartesian base;
  GList *series;
  BPoint op_start;
  BPoint cursor_pos;
  double v_cursor;
  double h_cursor;
  GdkRGBA cursor_color;
  double cursor_width;
  gboolean show_cursors;
  gboolean zoom_in_progress;
  gboolean pan_in_progress;
  gboolean v_cursor_move_in_progress;
  gboolean h_cursor_move_in_progress;
  GdkCursor *cursor;
};

G_DEFINE_TYPE (BScatterLineView, b_scatter_line_view,
	       B_TYPE_ELEMENT_VIEW_CARTESIAN);

static void
handlers_disconnect_and_clear (gpointer data, gpointer user_data)
{
  BScatterSeries *series = B_SCATTER_SERIES (data);
  BScatterLineView *v = B_SCATTER_LINE_VIEW (user_data);

  BData *xdata, *ydata, *xerr, *yerr;
  g_object_get(series, "x-data", &xdata, "y-data", &ydata,
                       "x-err", &xerr, "y-err", &yerr, NULL);

  if (xdata != NULL)
    {
      g_signal_handlers_disconnect_by_data (xdata, v);
      g_object_unref(xdata);
    }

  if (ydata != NULL)
    {
      g_signal_handlers_disconnect_by_data (ydata, v);
      g_object_unref(ydata);
    }

  if (xerr != NULL)
    {
      g_signal_handlers_disconnect_by_data (xerr, v);
      g_object_unref(xerr);
    }

  if (yerr != NULL)
    {
      g_signal_handlers_disconnect_by_data (yerr, v);
      g_object_unref(yerr);
    }
  g_clear_object(&series);
}

static void
b_scatter_line_view_finalize (GObject * obj)
{
  BScatterLineView *v = B_SCATTER_LINE_VIEW (obj);
  g_list_foreach (v->series, handlers_disconnect_and_clear, v);
  g_list_free (v->series);

  if (parent_class->finalize)
    parent_class->finalize (obj);
}

#if 0
static gboolean
rectangle_contains_point (cairo_rectangle_t rect, BPoint * point)
{
  return ((point->x > rect.x) && (point->x < rect.x + rect.width)
	  && (point->y > rect.y) && (point->y < rect.y + rect.height));
}
#endif

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
changed (BElementView * gev)
{
  BElementViewCartesian *cart = B_ELEMENT_VIEW_CARTESIAN (gev);

  BViewInterval *vix =
    b_element_view_cartesian_get_view_interval (cart, X_AXIS);
  BViewInterval *viy =
    b_element_view_cartesian_get_view_interval (cart, Y_AXIS);

  if (vix)
    b_view_interval_request_preferred_range (vix);
  if (viy)
    b_view_interval_request_preferred_range (viy);

  if (B_ELEMENT_VIEW_CLASS (parent_class)->changed)
    B_ELEMENT_VIEW_CLASS (parent_class)->changed (gev);
}

static gboolean
b_scatter_line_view_scroll_event (GtkEventControllerScroll * controller, double dx, double dy, gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET(user_data);
  BElementViewCartesian *view = B_ELEMENT_VIEW_CARTESIAN (widget);
  BScatterLineView *scat = B_SCATTER_LINE_VIEW(user_data);

  gboolean scroll = FALSE;
  gboolean direction;
  if (dy > 0.0)
    {
      scroll = TRUE;
      direction = TRUE;
    }
  else if (dy < 0.0)
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

  b_view_interval_rescale_around_point (vix,
					scat->cursor_pos.x,
					scale);
  b_view_interval_rescale_around_point (viy,
					scat->cursor_pos.y,
					scale);

  return FALSE;
}

/*
static void
show_cursors_toggled (GtkCheckMenuItem * checkmenuitem, gpointer user_data)
{
  BScatterLineView *scat = B_SCATTER_LINE_VIEW(user_data);
  g_object_set(scat,"show-cursors",
                    gtk_check_menu_item_get_active (checkmenuitem),NULL);
}

static void
b_scatter_line_do_popup (GtkGestureClick *gesture,
            guint            n_press,
            double           x,
            double           y,
            BElementViewCartesian *view)
{
  BScatterLineView *scat = B_SCATTER_LINE_VIEW(view);

  GMenu *menu = g_menu_new();

  GMenuItem *autoscale_x =
    _y_create_autoscale_menu_check_item (view, X_AXIS, "Autoscale X axis");
  gtk_widget_show (autoscale_x);
  GtkWidget *autoscale_y =
    _y_create_autoscale_menu_check_item (view, Y_AXIS, "Autoscale Y axis");
  gtk_widget_show (autoscale_y);

  GtkWidget *show_cursors = gtk_check_menu_item_new_with_label ("Show cursors");
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (show_cursors),
                                  scat->show_cursors);
  g_signal_connect (show_cursors, "toggled", G_CALLBACK (show_cursors_toggled),
                    scat);
  gtk_widget_show (show_cursors);

  gtk_menu_shell_append (GTK_MENU_SHELL (menu), autoscale_x);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), autoscale_y);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), show_cursors);

  gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}
*/

static gboolean
b_scatter_line_view_motion_notify_event (GtkEventControllerMotion *controller, double x, double y, gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET(user_data);
  BElementViewCartesian *view = B_ELEMENT_VIEW_CARTESIAN (user_data);
  BScatterLineView *line_view = B_SCATTER_LINE_VIEW (user_data);

  BViewInterval *viy = b_element_view_cartesian_get_view_interval (view,
                 Y_AXIS);
  BViewInterval *vix = b_element_view_cartesian_get_view_interval (view,
                 X_AXIS);

  BPoint ip, evp;
  evp.x = x;
  evp.y = y;

  _view_invconv (widget, &evp, &ip);

  line_view->cursor_pos.x = b_view_interval_unconv (vix, ip.x);
  line_view->cursor_pos.y = b_view_interval_unconv (viy, ip.y);

  if (line_view->zoom_in_progress)
    {
      gtk_widget_queue_draw (widget);	/* for the zoom box */
    }
    else if (line_view->pan_in_progress)
      {
        /* Calculate the translation required to put the cursor at the
         * start position. */

        double vx = b_view_interval_unconv (vix, ip.x);
        double dvx = vx - line_view->op_start.x;

        double vy = b_view_interval_unconv (viy, ip.y);
        double dvy = vy - line_view->op_start.y;

        b_view_interval_translate (vix, -dvx);
        b_view_interval_translate (viy, -dvy);
      }

  if (b_element_view_get_status_label(B_ELEMENT_VIEW(view)))
    {
      double x = b_view_interval_unconv (vix, ip.x);
      double y = b_view_interval_unconv (viy, ip.y);

      GString *str = g_string_new("(");
      _append_format_double_scinot(str,x);
      g_string_append(str,",");
      _append_format_double_scinot(str,y);
      g_string_append(str,")");
      b_element_view_set_status (B_ELEMENT_VIEW(view), str->str);
      g_string_free(str,TRUE);
    }

  /*if (line_view->show_cursors) {
    GdkWindow *window = gtk_widget_get_window (widget);
    GdkDisplay *display = gtk_widget_get_display (widget);
    double w = b_view_interval_get_width (vix);
    if(fabs(line_view->v_cursor - b_view_interval_unconv (vix, ip.x))<0.01*w)
      {
        g_clear_object(&line_view->cursor);
        line_view->cursor = gdk_cursor_new_from_name (display, "ew-resize");
        gdk_window_set_cursor (window, line_view->cursor);
      }
    else if(fabs(line_view->h_cursor - b_view_interval_unconv (viy, ip.y))<0.01*b_view_interval_get_width (viy))
      {
        g_clear_object(&line_view->cursor);
        line_view->cursor = gdk_cursor_new_from_name (display, "ns-resize");
        gdk_window_set_cursor (window, line_view->cursor);
      }
    else if (line_view->cursor != NULL)
    {
      gdk_window_set_cursor (window, NULL);
      g_object_unref(line_view->cursor);
      line_view->cursor = NULL;
    }

    if (line_view->h_cursor_move_in_progress == TRUE)
    {
      double y = b_view_interval_unconv (viy, ip.y);
      g_object_set(view, "h-cursor-pos", y, NULL);
    }

    if (line_view->v_cursor_move_in_progress == TRUE)
    {
      double x = b_view_interval_unconv (vix, ip.x);
      g_object_set(view, "v-cursor-pos", x, NULL);
    }
  }*/

  return FALSE;
}

static gboolean
b_scatter_line_view_press_event (GtkGestureClick *gesture,
               int n_press,
                                 double x, double y,
               gpointer         user_data)
{
  GtkWidget *widget = GTK_WIDGET(user_data);
  BElementViewCartesian *view = B_ELEMENT_VIEW_CARTESIAN (widget);
  BScatterLineView *line_view = B_SCATTER_LINE_VIEW (widget);

  /* Ignore double-clicks and triple-clicks */

  if(n_press != 1)
    return FALSE;

  BViewInterval *viy = b_element_view_cartesian_get_view_interval (view,
								       Y_AXIS);
  BViewInterval *vix = b_element_view_cartesian_get_view_interval (view,
								       X_AXIS);
  BPoint ip,evp;
  evp.x = x;
  evp.y = y;

  _view_invconv (widget, &evp, &ip);
  GdkModifierType t =gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));

  if (b_element_view_get_zooming (B_ELEMENT_VIEW (view)))
    {
      line_view->op_start.x = b_view_interval_unconv (vix, ip.x);
      line_view->op_start.y = b_view_interval_unconv (viy, ip.y);
      line_view->zoom_in_progress = TRUE;
    }
  else if ((t & GDK_SHIFT_MASK)
      && b_element_view_get_panning (B_ELEMENT_VIEW (view)))
    {
      b_view_interval_set_ignore_preferred_range (vix, TRUE);
      b_view_interval_set_ignore_preferred_range (viy, TRUE);

      b_view_interval_recenter_around_point (vix,
  				     b_view_interval_unconv (vix,
	  							ip.x));
      b_view_interval_recenter_around_point (viy,
				  	     b_view_interval_unconv (viy,
					  				ip.y));
      }
    else if (b_element_view_get_panning (B_ELEMENT_VIEW (view)))
      {
        b_view_interval_set_ignore_preferred_range (vix, TRUE);
        b_view_interval_set_ignore_preferred_range (viy, TRUE);

        line_view->op_start.x = b_view_interval_unconv (vix, ip.x);
        line_view->op_start.y = b_view_interval_unconv (viy, ip.y);

        /* this is the position where the pan started */

        line_view->pan_in_progress = TRUE;
      }
    else {
      double w = b_view_interval_get_width (vix);
      if(fabs(line_view->v_cursor - b_view_interval_unconv (vix, ip.x))<0.01*w)
        {
          line_view->v_cursor_move_in_progress = TRUE;
        }
      w = b_view_interval_get_width (viy);
      if(fabs(line_view->h_cursor - b_view_interval_unconv (viy, ip.y))<0.01*w)
        {
          line_view->h_cursor_move_in_progress = TRUE;
        }
    }

  return FALSE;
}

static gboolean
b_scatter_line_view_release_event (GtkGestureClick *gesture,
               int n_press,
                                 double x, double y,
               gpointer         user_data)
{
  GtkWidget *widget = GTK_WIDGET(user_data);
  BElementViewCartesian *view = B_ELEMENT_VIEW_CARTESIAN (widget);
  BScatterLineView *line_view = B_SCATTER_LINE_VIEW (widget);

  /* Ignore double-clicks and triple-clicks */

  /*if (gdk_event_triggers_context_menu (event) &&
      gdk_event_get_event_type(event) == GDK_BUTTON_PRESS)
    {
      //do_popup_menu (widget, event);
      return TRUE;
    }*/

  BPoint ip,evp;
  evp.x = x;
  evp.y = y;

  _view_invconv (widget, &evp, &ip);

  if (line_view->zoom_in_progress)
    {
      BViewInterval *viy = b_element_view_cartesian_get_view_interval (view,
								       Y_AXIS);
      BViewInterval *vix = b_element_view_cartesian_get_view_interval (view,
								       X_AXIS);

      BPoint zoom_end;
      zoom_end.x = b_view_interval_unconv (vix, ip.x);
      zoom_end.y = b_view_interval_unconv (viy, ip.y);
      b_view_interval_set_ignore_preferred_range (vix, TRUE);
      b_view_interval_set_ignore_preferred_range (viy, TRUE);
      b_element_view_freeze (B_ELEMENT_VIEW (widget));
      if (line_view->op_start.x != zoom_end.x
        || line_view->op_start.y != zoom_end.y)
        {
          b_view_interval_set (vix, line_view->op_start.x, zoom_end.x);
          b_view_interval_set (viy, line_view->op_start.y, zoom_end.y);
        }
      else
      {
        GdkModifierType t =gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));
        if(t & GDK_ALT_MASK) {
          b_view_interval_rescale_around_point (vix, zoom_end.x, 1.0/0.8);
          b_view_interval_rescale_around_point (viy, zoom_end.y, 1.0/0.8);
        }
        else {
          b_view_interval_rescale_around_point (vix, zoom_end.x, 0.8);
          b_view_interval_rescale_around_point (viy, zoom_end.y, 0.8);
        }
      }
      b_element_view_thaw (B_ELEMENT_VIEW (widget));

      line_view->zoom_in_progress = FALSE;
    }
    else if (line_view->pan_in_progress)
      {
        line_view->pan_in_progress = FALSE;
      }

    line_view->h_cursor_move_in_progress = FALSE;
    line_view->v_cursor_move_in_progress = FALSE;

  return FALSE;
}

/* calculate the interval that just fits the data inside, with a little extra margin, and put it in a and b

return TRUE ("valid") if:
The min and max calculated actually correspond to "valid" points in the sequence, in the sense that negative numbers are bad on a logarithmic interval.

*/
static gboolean
valid_range (BViewInterval * vi, BVector * data, double *a, double *b)
{
  gint i, i0, i1;
  double min = 0.0;
  double max = 1.0;
  double w;
  gboolean first_min = TRUE, first_max = TRUE;

  if (b_vector_get_len (data) == 0)
    {
      *a = 0.0;
      *b = 1.0;
      return TRUE;
    }

  b_vector_get_minmax (data, &min, &max);

  if (!(b_view_interval_valid (vi, min) && b_view_interval_valid (vi, max)))
    {
      i1 = b_vector_get_len (data) - 1;
      i0 = 0;

      for (i = i0; i <= i1; ++i)
      {
        double x = b_vector_get_value (data, i);

        if (b_view_interval_valid (vi, x))
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
  if(!b_view_interval_is_logarithmic(vi))
    min -= w * 0.025;
  max += w * 0.025;

  if (a)
    *a = min;
  if (b)
    *b = max;

  return TRUE;
}

static gboolean
preferred_range (BElementViewCartesian * cart, BAxisType ax, double *a,
		 double *b)
{
  BVector *seq = NULL;
  BScatterLineView *scat = B_SCATTER_LINE_VIEW (cart);

  *a = NAN;
  *b = NAN;

  /* should loop over all series, come up with a range that fits all */
  GList *l = scat->series;
  if (l == NULL)
    return FALSE;

  gboolean vr = FALSE;

  for(l = scat->series; l != NULL; l=l->next )
  {
    BScatterSeries *series = B_SCATTER_SERIES (l->data);

    BVector *xdata, *ydata;
    gboolean show;
    g_object_get (series, "x-data", &xdata, "y-data", &ydata,
                          "show", &show, NULL);

    if(!show)
      continue;

    if (ax == X_AXIS)
      seq = xdata;
    else if (ax == Y_AXIS)
      seq = ydata;
    else
      return FALSE;

    if (seq)
      {
        double ai,bi;
        gboolean vrp = valid_range (b_element_view_cartesian_get_view_interval (cart, ax),
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
        int n = b_vector_get_len (ydata);
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
    g_clear_object(&xdata);
    g_clear_object(&ydata);
  }
  return vr;

  return FALSE;
}

struct draw_struct
{
  BScatterLineView *scat;
  cairo_t *cr;
};

static void
series_draw (gpointer data, gpointer user_data)
{
  BScatterSeries *series = B_SCATTER_SERIES (data);

  if(!b_scatter_series_get_show(series))
    return;
  struct draw_struct *s = user_data;
  BScatterLineView *scat = s->scat;
  cairo_t *cr = s->cr;
  GtkWidget *w = GTK_WIDGET (scat);

  BVector *xdata, *ydata;
  BData *xerr, *yerr;
  g_object_get (series, "x-data", &xdata, "y-data", &ydata,
                        "x-err", &xerr, "y-err", &yerr, NULL);

  BViewInterval *vi_x, *vi_y;
  int i, N;

#if PROFILE
  GTimer *t = g_timer_new ();
#endif

  if (ydata == NULL)
    {
      return;
    }

  vi_x =
    b_element_view_cartesian_get_view_interval (B_ELEMENT_VIEW_CARTESIAN (w),
						X_AXIS);

  vi_y =
    b_element_view_cartesian_get_view_interval (B_ELEMENT_VIEW_CARTESIAN (w),
						Y_AXIS);

  if (xdata == NULL)
    {
      N = b_vector_get_len (ydata);
    }
  else
    {
      N = MIN (b_vector_get_len (xdata), b_vector_get_len (ydata));
    }

  if (N < 1)
    {
      g_clear_object(&xdata);
      g_clear_object(&ydata);
      return;
    }

  BPoint *pos = g_new (BPoint, N);
  double *buffer = g_new (double, N);

  if (xdata != NULL)
    {
      const double *xraw = b_vector_get_values (xdata);
      b_view_interval_conv_bulk (vi_x, xraw, buffer, N);

      for (i = 0; i < N; i++)
      {
        pos[i].x = buffer[i];
      }
    }
  else
    {
      for (i = 0; i < N; i++)
      {
        pos[i].x = b_view_interval_conv (vi_x, (double) i);
      }
    }

  const double *yraw = b_vector_get_values (ydata);

  b_view_interval_conv_bulk (vi_y, yraw, buffer, N);
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
  BDashing dash;

  g_object_get (series, "draw-line", &draw_line, "line-width", &line_width,
		"line-color", &line_color, "dashing", &dash, NULL);

  gboolean found_nan = FALSE;

  if (draw_line && N > 1)
    {
      cairo_save (cr);
      cairo_set_line_width (cr, line_width);

      cairo_set_source_rgba (cr, line_color->red, line_color->green,
			     line_color->blue, line_color->alpha);

      _b_dashing_set (dash, line_width, cr);

      if(isnan(pos[0].x) || isnan(pos[0].y))
        found_nan = TRUE;
      else
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
      cairo_restore (cr);
    }

  GdkRGBA *marker_color;
  double marker_size;
  BMarker marker_type;

  g_object_get (series, "marker-color", &marker_color,
                        "marker-size", &marker_size,
                        "marker", &marker_type, NULL);

  if(xerr != NULL && xdata != NULL) {
    const double *xraw = b_vector_get_values (xdata);
    cairo_save (cr);
    cairo_set_line_width (cr, line_width);

    cairo_set_source_rgba (cr, marker_color->red, marker_color->green,
         marker_color->blue, marker_color->alpha);

    gboolean fixed_err = FALSE;
    double fixed_err_val = 0.0;

    if(B_IS_SCALAR(xerr)) {
      fixed_err = TRUE;
      fixed_err_val = b_scalar_get_value(B_SCALAR(xerr));
    }

    for (i = 0; i < N; i++)
      {
        double err_val = fixed_err ? fixed_err_val : b_vector_get_value(B_VECTOR(xerr),i);
        if(!isnan(pos[i].x) & !isnan(pos[i].y)) {
          BPoint epos, epos2;
          _view_invconv(w,&pos[i],&epos);
          epos.x = b_view_interval_conv (vi_x, xraw[i]-err_val);
          _view_conv(w,&epos, &epos2);
          cairo_move_to(cr, epos2.x, epos2.y-marker_size/2);
          cairo_line_to(cr, epos2.x, epos2.y+marker_size/2);
          cairo_move_to(cr, epos2.x, epos2.y);
          epos.x = b_view_interval_conv (vi_x, xraw[i]+err_val);
          _view_conv(w,&epos, &epos2);
          cairo_line_to(cr, epos2.x, epos2.y);
          cairo_move_to(cr, epos2.x, epos2.y-marker_size/2);
          cairo_line_to(cr, epos2.x, epos2.y+marker_size/2);
          cairo_stroke(cr);
        }
      }
    cairo_restore(cr);
  }

  if(yerr != NULL) {
    cairo_save (cr);
    cairo_set_line_width (cr, line_width);

    cairo_set_source_rgba (cr, marker_color->red, marker_color->green,
         marker_color->blue, marker_color->alpha);

    gboolean fixed_err = FALSE;
    double fixed_err_val = 0.0;

    if(B_IS_SCALAR(yerr)) {
      fixed_err = TRUE;
      fixed_err_val = b_scalar_get_value(B_SCALAR(yerr));
    }

    for (i = 0; i < N; i++)
      {
        double err_val = fixed_err ? fixed_err_val : b_vector_get_value(B_VECTOR(yerr),i);
        if(!isnan(pos[i].x) & !isnan(pos[i].y)) {
          BPoint epos, epos2;
          _view_invconv(w,&pos[i],&epos);
          epos.y = b_view_interval_conv (vi_y, yraw[i]-err_val);
          _view_conv(w,&epos, &epos2);
          cairo_move_to(cr, epos2.x-marker_size/2, epos2.y);
          cairo_line_to(cr, epos2.x+marker_size/2, epos2.y);
          cairo_move_to(cr, epos2.x, epos2.y);
          epos.y = b_view_interval_conv (vi_y, yraw[i]+err_val);
          _view_conv(w,&epos, &epos2);
          cairo_line_to(cr, epos2.x, epos2.y);
          cairo_move_to(cr, epos2.x-marker_size/2, epos2.y);
          cairo_line_to(cr, epos2.x+marker_size/2, epos2.y);
          cairo_stroke(cr);
        }
      }
    cairo_restore(cr);
  }

  if (marker_type != B_MARKER_NONE)
    {
      cairo_set_source_rgba (cr, marker_color->red, marker_color->green,
			     marker_color->blue, marker_color->alpha);

      switch (marker_type)
      {
        case B_MARKER_CIRCLE:
        for (i = 0; i < N; i++)
        {
          if(!isnan(pos[i].x) & !isnan(pos[i].y))
            _draw_marker_circle (cr, pos[i], marker_size, TRUE);
        }
        break;
        case B_MARKER_OPEN_CIRCLE:
        for (i = 0; i < N; i++)
        {
          if(!isnan(pos[i].x) & !isnan(pos[i].y))
            _draw_marker_circle (cr, pos[i], marker_size, FALSE);
        }
        break;
        case B_MARKER_SQUARE:
        for (i = 0; i < N; i++)
        {
          if(!isnan(pos[i].x) & !isnan(pos[i].y))
            _draw_marker_square (cr, pos[i], marker_size, TRUE);
        }
        break;
        case B_MARKER_OPEN_SQUARE:
        for (i = 0; i < N; i++)
        {
          if(!isnan(pos[i].x) & !isnan(pos[i].y))
            _draw_marker_square (cr, pos[i], marker_size, FALSE);
        }
        break;
        case B_MARKER_DIAMOND:
        for (i = 0; i < N; i++)
        {
          if(!isnan(pos[i].x) & !isnan(pos[i].y))
            _draw_marker_diamond (cr, pos[i], marker_size, TRUE);
        }
        break;
        case B_MARKER_OPEN_DIAMOND:
        for (i = 0; i < N; i++)
        {
          if(!isnan(pos[i].x) & !isnan(pos[i].y))
            _draw_marker_diamond (cr, pos[i], marker_size, FALSE);
        }
        break;
        case B_MARKER_X:
        for (i = 0; i < N; i++)
        {
          if(!isnan(pos[i].x) & !isnan(pos[i].y))
            _draw_marker_x (cr, pos[i], marker_size);
        }
        break;
        case B_MARKER_PLUS:
        for (i = 0; i < N; i++)
        {
          if(!isnan(pos[i].x) & !isnan(pos[i].y))
            _draw_marker_plus (cr, pos[i], marker_size);
        }
        break;
        default:
        break;
      }
    }
  g_free(pos);
  g_free(buffer);

#if PROFILE
  gint64 now = g_get_real_time();
  gint64 then = y_data_get_timestamp(B_DATA(xdata));
  te = g_timer_elapsed (t, NULL);
  g_message ("scatter view draw %d points: %f ms", N, te * 1000);
  g_message ("microseconds: %d",(int) (now-then));
  g_timer_destroy (t);
#endif
  g_clear_object(&xdata);
  g_clear_object(&ydata);
}

static void
scatter_view_measure (GtkWidget      *widget,
         GtkOrientation  orientation,
         int             for_size,
         int            *minimum_size,
         int            *natural_size,
         int            *minimum_baseline,
         int            *natural_baseline)
{
  *minimum_size=1;
  *natural_size=20;
  //*minimum_baseline=1;
  //*natural_baseline=20;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
scatter_view_draw (GtkWidget * w, cairo_t * cr)
{
  BScatterLineView *scat = B_SCATTER_LINE_VIEW (w);

  struct draw_struct *s = g_malloc (sizeof (struct draw_struct));
  s->scat = scat;
  s->cr = cr;

  g_list_foreach (scat->series, series_draw, s);

  g_free (s);

  /* draw cursors */
  if(scat->show_cursors && !isnan(scat->v_cursor))
  {
    BViewInterval *vi_x =
      b_element_view_cartesian_get_view_interval (B_ELEMENT_VIEW_CARTESIAN
              (w),
              X_AXIS);

    BPoint pstart, pend;

    pstart.x = b_view_interval_conv (vi_x, scat->v_cursor);
    pstart.y = 0.0;

    pend.x = pstart.x;
    pend.y = 1.0;

    _view_conv(w,&pstart,&pstart);
    _view_conv(w,&pend,&pend);

    cairo_save(cr);

    cairo_set_line_width (cr, scat->cursor_width);

    cairo_set_source_rgba (cr, scat->cursor_color.red, scat->cursor_color.green,
         scat->cursor_color.blue, scat->cursor_color.alpha);

    cairo_move_to (cr, pstart.x, pstart.y);
    cairo_line_to (cr, pend.x, pend.y);
    cairo_stroke (cr);

    cairo_restore(cr);
  }

  if(scat->show_cursors && !isnan(scat->h_cursor))
  {
    BViewInterval *vi_y =
      b_element_view_cartesian_get_view_interval (B_ELEMENT_VIEW_CARTESIAN
              (w),
              Y_AXIS);

    BPoint pstart, pend;

    pstart.y = b_view_interval_conv (vi_y, scat->h_cursor);
    pstart.x = 0.0;

    pend.y = pstart.y;
    pend.x = 1.0;

    _view_conv(w,&pstart,&pstart);
    _view_conv(w,&pend,&pend);

    cairo_save(cr);

    cairo_set_line_width (cr, scat->cursor_width);

    cairo_set_source_rgba (cr, scat->cursor_color.red, scat->cursor_color.green,
         scat->cursor_color.blue, scat->cursor_color.alpha);

    cairo_move_to (cr, pstart.x, pstart.y);
    cairo_line_to (cr, pend.x, pend.y);
    cairo_stroke (cr);

    cairo_restore(cr);
  }

  if (scat->zoom_in_progress)
    {
      BViewInterval *vi_x =
        b_element_view_cartesian_get_view_interval (B_ELEMENT_VIEW_CARTESIAN
						    (w),
						    X_AXIS);

      BViewInterval *vi_y =
        b_element_view_cartesian_get_view_interval (B_ELEMENT_VIEW_CARTESIAN
						    (w),
						    Y_AXIS);

      BPoint pstart, pend;

      pstart.x = b_view_interval_conv (vi_x, scat->op_start.x);
      pend.x = b_view_interval_conv (vi_x, scat->cursor_pos.x);
      pstart.y = b_view_interval_conv (vi_y, scat->op_start.y);
      pend.y = b_view_interval_conv (vi_y, scat->cursor_pos.y);

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

static void
b_scatter_view_snapshot (GtkWidget   *w,
                      GtkSnapshot *s)
{
  graphene_rect_t bounds;
  if (gtk_widget_compute_bounds(w,w,&bounds)) {
    cairo_t *cr = gtk_snapshot_append_cairo (s, &bounds);
    scatter_view_draw(w, cr);
  }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
on_data_changed (BData * data, gpointer user_data)
{
  BElementView *mev = (BElementView *) user_data;
  g_return_if_fail (mev!=NULL);
  b_element_view_changed (mev);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
on_series_notify (GObject    *gobject,
               GParamSpec *pspec,
               gpointer    user_data)
{
  BElementView *v = (BElementView *) user_data;
  if(!strcmp("x-data",pspec->name) || !strcmp("y-data",pspec->name) || !strcmp("x-err",pspec->name) || !strcmp("y-err",pspec->name)) {
    BVector *data;
    g_object_get(gobject,pspec->name,&data,NULL);
    if (data != NULL)
    {
      g_signal_connect_after (data, "changed", G_CALLBACK (on_data_changed),
			      v);
    }
    g_clear_object(&data);
  }
  b_element_view_changed(v);
}

/**
 * b_scatter_line_view_add_series:
 * @v: a #BScatterLineView
 * @s: (transfer full): a #BScatterSeries
 *
 * Add a series to the plot @v.
 **/
void
b_scatter_line_view_add_series (BScatterLineView * v, BScatterSeries * s)
{
  v->series = g_list_append (v->series, g_object_ref_sink(s));

  g_signal_connect(s,"notify",G_CALLBACK(on_series_notify),v);

  /* connect changed signals */
  BVector *xdata, *ydata;
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
  g_clear_object(&xdata);
  g_clear_object(&ydata);

  BElementViewCartesian *cart = (BElementViewCartesian *) v;
  BViewInterval *vix =
    b_element_view_cartesian_get_view_interval (cart, X_AXIS);
  BViewInterval *viy =
    b_element_view_cartesian_get_view_interval (cart, Y_AXIS);

  if (vix)
    b_view_interval_request_preferred_range (vix);
  if (viy)
    b_view_interval_request_preferred_range (viy);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
b_scatter_line_view_set_property (GObject * object,
                               guint property_id,
                               const GValue * value,
                               GParamSpec * pspec)
{
  BScatterLineView *self = (BScatterLineView *) object;

  switch (property_id)
    {
    case PROP_V_CURSOR_POS:
      {
        self->v_cursor = g_value_get_double (value);
        if(self->show_cursors) {
          b_element_view_changed(B_ELEMENT_VIEW(self));
        }
      }
      break;
    case PROP_H_CURSOR_POS:
      {
        self->h_cursor = g_value_get_double (value);
        if(self->show_cursors) {
          b_element_view_changed(B_ELEMENT_VIEW(self));
        }
      }
      break;
    case PROP_SHOW_CURSORS:
      {
        self->show_cursors = g_value_get_boolean (value);
        b_element_view_changed(B_ELEMENT_VIEW(self));
      }
      break;
    case PROP_CURSOR_COLOR:
      {
        GdkRGBA *c = g_value_get_pointer (value);
        self->cursor_color = *c;
        if(self->show_cursors) {
          b_element_view_changed(B_ELEMENT_VIEW(self));
        }
      }
      break;
    case PROP_CURSOR_WIDTH:
      {
        self->cursor_width = g_value_get_double (value);
        if(self->show_cursors) {
          b_element_view_changed(B_ELEMENT_VIEW(self));
        }
      }
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
b_scatter_line_view_get_property (GObject * object,
                               guint property_id,
                               GValue * value,
                               GParamSpec * pspec)
{
  BScatterLineView *self = (BScatterLineView *) object;
  switch (property_id)
    {
    case PROP_V_CURSOR_POS:
      {
        g_value_set_double (value, self->v_cursor);
      }
      break;
    case PROP_H_CURSOR_POS:
      {
        g_value_set_double (value, self->h_cursor);
      }
      break;
    case PROP_SHOW_CURSORS:
      {
        g_value_set_boolean (value, self->show_cursors);
      }
      break;
    case PROP_CURSOR_COLOR:
      {
        g_value_set_pointer (value, &self->cursor_color);
      }
      break;
    case PROP_CURSOR_WIDTH:
      {
        g_value_set_double (value, self->cursor_width);
      }
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
b_scatter_line_view_class_init (BScatterLineViewClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  BElementViewClass *view_class = B_ELEMENT_VIEW_CLASS (klass);
  BElementViewCartesianClass *cart_class =
    B_ELEMENT_VIEW_CARTESIAN_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = b_scatter_line_view_finalize;

  object_class->set_property = b_scatter_line_view_set_property;
  object_class->get_property = b_scatter_line_view_get_property;

  /* properties */
  g_object_class_install_property (object_class, PROP_V_CURSOR_POS,
				   g_param_spec_double ("v-cursor-pos",
							 "Vertical cursor position",
							 "Vertical cursor position in plot units",
               -DBL_MAX, DBL_MAX,
							0.0,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_H_CURSOR_POS,
				   g_param_spec_double ("h-cursor-pos",
							 "Horizontal cursor position",
							 "Horizontal cursor position in plot units",
               -DBL_MAX, DBL_MAX,
							0.0,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_SHOW_CURSORS,
				   g_param_spec_boolean ("show-cursors",
							 "Whether to show cursors",
							 "Whether to show cursors",
               FALSE,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_CURSOR_COLOR,
				   g_param_spec_pointer ("cursor-color",
							 "Cursor Color",
							 "The cursor color",
							 G_PARAM_READWRITE |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_CURSOR_WIDTH,
				   g_param_spec_double ("cursor-width",
							"Cursor Width",
							"The cursor width in pixels",
							0.0, 100.0,
							1.0,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  widget_class->snapshot = b_scatter_view_snapshot;
  widget_class->measure = scatter_view_measure;

  view_class->changed = changed;

  cart_class->preferred_range = preferred_range;
}

static void
b_scatter_line_view_init (BScatterLineView * obj)
{
  obj->cursor_color.alpha = 1.0;

  g_object_set (obj, "valign", GTK_ALIGN_FILL, "halign",
		GTK_ALIGN_FILL, NULL);

  GtkEventController *motion_controller = gtk_event_controller_motion_new();
  gtk_widget_add_controller(GTK_WIDGET(obj), motion_controller);

  g_signal_connect(motion_controller, "motion", G_CALLBACK(b_scatter_line_view_motion_notify_event), obj);

  GtkGesture *click_controller = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click_controller),1);
  gtk_widget_add_controller(GTK_WIDGET(obj), GTK_EVENT_CONTROLLER(click_controller));

  g_signal_connect(click_controller, "pressed", G_CALLBACK(b_scatter_line_view_press_event), obj);
  g_signal_connect(click_controller, "released", G_CALLBACK(b_scatter_line_view_release_event), obj);

  GtkGesture *click3_controller = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click3_controller),3);
  gtk_widget_add_controller(GTK_WIDGET(obj), GTK_EVENT_CONTROLLER(click3_controller));

  //g_signal_connect(click3_controller, "pressed", G_CALLBACK(b_scatter_line_do_popup), obj);

  GtkEventController *scroll_controller = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
  gtk_widget_add_controller(GTK_WIDGET(obj), GTK_EVENT_CONTROLLER(scroll_controller));

  g_signal_connect(scroll_controller, "scroll", G_CALLBACK(b_scatter_line_view_scroll_event), obj);

  b_element_view_cartesian_add_view_interval (B_ELEMENT_VIEW_CARTESIAN (obj),
       				      X_AXIS);
  b_element_view_cartesian_add_view_interval (B_ELEMENT_VIEW_CARTESIAN (obj),
       				      Y_AXIS);
}

/**
 * b_scatter_line_view_get_all_series:
 * @v: a #BScatterLineView
 *
 * Get the #GList containing all series.
 *
 * Returns: (transfer none) (element-type BScatterSeries): a #GList
 **/
GList *b_scatter_line_view_get_all_series(BScatterLineView *v)
{
	return v->series;
}

static
gint find_func (gconstpointer a,
                 gconstpointer b)
{
  GList *al = (GList*)a;
  BScatterSeries *ss = (BScatterSeries *) al->data;
  gchar *l;
  g_object_get(ss,"label",&l,NULL);
  return g_strcmp0(l,b);
}

/**
 * b_scatter_line_view_remove_series:
 * @v: a #BScatterLineView
 * @label: a label
 *
 * Remove the series with the label @label. Note that if more than one series
 * is carries @label, only the first one will be removed.
 **/
void b_scatter_line_view_remove_series(BScatterLineView *v, const gchar *label)
{
  GList *found = g_list_find_custom(v->series,g_strdup(label),find_func);
  if(found != NULL) {
    g_object_unref(found->data);
    v->series = g_list_remove(v->series,found);
  }
}

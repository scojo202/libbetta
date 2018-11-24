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

#include "y-scatter-line-view.h"
#include <math.h>
#include <y-data.h>

/**
 * SECTION: y-scatter-line-view
 * @short_description: View for a scatter plot.
 *
 * Controls for a scatter plot.
 */

static GObjectClass *parent_class = NULL;

#define PROFILE 0

enum {
  SCATTER_LINE_VIEW_XDATA = 1,
  SCATTER_LINE_VIEW_YDATA,
  SCATTER_LINE_VIEW_DRAW_LINE,
  SCATTER_LINE_VIEW_LINE_COLOR,
  SCATTER_LINE_VIEW_LINE_WIDTH,
  SCATTER_LINE_VIEW_LINE_DASHING,
  SCATTER_LINE_VIEW_DRAW_MARKERS,
  SCATTER_LINE_VIEW_MARKER,
  SCATTER_LINE_VIEW_MARKER_COLOR,
  SCATTER_LINE_VIEW_MARKER_SIZE,
};

struct _YScatterLineView {
    YElementViewCartesian base;
    GList *series;
};

G_DEFINE_TYPE (YScatterLineView, y_scatter_line_view, Y_TYPE_ELEMENT_VIEW_CARTESIAN);

static void
handlers_disconnect(gpointer data, gpointer user_data)
{
  YScatterSeries *series = Y_SCATTER_SERIES(data);
  YScatterLineView *v = Y_SCATTER_LINE_VIEW(user_data);

  YData *xdata = y_struct_get_data(Y_STRUCT(series),"x");

  if(xdata!=NULL) {
    g_signal_handlers_disconnect_by_data(xdata,v);
  }

  YData *ydata = y_struct_get_data(Y_STRUCT(series),"y");

  if(ydata!=NULL) {
    g_signal_handlers_disconnect_by_data(ydata,v);
  }
}

static void
y_scatter_line_view_finalize (GObject *obj)
{
  YScatterLineView *v = Y_SCATTER_LINE_VIEW(obj);
  g_list_foreach(v->series,handlers_disconnect,v);

  if (parent_class->finalize)
    parent_class->finalize (obj);
}

static gboolean
rectangle_contains_point (cairo_rectangle_t rect, Point * point) {
  return ((point->x > rect.x) && (point->x < rect.x+rect.width) && (point->y > rect.y) && (point->y < rect.y + rect.height));
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
changed (YElementView *gev)
{
  YElementViewCartesian *cart = Y_ELEMENT_VIEW_CARTESIAN(gev);

  y_element_view_cartesian_set_preferred_view (cart, X_AXIS);
  y_element_view_cartesian_set_preferred_view (cart, Y_AXIS);

  YViewInterval * vix = y_element_view_cartesian_get_view_interval (cart, X_AXIS);
  YViewInterval * viy = y_element_view_cartesian_get_view_interval (cart, Y_AXIS);

  if(vix)
    y_view_interval_request_preferred_range (vix);
  if(viy)
    y_view_interval_request_preferred_range (viy);

  if (Y_ELEMENT_VIEW_CLASS (parent_class)->changed)
    Y_ELEMENT_VIEW_CLASS (parent_class)->changed (gev);
}

static gboolean
y_scatter_line_view_scroll_event (GtkWidget *widget, GdkEventScroll *event)
{
  YElementViewCartesian *view = Y_ELEMENT_VIEW_CARTESIAN(widget);

    gboolean scroll = FALSE;
    gboolean direction;
  if(event->direction==GDK_SCROLL_UP) {
    scroll=TRUE;
    direction=TRUE;
  }
  else if(event->direction==GDK_SCROLL_DOWN) {
    scroll=TRUE;
    direction=FALSE;
  }
  if(!scroll) return FALSE;

  YViewInterval *viy = y_element_view_cartesian_get_view_interval (view,
						       Y_AXIS);
  YViewInterval *vix = y_element_view_cartesian_get_view_interval (view,
						       X_AXIS);

  double scale = direction ? 0.8 : 1.0/0.8;

  /* find the cursor position */

  Point ip;
  Point *evp = (Point *) &(event->x);

  view_invconv(widget,evp,&ip);
  y_view_interval_rescale_around_point(vix,y_view_interval_unconv_fn(vix,ip.x),scale);
  y_view_interval_rescale_around_point(viy,y_view_interval_unconv_fn(viy,ip.y),scale);

  return FALSE;
}

static void
do_popup_menu (GtkWidget *my_widget, GdkEventButton *event)
{
  YElementViewCartesian *view = Y_ELEMENT_VIEW_CARTESIAN(my_widget);

  GtkWidget *menu;

  menu = gtk_menu_new ();

  GtkWidget *autoscale_x = create_autoscale_menu_check_item ("Autoscale X axis", view, X_AXIS);
  gtk_widget_show(autoscale_x);
  GtkWidget *autoscale_y = create_autoscale_menu_check_item ("Autoscale Y axis", view, Y_AXIS);
  gtk_widget_show(autoscale_y);

  gtk_menu_shell_append(GTK_MENU_SHELL(menu),autoscale_x);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),autoscale_y);

  gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}

static gboolean
y_scatter_line_view_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
  YElementViewCartesian *view = Y_ELEMENT_VIEW_CARTESIAN(widget);

  /* Ignore double-clicks and triple-clicks */
  if (gdk_event_triggers_context_menu ((GdkEvent *) event) &&
      event->type == GDK_BUTTON_PRESS)
  {
    do_popup_menu (widget, event);
    return TRUE;
  }

  if(event->button == 1 && (event->state & GDK_SHIFT_MASK)) {
    YViewInterval *viy = y_element_view_cartesian_get_view_interval (view,
						       Y_AXIS);
    YViewInterval *vix = y_element_view_cartesian_get_view_interval (view,
						       X_AXIS);

    Point ip;
    Point *evp = (Point *) &(event->x);

    view_invconv(widget,evp,&ip);

    y_view_interval_recenter_around_point(vix,y_view_interval_unconv_fn(vix,ip.x));
    y_view_interval_recenter_around_point(viy,y_view_interval_unconv_fn(viy,ip.y));
  }

  return FALSE;
}

/* calculate the interval that just fits the data inside, with a little extra margin, and put it in a and b

return TRUE ("valid") if:
The min and max calculated actually correspond to "valid" points in the sequence, in the sense that negative numbers are bad on a logarithmic interval.

*/
static gboolean
valid_range (YViewInterval *vi, YVector *data, double *a, double *b)
{
  gint i, i0, i1;
  double min, max, w;
  gboolean first_min = TRUE, first_max = TRUE;

  if(y_vector_get_len (data)==0) {
    *a=0.0;
    *b=1.0;
    return TRUE;
  }

  y_vector_get_minmax (data, &min, &max);

  g_debug("seq range: %e %e\n",min, max);

  if (!(y_view_interval_valid (vi, min) && y_view_interval_valid (vi, max))) {
    i1 = y_vector_get_len (data)-1;
    i0 = 0;

    for (i = i0; i <= i1; ++i) {
      double x = y_vector_get_value (data, i);

      if (y_view_interval_valid (vi, x)) {
        if (first_min) {
	      min = x;
	      first_min = FALSE;
	    } else {
	      if (x < min)
	        min = x;
	    }

	    if (first_max) {
	      max = x;
	      first_max = FALSE;
	    } else {
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
  if (w==0)
    w=(min!=0 ? min: 1.0);
  min -= w * 0.025;
  max += w * 0.025;

  if (a)
    *a = min;
  if (b)
    *b = max;

  //g_message("VI range: %e %e\n",*a, *b);

  return TRUE;
}

static gboolean
preferred_range (YElementViewCartesian *cart, axis_t ax, double *a, double *b)
{
  YVector *seq = NULL;
  YScatterLineView *scat = Y_SCATTER_LINE_VIEW(cart);

  g_debug("scatter view preferred range");

  /* should loop over all series, come up with a range that works for all */
  GList *first = g_list_first(scat->series);
  if(first ==NULL)
    return FALSE;

  YScatterSeries *series = Y_SCATTER_SERIES(first->data);

  YVector *xdata, *ydata;
  xdata = Y_VECTOR(y_struct_get_data(Y_STRUCT(series),"x"));
  ydata = Y_VECTOR(y_struct_get_data(Y_STRUCT(series),"y"));

  if (ax == X_AXIS)
    seq=xdata;
  else if (ax == Y_AXIS)
    seq=ydata;
  else
    return FALSE;

  if (seq) {
    return valid_range (y_element_view_cartesian_get_view_interval (cart, ax), seq, a, b);
  }
  else if (ax == X_AXIS && ydata != NULL) {
    int n = y_vector_get_len(ydata);
    *a = 0.0;
    *b = (double) n;

    return TRUE;
  }

  return FALSE;
}

static void
get_preferred_size (GtkWidget *w, gint *minimum, gint *natural) {
  *minimum = 100;
  *natural = 2000;
}

struct draw_struct
{
  YScatterLineView *scat;
  cairo_t *cr;
};

static void
series_draw(gpointer data, gpointer user_data)
{
  YScatterSeries *series = Y_SCATTER_SERIES(data);
  struct draw_struct *s = user_data;
  YScatterLineView *scat = s->scat;
  cairo_t *cr = s->cr;
  GtkWidget *w = GTK_WIDGET(scat);

  YVector *xdata, *ydata;
  xdata = Y_VECTOR(y_struct_get_data(Y_STRUCT(series),"x"));
  ydata = Y_VECTOR(y_struct_get_data(Y_STRUCT(series),"y"));

  YViewInterval *vi_x, *vi_y;
  int i, N;

#if PROFILE
  GTimer *t = g_timer_new();
#endif

  g_debug("scatter view draw");

  if (ydata == NULL) {
    g_debug("data was null...");
    return;
  }

  vi_x = y_element_view_cartesian_get_view_interval (Y_ELEMENT_VIEW_CARTESIAN (w),
							 X_AXIS);

  vi_y = y_element_view_cartesian_get_view_interval (Y_ELEMENT_VIEW_CARTESIAN (w),
							 Y_AXIS);

  if(xdata==NULL) {
    N = y_vector_get_len(ydata);
  }
  else {
    N = MIN(y_vector_get_len (xdata), y_vector_get_len (ydata));
  }

  //g_message("length is %d %d",y_vector_get_len (xdata),y_vector_get_len (ydata));

  if(N<1) {
    return;
  }

  Point pos[N];
  double buffer[N];

  if(xdata!=NULL) {
    const double *xraw = y_vector_get_values(xdata);
    y_view_interval_conv_bulk(vi_x,xraw,buffer,N);

    for (i = 0; i < N; i++) {
      pos[i].x = buffer[i];
    }
  }
  else {
    for (i = 0; i < N; i++) {
      pos[i].x = y_view_interval_conv_fn(vi_x,(double) i);
    }
  }

  const double *yraw = y_vector_get_values(ydata);

  y_view_interval_conv_bulk(vi_y,yraw,buffer,N);
  for (i=0;i<N;i++) {
    pos[i].y = buffer[i];
  }

  view_conv_bulk (w, pos, pos, N);

  #if PROFILE
  double te = g_timer_elapsed(t,NULL);
  g_message("scatter view before draw: %f ms",N,te*1000);
#endif

  gboolean draw_line;
  double line_width;
  GdkRGBA *line_color;

  g_object_get(series, "draw-line", &draw_line, "line-width", &line_width, "line-color", &line_color, NULL);

  if (draw_line && N>1) {
    cairo_set_line_width (cr, line_width);
    //canvas_set_dashing (canvas, NULL, 0);

    cairo_set_source_rgba(cr,line_color->red,line_color->green,line_color->blue,line_color->alpha);

    cairo_move_to(cr,pos[0].x,pos[0].y);
    for(i=1;i<N;i++) {
      cairo_line_to(cr,pos[i].x,pos[i].y);
    }
    cairo_stroke(cr);
  }

  /*if (scat->draw_markers) {
    cairo_set_source_rgba (cr, scat->marker_color.red,scat->marker_color.green,scat->marker_color.blue,scat->marker_color.alpha);
    double radius = 3;
    cairo_arc(cr,0,0,radius,0,2*G_PI);
    cairo_close_path(cr);
    cairo_path_t *path = cairo_copy_path(cr);
    for(i=0;i<N;i++) {
      cairo_save(cr);
      //cairo_new_path(cr);
      cairo_translate(cr,pos[i].x,pos[i].y);
      cairo_append_path(cr,path);
      cairo_fill(cr);
      cairo_restore(cr);
    }
    cairo_path_destroy(path);
  }*/

  gboolean draw_markers;
  GdkRGBA *marker_color;

  g_object_get(series, "draw-markers", &draw_markers, "marker-color", &marker_color, NULL);

  if (draw_markers) {
    cairo_set_source_rgba (cr, marker_color->red,marker_color->green,marker_color->blue,marker_color->alpha);
    double radius = 3;

    for(i=0;i<N;i++) {
      cairo_arc(cr,pos[i].x,pos[i].y,radius,0,2*G_PI);
      cairo_fill(cr);
    }
  }

#if PROFILE
  te = g_timer_elapsed(t,NULL);
  g_message("scatter view draw %d points: %f ms",N,te*1000);
  g_timer_destroy(t);
#endif
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
y_scatter_line_view_draw (GtkWidget *w, cairo_t *cr)
{
  YScatterLineView *scat = Y_SCATTER_LINE_VIEW (w);

  struct draw_struct *s = g_malloc(sizeof(struct draw_struct));
  s->scat = scat;
  s->cr = cr;

  g_list_foreach(scat->series, series_draw, s);

  g_free(s);

  return TRUE;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static
void on_data_changed(YData *data, gpointer   user_data) {
  YElementView * mev = (YElementView *) user_data;
  g_assert(mev);
  y_element_view_changed (mev);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void y_scatter_line_view_add_series(YScatterLineView *v, YScatterSeries *s)
{
  v->series = g_list_append(v->series,s);
  /* connect changed signals */
  YVector *xdata, *ydata;
  xdata = Y_VECTOR(y_struct_get_data(Y_STRUCT(s),"x"));
  ydata = Y_VECTOR(y_struct_get_data(Y_STRUCT(s),"y"));

  /* TODO: connect to a "subdata changed" signal on series so that:
   - if x is set after series is added, we can connect to signals
   - if data is changed we connect and disconnect to signals
   */

  if(xdata!=NULL) {
    g_signal_connect_after(xdata,"changed",G_CALLBACK(on_data_changed),v);
  }
  if(ydata!=NULL) {
    g_signal_connect_after(ydata,"changed",G_CALLBACK(on_data_changed),v);
  }

	YElementViewCartesian *cart = (YElementViewCartesian *) v;
	YViewInterval * vix = y_element_view_cartesian_get_view_interval (cart, X_AXIS);
  YViewInterval * viy = y_element_view_cartesian_get_view_interval (cart, Y_AXIS);

  if(vix)
    y_view_interval_request_preferred_range (vix);
  if(viy)
    y_view_interval_request_preferred_range (viy);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
y_scatter_line_view_class_init (YScatterLineViewClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  YElementViewClass *view_class = Y_ELEMENT_VIEW_CLASS (klass);
  YElementViewCartesianClass *cart_class = Y_ELEMENT_VIEW_CARTESIAN_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = y_scatter_line_view_finalize;

  widget_class->get_preferred_width = get_preferred_size;
  widget_class->get_preferred_height = get_preferred_size;

  widget_class->scroll_event = y_scatter_line_view_scroll_event;
  widget_class->button_press_event = y_scatter_line_view_button_press_event;

  widget_class->draw = y_scatter_line_view_draw;

  view_class->changed   = changed;

  cart_class->preferred_range  = preferred_range;
}

static void
y_scatter_line_view_init (YScatterLineView * obj)
{
  g_object_set(obj,"expand",FALSE,"valign",GTK_ALIGN_START,"halign",GTK_ALIGN_START,NULL);

  gtk_widget_add_events(GTK_WIDGET(obj),GDK_SCROLL_MASK | GDK_BUTTON_PRESS_MASK);

  g_debug("y_scatter_line_view_init");
}

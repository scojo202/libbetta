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

#include "y-scatter-view.h"
#include <math.h>
#include <y-data.h>

static GObjectClass *parent_class = NULL;

#define PROFILE 0

enum {
  SCATTER_VIEW_XDATA = 1,
  SCATTER_VIEW_YDATA,
  SCATTER_VIEW_DRAW_LINE,
  SCATTER_VIEW_LINE_COLOR,
  SCATTER_VIEW_LINE_WIDTH,
  SCATTER_VIEW_LINE_DASHING,
  SCATTER_VIEW_DRAW_MARKERS,
  SCATTER_VIEW_MARKER,
  SCATTER_VIEW_MARKER_COLOR,
  SCATTER_VIEW_MARKER_SIZE,
};

typedef struct _YScatterViewPrivate YScatterViewPrivate;
struct _YScatterViewPrivate {
    YVector * xdata;
    YVector * ydata;
    gulong xdata_changed_id;
    gulong ydata_changed_id;
    GtkLabel * label;
    
    gboolean draw_line, draw_markers;
    GdkRGBA line_color, marker_color;
    double line_width, marker_size;
    //Marker marker;
};

static void
y_scatter_view_finalize (GObject *obj)
{
  if (parent_class->finalize)
    parent_class->finalize (obj);
}

gboolean
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
    
  /*if(gev->draw_pending)
    g_message("scat fail");
  else g_message("scat success");*/
  
  if (Y_ELEMENT_VIEW_CLASS (parent_class)->changed)
    Y_ELEMENT_VIEW_CLASS (parent_class)->changed (gev);
}

static gboolean
y_scatter_view_scroll_event (GtkWidget *widget, GdkEventScroll *event)
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
  int button, event_time;

  menu = gtk_menu_new ();

  GtkWidget *autoscale_x = create_autoscale_menu_check_item ("Autoscale X axis", view, X_AXIS);
  gtk_widget_show(autoscale_x);
  GtkWidget *autoscale_y = create_autoscale_menu_check_item ("Autoscale Y axis", view, Y_AXIS);
  gtk_widget_show(autoscale_y);

  gtk_menu_shell_append(GTK_MENU_SHELL(menu),autoscale_x);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu),autoscale_y);
  
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
y_scatter_view_button_press_event (GtkWidget *widget, GdkEventButton *event)
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
  YScatterView *scat = Y_SCATTER_VIEW(cart);
  
  if (ax == X_AXIS)
    seq=scat->priv->xdata;
  else if (ax == Y_AXIS)
    seq=scat->priv->ydata;
  else
    return FALSE;

  if (seq) {
    return valid_range (y_element_view_cartesian_get_view_interval (cart, ax), seq, a, b);
  }

  return FALSE;
}

static void
get_preferred_size (GtkWidget *w, gint *minimum, gint *natural) {
  *minimum = 100;
  *natural = 2000;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
y_scatter_view_draw (GtkWidget *w, cairo_t *cr)
{
  YScatterView *scat = Y_SCATTER_VIEW (w);
  YElementView *view = Y_ELEMENT_VIEW(w);
  YScatterViewPrivate *priv = scat->priv;
  YVector *xdata = Y_VECTOR(priv->xdata);
  YVector *ydata = Y_VECTOR(priv->ydata);
  YViewInterval *vi_x, *vi_y;
  int i, N;
  
#if PROFILE
  GTimer *t = g_timer_new();
#endif

  if (xdata == NULL || ydata == NULL) {
    g_warning("data was null...");
    return FALSE;
  }
  
  vi_x = y_element_view_cartesian_get_view_interval (Y_ELEMENT_VIEW_CARTESIAN (w),
							 X_AXIS);

  vi_y = y_element_view_cartesian_get_view_interval (Y_ELEMENT_VIEW_CARTESIAN (w),
							 Y_AXIS);

  N = MIN(y_vector_get_len (xdata), y_vector_get_len (ydata));
  
  //g_message("length is %d %d",y_data_vector_get_len (xdata),y_data_vector_get_len (ydata));

  if(N<1) {
    view->draw_pending = FALSE;
    return FALSE;
  }

  Point pos[N];

  const double *xraw = y_vector_get_values(xdata);
  const double *yraw = y_vector_get_values(ydata);
  
  double buffer[N];
  y_view_interval_conv_bulk(vi_x,xraw,buffer,N);
  
  for (i = 0; i < N; i++) {
    pos[i].x = buffer[i];
  }
  
  y_view_interval_conv_bulk(vi_y,yraw,buffer,N);
  for (i=0;i<N;i++) {
    pos[i].y = buffer[i];
  }
  
  view_conv_bulk (w, pos, pos, N);
  
  #if PROFILE
  double te = g_timer_elapsed(t,NULL);
  g_message("scatter view before draw: %f ms",N,te*1000);
#endif
  
  if (priv->draw_line && N>1) {
    cairo_set_line_width (cr, priv->line_width);
    //canvas_set_dashing (canvas, NULL, 0);
    
    cairo_set_source_rgba(cr,priv->line_color.red,priv->line_color.green,priv->line_color.blue,priv->line_color.alpha);

    cairo_move_to(cr,pos[0].x,pos[0].y);
    for(i=1;i<N;i++) {
      cairo_line_to(cr,pos[i].x,pos[i].y);
    }
    cairo_stroke(cr);
  }
  
  /*if (priv->draw_markers) {
    cairo_set_source_rgba (cr, priv->marker_color.red,priv->marker_color.green,priv->marker_color.blue,priv->marker_color.alpha);
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
  
  if (priv->draw_markers) {
    cairo_set_source_rgba (cr, priv->marker_color.red,priv->marker_color.green,priv->marker_color.blue,priv->marker_color.alpha);
    double radius = 3;
    
    for(i=0;i<N;i++) {
      cairo_arc(cr,pos[i].x,pos[i].y,radius,0,2*G_PI);
      cairo_fill(cr);
    }
  }

  view->draw_pending = FALSE;
    
#if PROFILE
  te = g_timer_elapsed(t,NULL);
  g_message("scatter view draw %d points: %f ms",N,te*1000);
  g_timer_destroy(t);
#endif
    
  return TRUE;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void y_scatter_view_set_label (YScatterView *view, GtkLabel *label)
{
  view->priv->label = label;
}

void y_scatter_view_set_line_color_from_string (YScatterView *view, gchar * colorstring)
{
  GdkRGBA c;
  gboolean success = gdk_rgba_parse (&c,colorstring);
  if(success)
    g_object_set (view, "line_color", &c, NULL);
  else
    g_warning("Failed to parse color string %s",colorstring);
}

void y_scatter_view_set_marker_color_from_string (YScatterView *view, gchar * colorstring)
{
  GdkRGBA c;
  gboolean success = gdk_rgba_parse (&c,colorstring);
  if(success)
    g_object_set (view, "marker_color", &c, NULL);
  else
    g_warning("Failed to parse color string %s",colorstring);
}

static
void on_data_changed(YData *data, gpointer   user_data) {
  YElementView * mev = (YElementView *) user_data;
  g_assert(mev);
  y_element_view_changed (mev);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
y_scatter_view_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
    YScatterView *self = (YScatterView *) object;
    g_debug("set_property: %d",property_id);
    
    switch (property_id) {
    case SCATTER_VIEW_XDATA: {
      if(self->priv->xdata) {
        g_signal_handler_disconnect(self->priv->xdata, self->priv->xdata_changed_id);
      //disconnect old data, if present
	g_object_unref(self->priv->xdata);
      }
      self->priv->xdata = g_value_dup_object (value);
      /* connect to changed signal */
      self->priv->xdata_changed_id = g_signal_connect_after(self->priv->xdata,"changed",G_CALLBACK(on_data_changed),self);
      break;
    }
    case SCATTER_VIEW_YDATA: {
      if(self->priv->ydata)
        g_signal_handler_disconnect(self->priv->ydata, self->priv->ydata_changed_id);
      //disconnect old data, if present
      self->priv->ydata = g_value_dup_object (value);
      /* connect to changed signal */
      self->priv->ydata_changed_id = g_signal_connect_after(self->priv->ydata,"changed",G_CALLBACK(on_data_changed),self);
      break;
    }
    case SCATTER_VIEW_DRAW_LINE: {
      self->priv->draw_line = g_value_get_boolean (value);
    }
      break;
    case SCATTER_VIEW_LINE_COLOR: {
      GdkRGBA * c = g_value_get_pointer (value);
      self->priv->line_color = *c;
    }
      break;
    case SCATTER_VIEW_LINE_WIDTH: {
      self->priv->line_width = g_value_get_double (value);
    }
      break;
    case SCATTER_VIEW_DRAW_MARKERS: {
      self->priv->draw_markers = g_value_get_boolean (value);
    }
      break;
    //case SCATTER_VIEW_MARKER: {
    //  self->priv->marker = g_value_get_int (value);
    //}
      //break;
    case SCATTER_VIEW_MARKER_COLOR: {
      GdkRGBA * c = g_value_get_pointer (value);
      self->priv->marker_color = *c;
    }
      break;
      case SCATTER_VIEW_MARKER_SIZE: {
      self->priv->marker_size = g_value_get_double (value);
    }
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
      break;
    }
  y_element_view_changed (Y_ELEMENT_VIEW(self));
}
                        
static void
y_scatter_view_get_property (GObject      *object,
                        guint         property_id,
                        GValue       *value,
                        GParamSpec   *pspec)
{
    YScatterView *self = (YScatterView *) object;
    switch (property_id) {
    case SCATTER_VIEW_XDATA: {
      g_value_set_object (value, self->priv->xdata);
    }
      break;
    case SCATTER_VIEW_YDATA: {
      g_value_set_object (value, self->priv->ydata);
    }
      break;
    case SCATTER_VIEW_DRAW_LINE: {
      g_value_set_boolean (value, self->priv->draw_line);
    }
      break;
      case SCATTER_VIEW_LINE_COLOR: {
      g_value_set_pointer (value, &self->priv->line_color);
    }
      break;
    case SCATTER_VIEW_LINE_WIDTH: {
      g_value_set_double (value, self->priv->line_width);
    }
      break;
    case SCATTER_VIEW_DRAW_MARKERS: {
      g_value_set_boolean (value, self->priv->draw_markers);
    }
      break;
      //case SCATTER_VIEW_MARKER: {
      //g_value_set_int (value, self->priv->marker);
    //}
      //break;
      case SCATTER_VIEW_MARKER_COLOR: {
      g_value_set_pointer (value, &self->priv->marker_color);
    }
      break;
      case SCATTER_VIEW_MARKER_SIZE: {
      g_value_set_double (value, self->priv->marker_size);
    }
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
      break;
    }
}


/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

#define DEFAULT_DRAW_LINE (TRUE)
#define DEFAULT_DRAW_MARKERS (FALSE)
#define DEFAULT_LINE_WIDTH 1.0
#define DEFAULT_MARKER_SIZE (1.0*72.0/64.0)

static void
y_scatter_view_class_init (YScatterViewClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  
  object_class->set_property = y_scatter_view_set_property;
  object_class->get_property = y_scatter_view_get_property;
  
  YElementViewClass *view_class = Y_ELEMENT_VIEW_CLASS (klass);
  YElementViewCartesianClass *cart_class = Y_ELEMENT_VIEW_CARTESIAN_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = y_scatter_view_finalize;
  
  widget_class->get_preferred_width = get_preferred_size;
  widget_class->get_preferred_height = get_preferred_size;
  
  widget_class->scroll_event = y_scatter_view_scroll_event;
  widget_class->button_press_event = y_scatter_view_button_press_event;
  
  widget_class->draw = y_scatter_view_draw;
  
  /* properties */
  
  g_object_class_install_property (object_class, SCATTER_VIEW_XDATA, 
                    g_param_spec_object ("xdata", "X Data", "X axis data", Y_TYPE_VECTOR, G_PARAM_READWRITE));
                                        
  g_object_class_install_property (object_class, SCATTER_VIEW_YDATA, 
                    g_param_spec_object ("ydata", "Y Data", "Y axis data",
                                        Y_TYPE_VECTOR, G_PARAM_READWRITE));
                                        
  g_object_class_install_property (object_class, SCATTER_VIEW_DRAW_LINE, 
                    g_param_spec_boolean ("draw_line", "Draw Line", "Whether to draw a line between points",
                                        DEFAULT_DRAW_LINE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  
  g_object_class_install_property (object_class, SCATTER_VIEW_LINE_COLOR, 
                    g_param_spec_pointer ("line_color", "Line Color", "The line color",
                                        G_PARAM_READWRITE));
                                        
  g_object_class_install_property (object_class, SCATTER_VIEW_LINE_WIDTH, 
                    g_param_spec_double ("line_width", "Line Width", "The line width in points",
                                        0.0, 100.0, DEFAULT_LINE_WIDTH, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  
  // dashing
  
  g_object_class_install_property (object_class, SCATTER_VIEW_LINE_DASHING,
                    g_param_spec_value_array ("line_dashing", "Line Dashing", "Array for dashing", g_param_spec_double("dash","","",0.0,100.0,1.0,G_PARAM_READWRITE), G_PARAM_READWRITE));
  
  // marker-related
  
  g_object_class_install_property (object_class, SCATTER_VIEW_DRAW_MARKERS, 
                    g_param_spec_boolean ("draw_markers", "Draw Markers", "Whether to draw markers at points",
                                        DEFAULT_DRAW_MARKERS, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
                                        
  /*g_object_class_install_property (object_class, SCATTER_VIEW_MARKER, 
                    g_param_spec_int ("marker", "Marker", "The marker",
                                        _MARKER_NONE, _MARKER_UNKNOWN, _MARKER_SQUARE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));*/
                                        
  g_object_class_install_property (object_class, SCATTER_VIEW_MARKER_COLOR, 
                    g_param_spec_pointer ("marker_color", "Marker Color", "The marker color",
                                        G_PARAM_READWRITE));
                                        
  g_object_class_install_property (object_class, SCATTER_VIEW_MARKER_SIZE, 
                    g_param_spec_double ("marker_size", "Marker Size", "The marker size in points",
                                        0.0, 100.0, DEFAULT_MARKER_SIZE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  view_class->changed   = changed;

  cart_class->preferred_range  = preferred_range;
}

static void
y_scatter_view_init (YScatterView * obj)
{
  obj->priv = g_new0 (YScatterViewPrivate, 1);
  
  obj->priv->line_color.alpha = 1.0;
  obj->priv->marker_color.alpha = 1.0;
    
  g_object_set(obj,"expand",FALSE,"valign",GTK_ALIGN_START,"halign",GTK_ALIGN_START,NULL);
  
  gtk_widget_add_events(GTK_WIDGET(obj),GDK_SCROLL_MASK | GDK_BUTTON_PRESS_MASK);
  
  g_debug("y_scatter_view_init");
}

G_DEFINE_TYPE (YScatterView, y_scatter_view, Y_TYPE_ELEMENT_VIEW_CARTESIAN);


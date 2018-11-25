/*
 * y-axis-view.c
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

/* TODO
 * explore using popovers to set view interval!
 */

#include <math.h>
#include "y-axis-view.h"
#include "y-axis-markers.h"

/**
 * SECTION: y-axis-view
 * @short_description: Widget for displaying a linear or logarithmic axis.
 *
 * This widget is used to display axes along the edges of a #YScatterView or #YDensityPlot.
 *
 */

#define PROFILE 0

static GObjectClass *parent_class = NULL;

enum {
  AXIS_VIEW_DRAW_EDGE = 1,
  AXIS_VIEW_EDGE_THICKNESS,
  AXIS_VIEW_POSITION,
  AXIS_VIEW_DRAW_LABEL,
  AXIS_VIEW_LABEL_OFFSET,
  AXIS_VIEW_AXIS_LABEL,
  AXIS_VIEW_SHOW_MAJOR_TICKS,
  AXIS_VIEW_MAJOR_TICK_THICKNESS,
  AXIS_VIEW_MAJOR_TICK_LENGTH,
  AXIS_VIEW_SHOW_MAJOR_LABELS,
  AXIS_VIEW_SHOW_MINOR_TICKS,
  AXIS_VIEW_MINOR_TICK_THICKNESS,
  AXIS_VIEW_MINOR_TICK_LENGTH,
};

struct _YAxisView {
    YElementViewCartesian base;
    compass_t pos;
    gboolean draw_edge, draw_label, show_major_ticks, show_minor_ticks, show_major_labels;
    double label_offset, edge_thickness, major_tick_thickness, major_tick_length, minor_tick_thickness, minor_tick_length;
    gchar * axis_label;
    PangoFontDescription *label_font;
		double zoom_start;
		double cursor_pos;
		gboolean zoom_in_progress;
};

G_DEFINE_TYPE (YAxisView, y_axis_view, Y_TYPE_ELEMENT_VIEW_CARTESIAN);

static
gboolean get_horizontal (YAxisView *y_axis_view)
{
  gboolean horizontal = FALSE;

  switch (y_axis_view->pos) {
  case NORTH:
  case SOUTH:
    horizontal = TRUE;
    break;

  case EAST:
  case WEST:
    horizontal = FALSE;
    break;
  case COMPASS_INVALID:
    g_assert_not_reached();
    break;
  }

  return horizontal;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static
void
y_axis_view_tick_properties (YAxisView *view,
				  const YTick *tick,
				  gboolean *show_tick,
				  guint32 *color,
				  double *thickness,
				  double *length,
				  gboolean *show_label,
				  double *label_offset,
				  guint32 *label_color,
				  PangoFontDescription **label_font)
{
  g_return_if_fail (Y_IS_AXIS_VIEW (view));

  if (show_tick)
    *show_tick = FALSE;

  if (show_label)
    *show_label = FALSE;

  g_return_if_fail (tick != NULL);

  if (label_offset)
    *label_offset = view->label_offset;

  switch (y_tick_type (tick)) {

  case Y_TICK_NONE:

    if (color)
      *color = 0;
    if (thickness)
      *thickness = 0;
    if (length)
      *length = 0;

    /*show_label = view->show_label;*/
    g_object_get (view,
			     "show_lone_labels",        show_label,
			     //"extra_lone_label_offset", label_offset,
			     "lone_label_color",        label_color,
			     "lone_label_font",         label_font,
			     NULL);
    break;

  case Y_TICK_MAJOR:
  case Y_TICK_MAJOR_RULE:

    *show_tick = view->show_major_ticks;
    *thickness = view->major_tick_thickness;
    *length = view->major_tick_length;
    *show_label = view->show_major_labels;
    /*g_object_get (view,
			     "show_major_ticks",     show_tick,
			     //"major_tick_color",     color,
			     "major_tick_thickness", thickness,
			     "major_tick_length",    length,
			     "show_major_labels",    show_label,
			     //"major_label_color",    label_color,
			     //"major_label_font",     label_font,
			     NULL);*/
    break;

  case Y_TICK_MINOR:
  case Y_TICK_MINOR_RULE:

    *show_tick = view->show_minor_ticks;
    /*g_object_get (G_OBJECT (view),
			     "show_minor_ticks",     show_tick,
			     //"minor_tick_color",     color,
			     //"minor_tick_thickness", thickness,
			     //"minor_tick_length",    length,
			     //"show_minor_labels",    show_label,
			     //"minor_label_color",    label_color,
			     //"minor_label_font",     label_font,
			     NULL);*/
    break;

  case Y_TICK_MICRO:
  case Y_TICK_MICRO_RULE:

    /*g_object_get (G_OBJECT (view),
			     "show_micro_ticks",     show_tick,
			     "micro_tick_color",     color,
			     "micro_tick_thickness", thickness,
			     "micro_tick_length",    length,
			     "show_micro_labels",    show_label,
			     "micro_label_color",    label_color,
			     "micro_label_font",     label_font,
			     NULL);*/
    break;

  default:

    g_assert_not_reached ();

  }
}

static int
compute_axis_size_request (YAxisView *y_axis_view)
{
  YAxisMarkers *am;
  g_return_val_if_fail (Y_IS_AXIS_VIEW (y_axis_view),0);

  #if PROFILE
  GTimer *t = g_timer_new();
#endif

  g_debug ("compute axis size request");

  gboolean horizontal = TRUE;
  double edge_thickness = 0, legend_offset = 0;
  gchar *legend;
  int w = 0, h = 0;
  gint i;

  horizontal = get_horizontal(y_axis_view);

  legend = y_axis_view->axis_label;

  am = y_element_view_cartesian_get_axis_markers ((YElementViewCartesian *) y_axis_view,
						      META_AXIS);

  /* Account for the size of the axis labels */

  PangoContext *context = NULL;
  PangoLayout *layout = NULL;

  context = gdk_pango_context_get ();
  layout = pango_layout_new (context);

  pango_layout_set_font_description (layout, y_axis_view->label_font);

  for (i = am ? y_axis_markers_size (am) - 1 : -1; i >= 0; --i) {

    const YTick *tick;
    gboolean show_tick, show_label;
    double length, label_offset, thickness;
    int tick_w = 0, tick_h = 0;

    tick = y_axis_markers_get (am, i);

    y_axis_view_tick_properties (y_axis_view,
				      tick,
				      &show_tick,
				      NULL,
				      &thickness,
				      &length,
				      &show_label,
				      &label_offset,
				      NULL,
				      NULL);

    if (show_label && y_tick_is_labelled (tick)) {
      pango_layout_set_text (layout, y_tick_label (tick), -1);

      pango_layout_get_pixel_size (layout, &tick_w, &tick_h);

      if (horizontal)
	      tick_h += label_offset;
      else
	      tick_w += label_offset;
    }

    if (show_tick) {
      if (horizontal)
	    tick_h += length;
      else
	    tick_w += length;
    }

    if (tick_w > w)
      w = tick_w;

    if (tick_h > h)
      h = tick_h;
  }

  /* Account for the edge thickness */

  if (y_axis_view->draw_edge) {
    if (horizontal)
      h += edge_thickness;
    else
      w += edge_thickness;
  }

  /* Account for the height of the legend */

  if (legend && *legend) {
    int legend_h = 0;
    pango_layout_set_text (layout, legend, -1);

    pango_layout_get_pixel_size (layout, NULL, &legend_h);

    if (horizontal)
      h += legend_h + legend_offset;
    else
      w += legend_h + legend_offset;
  }

  g_object_unref (layout);
  g_object_unref (context);

  w+=5;
  h+=5;

  #if PROFILE
  double te = g_timer_elapsed(t,NULL);
  g_message("axis view compute size %d: %f ms",y_axis_view->pos,te*1000);
  g_timer_destroy(t);
#endif

  if (horizontal)
    return h;
  else return w;
}

static void
get_preferred_width (GtkWidget *w, gint *minimum, gint *natural) {
  YAxisView *a = Y_AXIS_VIEW(w);
  *minimum = 1;
  if(a->pos==NORTH || a->pos==SOUTH) {
      *natural = 20;
  }
  else {
    *natural = compute_axis_size_request(a);
    g_debug("axis: requesting width %d",*natural);
  }

}

static void
get_preferred_height (GtkWidget *w, gint *minimum, gint *natural) {
  YAxisView *a = Y_AXIS_VIEW(w);
  *minimum = 1;
  if(a->pos==EAST || a->pos==WEST) {
      *natural = 20;
  }
  else {
    *natural = compute_axis_size_request(a);
    g_debug("axis: requesting height %d",*natural);
  }

}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
changed (YElementView *view)
{
  YAxisView *a = Y_AXIS_VIEW(view);
  /* don't let this run before the position is set */
  if(a->pos==COMPASS_INVALID)
    return;
  g_debug("SIGNAL: axis view changed");
  gint thickness = compute_axis_size_request ((YAxisView *) view);
  int current_thickness;
  if(a->pos==EAST || a->pos==WEST) {
    current_thickness = gtk_widget_get_allocated_width(GTK_WIDGET(view));
  }
  else {
    current_thickness = gtk_widget_get_allocated_height(GTK_WIDGET(view));
  }
  if(thickness != current_thickness)
    gtk_widget_queue_resize(GTK_WIDGET(view));

  if (Y_ELEMENT_VIEW_CLASS (parent_class)->changed)
    Y_ELEMENT_VIEW_CLASS (parent_class)->changed (view);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
y_axis_view_draw (GtkWidget *w, cairo_t *cr)
{
  YElementView *view = Y_ELEMENT_VIEW(w);

  YAxisMarkers *am;
  YViewInterval *vi;
  gboolean horizontal = TRUE;
  //double edge_thickness = 1, legend_offset;
  //guint32 edge_color;
  //int width;
  gchar *legend;
  Point pt1, pt2, pt3;
  gint i;

  #if PROFILE
  GTimer *t = g_timer_new();
#endif

  YAxisView * y_axis_view = Y_AXIS_VIEW (view);

  horizontal = get_horizontal(y_axis_view);

  /*cairo_move_to(cr, 0,0);
  cairo_line_to(cr,0,view->alloc_height);
  cairo_line_to(cr,view->alloc_width,view->alloc_height);
  cairo_line_to(cr,view->alloc_width,0);
  cairo_line_to(cr,0,0);
  cairo_stroke(cr);*/

  vi = y_element_view_cartesian_get_view_interval ((YElementViewCartesian *) view,
						       META_AXIS);

  /* Render the edge */

  if (y_axis_view->draw_edge) {
    switch (y_axis_view->pos) {

    case NORTH:
      pt1.x = 0;
      pt1.y = 0;
      pt2.x = 1;
      pt2.y = 0;
      break;

    case SOUTH:
      pt1.x = 0;
      pt1.y = 1;
      pt2.x = 1;
      pt2.y = 1;
      break;

    case EAST:
      pt1.x = 0;
      pt1.y = 0;
      pt2.x = 0;
      pt2.y = 1;
      break;

    case WEST:
      pt1.x = 1;
      pt1.y = 0;
      pt2.x = 1;
      pt2.y = 1;
      break;

    default:
      g_assert_not_reached ();
    }

    view_conv (w, &pt1, &pt1);
    view_conv (w, &pt2, &pt2);

    cairo_set_line_width(cr, y_axis_view->edge_thickness);

    cairo_move_to(cr, pt1.x,pt1.y);
    cairo_line_to(cr, pt2.x,pt2.y);
    cairo_stroke(cr);

    //y_canvas_set_edge_color_doubles (canvas, 0,0,0);
    //y_canvas_set_dashing (canvas, NULL, 0);
  }

  /* Render our markers */

  am = y_element_view_cartesian_get_axis_markers ((YElementViewCartesian *) view,
						      META_AXIS);

  double tick_length = 0;
  double max_offset = 0;

  PangoContext *context = NULL;
  PangoLayout *layout = NULL;

  context = gdk_pango_context_get ();
  layout = pango_layout_new (context);

  pango_layout_set_font_description (layout, y_axis_view->label_font);

  for (i = am ? y_axis_markers_size (am) - 1 : -1; i >= 0; --i) {
    const YTick *tick;
    gboolean show_tick, show_label;
    guint32 tick_color, label_color;
    double t, length, thickness, label_offset;
    PangoFontDescription *label_font;
    Anchor anchor;

    tick = y_axis_markers_get (am, i);

    y_axis_view_tick_properties (Y_AXIS_VIEW(view),
				      tick,
				      &show_tick,
				      &tick_color,
				      &thickness,
				      &length,
				      &show_label,
				      &label_offset,
				      &label_color,
				      &label_font);

    t = y_tick_position (tick);
    t = y_view_interval_conv_fn (vi, t);

    switch (y_axis_view->pos) {
    case NORTH:
      pt1.x = t;
      pt1.y = 0;
      view_conv (w, &pt1, &pt1);

      pt2 = pt1;
      pt2.y -= length;

      pt3 = pt2;
      pt3.y -= label_offset;

      anchor = ANCHOR_BOTTOM;
      break;

    case SOUTH:
      pt1.x = t;
      pt1.y = 1;
      view_conv (w, &pt1, &pt1);

      pt2 = pt1;
      pt2.y += length;

      pt3 = pt2;
      pt3.y += label_offset;

      anchor = ANCHOR_TOP;
      break;

    case EAST:
      pt1.x = 0;
      pt1.y = t;
      view_conv (w, &pt1, &pt1);

      pt2 = pt1;
      pt2.x += length;

      pt3 = pt2;
      pt3.x += label_offset;

      anchor = ANCHOR_LEFT;
      break;

    case WEST:
      pt1.x = 1;
      pt1.y = t;
      view_conv (w, &pt1, &pt1);

      pt2 = pt1;
      pt2.x -= length;

      pt3 = pt2;
      pt3.x -= label_offset;

      anchor = ANCHOR_RIGHT;
      break;

    default:
      g_assert_not_reached ();

    }

    if (show_tick) {
      cairo_set_line_width (cr, y_axis_view->major_tick_thickness);
      //y_canvas_set_dashing (canvas, NULL, 0);
      cairo_move_to(cr, pt1.x,pt1.y);
      cairo_line_to(cr, pt2.x,pt2.y);
      cairo_stroke(cr);
      tick_length = MAX(tick_length,length);
    }

    if(y_tick_is_labelled (tick) && show_label) {
      int dw,dh;

      pango_layout_set_text (layout, y_tick_label (tick), -1);

      pango_layout_get_pixel_size (layout, &dw, &dh);

      gboolean over_edge = FALSE;

      if(horizontal) {
        if(pt3.x-dw/2 < 0)
          over_edge = TRUE;
        if(pt3.x+dw/2 > gtk_widget_get_allocated_width(w))
          over_edge = TRUE;
      }
      else {
        if(pt3.y-dh/2 < 0)
          over_edge = TRUE;
        if(pt3.y+dh/2 > gtk_widget_get_allocated_height(w))
          over_edge = TRUE;
      }

      if (!over_edge) {
        string_draw_no_rotate (cr, pt3,	anchor, layout);

	      if (horizontal) {
            if (dh > max_offset) {
                max_offset = dh+label_offset;
            }
        }
        else {
            if (dw > max_offset) {
                max_offset = dw+label_offset;
            }
        }
      }
    }
  }

  g_object_unref (layout);
  g_object_unref (context);

  legend = y_axis_view->axis_label;

  if (legend && *legend) {
    switch (y_axis_view->pos) {
    case NORTH:
      pt1.x = 0.5;
      pt1.y = 0;
      view_conv (w, &pt1, &pt1);
      pt1.y-=(max_offset+tick_length);
      string_draw (cr, y_axis_view->label_font, pt1, ANCHOR_BOTTOM, ROT_0, legend);
      break;
    case SOUTH:
      pt1.x = 0.5;
      pt1.y = 1;
      view_conv (w, &pt1, &pt1);
      pt1.y+=(max_offset+tick_length);
      string_draw (cr, y_axis_view->label_font, pt1, ANCHOR_TOP, ROT_0, legend);
      break;
    case EAST:
      pt1.x = 0;
      pt1.y = 0.5;
      view_conv (w, &pt1, &pt1);
      pt1.x+=(max_offset+tick_length);
      string_draw (cr, y_axis_view->label_font, pt1, ANCHOR_BOTTOM, ROT_270, legend);
      break;
    case WEST:
      pt1.x = 1;
      pt1.y = 0.5;
      view_conv (w, &pt1, &pt1);
      pt1.x-=(max_offset+tick_length);
      string_draw (cr, y_axis_view->label_font, pt1, ANCHOR_BOTTOM, ROT_90, legend);
      break;
    default:
      g_assert_not_reached ();
    }
  }

	/* draw zoom thing */
	if(y_axis_view->zoom_in_progress)
	{
		double z = y_view_interval_conv_fn (vi, y_axis_view->zoom_start);
		double e = y_view_interval_conv_fn (vi, y_axis_view->cursor_pos);

		switch (y_axis_view->pos) {

    case NORTH:
      pt1.x = z;
      pt1.y = 0;
      pt2.x = e;
      pt2.y = 0;
      break;

    case SOUTH:
      pt1.x = z;
      pt1.y = 1;
      pt2.x = e;
      pt2.y = 1;
      break;

    case EAST:
      pt1.x = 0;
      pt1.y = z;
      pt2.x = 0;
      pt2.y = e;
      break;

    case WEST:
      pt1.x = 1;
      pt1.y = z;
      pt2.x = 1;
      pt2.y = e;
      break;

    default:
      g_assert_not_reached ();
    }

		view_conv (w, &pt1, &pt1);
		view_conv (w, &pt2, &pt2);

		cairo_set_line_width(cr, y_axis_view->edge_thickness);
		cairo_set_source_rgba(cr,0.0,0.0,1.0,0.25);

    cairo_move_to(cr, pt1.x,pt1.y);
    cairo_line_to(cr, pt2.x,pt2.y);

		Point pt0 = pt1;

		switch (y_axis_view->pos) {

    case NORTH:
      pt1.x = z;
      pt1.y = 1;
      pt2.x = e;
      pt2.y = 1;
      break;

    case SOUTH:
      pt1.x = z;
      pt1.y = 0;
      pt2.x = e;
      pt2.y = 0;
      break;

    case EAST:
      pt1.x = 1;
      pt1.y = z;
      pt2.x = 1;
      pt2.y = e;
      break;

    case WEST:
      pt1.x = 0;
      pt1.y = z;
      pt2.x = 0;
      pt2.y = e;
      break;

    default:
      g_assert_not_reached ();
    }

		view_conv (w, &pt1, &pt1);
		view_conv (w, &pt2, &pt2);

		cairo_line_to(cr, pt2.x,pt2.y);
    cairo_line_to(cr, pt1.x,pt1.y);
		cairo_line_to(cr, pt0.x,pt0.y);

    cairo_fill(cr);
	}

#if PROFILE
  double te = g_timer_elapsed(t,NULL);
  g_message("axis view draw %d: %f ms",y_axis_view->pos,te*1000);
  g_timer_destroy(t);
#endif

  return TRUE;
}

static gboolean
y_axis_view_scroll_event (GtkWidget *widget, GdkEventScroll *event)
{
  YAxisView *view = (YAxisView *) widget;

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

  YViewInterval *vi = y_element_view_cartesian_get_view_interval ((YElementViewCartesian *) view,
						       META_AXIS);

  double scale = direction ? 0.8 : 1.0/0.8;

  /* find the cursor position */

  Point ip;
  Point *evp = (Point *) &(event->x);

  view_invconv(widget,evp,&ip);

  gboolean horizontal = get_horizontal(view);

  double z = horizontal ? ip.x : ip.y;

  y_view_interval_rescale_around_point(vi,y_view_interval_unconv_fn(vi,z),scale);

  return FALSE;
}

static void
y_axis_view_do_popup_menu (GtkWidget *my_widget, GdkEventButton *event)
{
  YAxisView *view = (YAxisView *) my_widget;

  GtkWidget *menu;

  menu = gtk_menu_new ();
  //g_signal_connect (menu, "deactivate",
  //                  G_CALLBACK (gtk_widget_destroy), NULL);

  GtkWidget *autoscale = create_autoscale_menu_check_item ("Autoscale axis", (YElementViewCartesian *) view, META_AXIS);
  gtk_widget_show(autoscale);

  gtk_menu_shell_append(GTK_MENU_SHELL(menu),autoscale);

  gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}

static gboolean
y_axis_view_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
  YAxisView *view = (YAxisView *) widget;
/* Ignore double-clicks and triple-clicks */
  if (gdk_event_triggers_context_menu ((GdkEvent *) event) &&
      event->type == GDK_BUTTON_PRESS)
  {
    y_axis_view_do_popup_menu (widget, event);
    return TRUE;
  }

	if(y_element_view_get_zooming(Y_ELEMENT_VIEW(view)) && event->button == 1) {
		YViewInterval *vi = y_element_view_cartesian_get_view_interval ((YElementViewCartesian *) view,
						       META_AXIS);
    Point ip;
    Point *evp = (Point *) &(event->x);

    view_invconv(widget,evp,&ip);

    gboolean horizontal = get_horizontal(view);

    double z = horizontal ? ip.x : ip.y;
		view->zoom_start = y_view_interval_unconv_fn(vi,z);
		//g_message("zoom start: %f",view->zoom_start);
		view->zoom_in_progress = TRUE;
	}
  else if(event->button == 1 && (event->state & GDK_SHIFT_MASK)) {
    YViewInterval *vi = y_element_view_cartesian_get_view_interval ((YElementViewCartesian *) view,
						       META_AXIS);
    Point ip;
    Point *evp = (Point *) &(event->x);

    view_invconv(widget,evp,&ip);

    gboolean horizontal = get_horizontal(view);

    double z = horizontal ? ip.x : ip.y;

    y_view_interval_recenter_around_point(vi,y_view_interval_unconv_fn(vi,z));
  }
  return FALSE;
}

static gboolean
y_axis_view_motion_notify_event (GtkWidget *widget, GdkEventMotion *event)
{
	YAxisView *view = (YAxisView *) widget;
	if(view->zoom_in_progress) {
		YViewInterval *vi = y_element_view_cartesian_get_view_interval ((YElementViewCartesian *) view,
						       META_AXIS);
    Point ip;
    Point *evp = (Point *) &(event->x);

    view_invconv(widget,evp,&ip);

    gboolean horizontal = get_horizontal(view);

    double z = horizontal ? ip.x : ip.y;

		double pos = y_view_interval_unconv_fn(vi,z);
		if(pos!=view->cursor_pos) {
			view->cursor_pos = pos;
			gtk_widget_queue_draw(widget);
		}
	}
	return FALSE;
}

static gboolean
y_axis_view_button_release_event (GtkWidget *widget, GdkEventButton *event)
{
	YAxisView *view = (YAxisView *) widget;
	if(view->zoom_in_progress) {
		YViewInterval *vi = y_element_view_cartesian_get_view_interval ((YElementViewCartesian *) view,
						       META_AXIS);
    Point ip;
    Point *evp = (Point *) &(event->x);

    view_invconv(widget,evp,&ip);

    gboolean horizontal = get_horizontal(view);

    double z = horizontal ? ip.x : ip.y;
		double zoom_end = y_view_interval_unconv_fn(vi,z);

		y_view_interval_set_ignore_preferred_range(vi,TRUE);
		y_view_interval_set(vi,view->zoom_start,zoom_end);

		view->zoom_in_progress = FALSE;
		//g_message("zoom: %f to %f",view->zoom_start,zoom_end);
	}
	return FALSE;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
y_axis_view_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
    YAxisView *self = (YAxisView *) object;

    switch (property_id) {
    case AXIS_VIEW_DRAW_EDGE: {
      self->draw_edge = g_value_get_boolean (value);
      break;
    }
    case AXIS_VIEW_EDGE_THICKNESS: {
      self->edge_thickness = g_value_get_double (value);
      break;
    }
    case AXIS_VIEW_POSITION: {
      self->pos = g_value_get_int (value);
      break;
    }
    case AXIS_VIEW_DRAW_LABEL: {
      self->draw_label = g_value_get_boolean (value);
      break;
    }
    case AXIS_VIEW_LABEL_OFFSET: {
      self->label_offset = g_value_get_double (value);
      break;
    }
    case AXIS_VIEW_AXIS_LABEL: {
      //g_free (self->label);
      self->axis_label = g_value_dup_string (value);
      break;
    }
    case AXIS_VIEW_SHOW_MAJOR_TICKS: {
      self->show_major_ticks = g_value_get_boolean (value);
    }
      break;
    case AXIS_VIEW_MAJOR_TICK_THICKNESS: {
      self->major_tick_thickness = g_value_get_double (value);
      break;
    }
    case AXIS_VIEW_MAJOR_TICK_LENGTH: {
      self->major_tick_length = g_value_get_double (value);
      break;
    }
      case AXIS_VIEW_SHOW_MINOR_TICKS: {
      self->show_minor_ticks = g_value_get_boolean (value);
    }
      break;
    case AXIS_VIEW_SHOW_MAJOR_LABELS: {
      self->show_major_labels = g_value_get_boolean (value);
    }
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
      break;
    }
		if(property_id != AXIS_VIEW_POSITION) {
    	y_element_view_changed (Y_ELEMENT_VIEW(self));
		}
}

static void
y_axis_view_get_property (GObject      *object,
                        guint         property_id,
                        GValue       *value,
                        GParamSpec   *pspec)
{
    YAxisView *self = (YAxisView *) object;
    switch (property_id) {
    case AXIS_VIEW_DRAW_EDGE: {
      g_value_set_boolean (value, self->draw_edge);
    }
      break;
    case AXIS_VIEW_EDGE_THICKNESS: {
      g_value_set_double (value, self->edge_thickness);
    }
      break;
    case AXIS_VIEW_POSITION: {
      g_value_set_int (value, self->pos);
    }
      break;
    case AXIS_VIEW_DRAW_LABEL: {
      g_value_set_boolean (value, self->draw_label);
    }
      break;
    case AXIS_VIEW_LABEL_OFFSET: {
      g_value_set_double (value, self->label_offset);
    }
      break;
    case AXIS_VIEW_AXIS_LABEL: {
      g_value_set_string (value, self->axis_label);
    }
      break;
    case AXIS_VIEW_SHOW_MAJOR_TICKS: {
      g_value_set_boolean (value, self->show_major_ticks);
    }
      break;
    case AXIS_VIEW_MAJOR_TICK_THICKNESS: {
      g_value_set_double (value, self->major_tick_thickness);
    }
      break;
    case AXIS_VIEW_MAJOR_TICK_LENGTH: {
      g_value_set_double (value, self->major_tick_length);
    }
      break;
      case AXIS_VIEW_SHOW_MINOR_TICKS: {
      g_value_set_boolean (value, self->show_minor_ticks);
    }
      break;
    case AXIS_VIEW_SHOW_MAJOR_LABELS: {
      g_value_set_boolean (value, self->show_major_labels);
    }
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
      break;
    }
}


/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static GObject *
y_axis_view_constructor (GType                  gtype,
                       guint                  n_properties,
                       GObjectConstructParam *properties)
{
  GObject *obj;

  {
    /* Always chain up to the parent constructor */
    obj = G_OBJECT_CLASS (parent_class)->constructor (gtype, n_properties, properties);
  }

  /* update the object state depending on constructor properties */

  YAxisView *ax = Y_AXIS_VIEW(obj);

	if(ax->pos==SOUTH || ax->pos==NORTH) {
		gtk_widget_set_halign(GTK_WIDGET(obj),GTK_ALIGN_FILL);
		gtk_widget_set_hexpand(GTK_WIDGET(obj),TRUE);
	}
	else {
		gtk_widget_set_valign(GTK_WIDGET(obj),GTK_ALIGN_FILL);
		gtk_widget_set_vexpand(GTK_WIDGET(obj),TRUE);
	}

  if(ax->pos==SOUTH) {
    g_object_set(obj,"valign",GTK_ALIGN_START,NULL);
  }
  else if (ax->pos==WEST) {
    g_object_set(obj,"halign",GTK_ALIGN_END,NULL);
  }
  else if (ax->pos==EAST) {
    g_object_set(obj,"halign",GTK_ALIGN_START,NULL);
  }
  else if (ax->pos==NORTH) {
    g_object_set(obj,"valign",GTK_ALIGN_END,NULL);
  }

  return obj;
}

#define DEFAULT_DRAW_EDGE (TRUE)
#define DEFAULT_LINE_THICKNESS 1.5
#define DEFAULT_DRAW_LABEL (TRUE)
#define DEFAULT_SHOW_MAJOR_TICKS (TRUE)
#define DEFAULT_SHOW_MINOR_TICKS (FALSE)
#define DEFAULT_SHOW_MAJOR_LABELS (TRUE)

static void
y_axis_view_class_init (YAxisViewClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->set_property = y_axis_view_set_property;
  object_class->get_property = y_axis_view_get_property;
  object_class->constructor = y_axis_view_constructor;

  YElementViewClass *view_class = Y_ELEMENT_VIEW_CLASS (klass);

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

  widget_class->draw = y_axis_view_draw;
  widget_class->get_preferred_width = get_preferred_width;
  widget_class->get_preferred_height = get_preferred_height;

  widget_class->scroll_event = y_axis_view_scroll_event;
  widget_class->button_press_event = y_axis_view_button_press_event;
	widget_class->button_release_event = y_axis_view_button_release_event;
	widget_class->motion_notify_event = y_axis_view_motion_notify_event;

  parent_class = g_type_class_peek_parent (klass);

  /* properties */

  g_object_class_install_property (object_class, AXIS_VIEW_DRAW_EDGE,
                    g_param_spec_boolean ("draw-edge", "Draw Edge", "Whether to draw the axis edge",
                                        DEFAULT_DRAW_EDGE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, AXIS_VIEW_EDGE_THICKNESS,
                    g_param_spec_double ("edge-thickness", "Edge Thickness", "The thickness of the axis edge in points",
                                        0, 10, DEFAULT_LINE_THICKNESS , G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, AXIS_VIEW_POSITION,
                    g_param_spec_int ("position", "Axis position", "The position of the axis with respect to a plot",
                                        NORTH, WEST, WEST, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, AXIS_VIEW_DRAW_LABEL,
                    g_param_spec_boolean ("draw-label", "Draw Label", "Whether to draw an axis label",
                                        DEFAULT_DRAW_LABEL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, AXIS_VIEW_LABEL_OFFSET,
                    g_param_spec_double ("label-offset", "Label Offset", "The gap between ticks and labels in points",
                                        0, 10, 72.0/64.0 , G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, AXIS_VIEW_AXIS_LABEL,
                    g_param_spec_string ("axis-label", "Axis Label", "Set axis label", "", G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, AXIS_VIEW_SHOW_MAJOR_TICKS,
                    g_param_spec_boolean ("show-major-ticks", "Show major ticks", "Whether to draw major ticks",
                                        DEFAULT_SHOW_MAJOR_TICKS, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, AXIS_VIEW_MAJOR_TICK_THICKNESS,
                    g_param_spec_double ("major-tick-thickness", "Major Tick Thickness", "The thickness of major ticks in points",
                                        0, 10, DEFAULT_LINE_THICKNESS , G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, AXIS_VIEW_MAJOR_TICK_LENGTH,
                    g_param_spec_double ("major-tick-length", "Major Tick Length", "The length of major ticks in points",
                                        0, 10, 3.0 , G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, AXIS_VIEW_SHOW_MINOR_TICKS,
                    g_param_spec_boolean ("show-minor-ticks", "Show minor ticks", "Whether to draw minor ticks",
                                        DEFAULT_SHOW_MINOR_TICKS, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, AXIS_VIEW_SHOW_MAJOR_LABELS,
                    g_param_spec_boolean ("show-major-labels", "Show major labels", "Whether to draw major labels",
                                        DEFAULT_SHOW_MAJOR_LABELS, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  view_class->changed   = changed;
}

/**
 * y_axis_view_new:
 * @t: axis type
 *
 * Convenience function to create a new #YAxisView.
 *
 * Returns: the new axis view.
 **/
YAxisView * y_axis_view_new(compass_t t)
{
	YAxisView *a = g_object_new(Y_TYPE_AXIS_VIEW,"position",t,NULL);

  return a;
}

static void
y_axis_view_init (YAxisView *obj)
{
	obj->label_font = pango_font_description_from_string("Sans 10");

  gtk_widget_add_events(GTK_WIDGET(obj),GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  y_element_view_cartesian_add_view_interval ((YElementViewCartesian *) obj,
						  META_AXIS);
}

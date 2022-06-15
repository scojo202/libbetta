/*
 * b-color-bar.c
 *
 * Copyright (C) 2019 Scott O. Johnson (scojo202@gmail.com)
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

#include <math.h>
#include "plot/b-color-bar.h"
#include "plot/b-axis-markers.h"
#include "plot/b-color-map.h"

/**
 * SECTION: b-color-bar
 * @short_description: Widget for displaying a color map with an axis.
 *
 * Widget showing the color scale for a #BDensityPlot.
 *
 * The axis type to use to get/set the view interval and axis markers is
 * #B_AXIS_TYPE_META.
 *
 */

#define PROFILE 0

static GObjectClass *parent_class = NULL;

enum
{
  COLOR_BAR_DRAW_EDGE = 1,
  COLOR_BAR_EDGE_THICKNESS,
  COLOR_BAR_ORIENTATION,
  COLOR_BAR_DRAW_LABEL,
  COLOR_BAR_LABEL_OFFSET,
  COLOR_BAR_LABEL,
  COLOR_BAR_SHOW_MAJOR_TICKS,
  COLOR_BAR_MAJOR_TICK_THICKNESS,
  COLOR_BAR_MAJOR_TICK_LENGTH,
  COLOR_BAR_SHOW_MAJOR_LABELS,
  COLOR_BAR_SHOW_MINOR_TICKS,
  COLOR_BAR_MINOR_TICK_THICKNESS,
  COLOR_BAR_MINOR_TICK_LENGTH,
};

struct _BColorBar
{
  BElementViewCartesian base;
  BColorMap *map;
  gboolean is_horizontal;
  gboolean draw_edge, draw_label, show_major_ticks, show_minor_ticks,
    show_major_labels;
  double label_offset, edge_thickness, major_tick_thickness,
    major_tick_length, minor_tick_thickness, minor_tick_length;
  gchar *axis_label;
  PangoFontDescription *label_font;
  double op_start;
  double cursor_pos;
  gboolean zoom_in_progress;
  gboolean pan_in_progress;

};

G_DEFINE_TYPE (BColorBar, b_color_bar, B_TYPE_ELEMENT_VIEW_CARTESIAN);

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
color_bar_finalize(GObject *obj)
{
  BColorBar *cb = (BColorBar *) obj;

  g_clear_object(&cb->map);
  g_clear_pointer(&cb->axis_label,g_free);
  pango_font_description_free(cb->label_font);

  if (parent_class->finalize)
    parent_class->finalize (obj);
}

static void
b_color_bar_tick_properties (BColorBar * view,
			     const BTick * tick,
			     gboolean * show_tick,
			     double *thickness,
			     double *length,
			     gboolean * show_label,
			     double *label_offset)
{
  g_return_if_fail (B_IS_COLOR_BAR (view));

  if (show_tick)
    *show_tick = FALSE;

  if (show_label)
    *show_label = FALSE;

  g_return_if_fail (tick != NULL);

  if (label_offset)
    *label_offset = view->label_offset;

  switch (b_tick_type (tick))
    {

    case B_TICK_NONE:

      if (thickness)
        *thickness = 0;
      if (length)
        *length = 0;

      /*show_label = view->show_label; */
      g_object_get (view, "show_lone_labels", show_label, NULL);
      break;

    case B_TICK_MAJOR:
    case B_TICK_MAJOR_RULE:

      *show_tick = view->show_major_ticks;
      *thickness = view->major_tick_thickness;
      *length = view->major_tick_length;
      *show_label = view->show_major_labels;
      break;

    case B_TICK_MINOR:
    case B_TICK_MINOR_RULE:

      *show_tick = view->show_minor_ticks;
			*length = view->minor_tick_length;
			*show_label = FALSE;
      break;

    case B_TICK_MICRO:
    case B_TICK_MICRO_RULE:

      break;

    default:

      g_assert_not_reached ();

    }
}

static GtkSizeRequestMode
color_bar_get_request_mode(GtkWidget *widget)
{
  //BAxisView *ax = B_AXIS_VIEW(widget);
  //gboolean horizontal = get_horizontal (ax);
  //if (horizontal)
  //  return
  return GTK_SIZE_REQUEST_CONSTANT_SIZE;
}

static int
compute_axis_size_request (BColorBar * b_color_bar)
{
  BAxisMarkers *am;
  g_return_val_if_fail (B_IS_COLOR_BAR (b_color_bar), 0);

#if PROFILE
  GTimer *t = g_timer_new ();
#endif

  gboolean horizontal = b_color_bar->is_horizontal;
  double edge_thickness = 0, legend_offset = 0;
  gchar *legend;
  int w = 0, h = 0;
  gint i;

  horizontal = b_color_bar->is_horizontal;

  legend = b_color_bar->axis_label;

  am =
    b_element_view_cartesian_get_axis_markers ((BElementViewCartesian *)
					       b_color_bar, B_AXIS_TYPE_META);

  /* Account for the size of the axis labels */

  PangoContext *context = gtk_widget_get_pango_context(GTK_WIDGET(b_color_bar));
  PangoLayout *layout = NULL;

  layout = pango_layout_new (context);

  pango_layout_set_font_description (layout, b_color_bar->label_font);

  for (i = am ? b_axis_markers_size (am) - 1 : -1; i >= 0; --i)
    {
      const BTick *tick;
      gboolean show_tick, show_label;
      double length, label_offset, thickness;
      int tick_w = 0, tick_h = 0;

      tick = b_axis_markers_get (am, i);

      b_color_bar_tick_properties (b_color_bar, tick, &show_tick, &thickness,
                                   &length, &show_label, &label_offset);

      if (show_label && b_tick_is_labelled (tick))
      {
        pango_layout_set_text (layout, b_tick_label (tick), -1);

        pango_layout_get_pixel_size (layout, &tick_w, &tick_h);

        if (horizontal)
          tick_h += label_offset;
        else
          tick_w += label_offset;
      }

      if (show_tick)
      {
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

    if (horizontal)
      h += 25;
    else
      w += 25;

  /* Account for the edge thickness */

  if (b_color_bar->draw_edge)
    {
      if (horizontal)
        h += edge_thickness;
      else
        w += edge_thickness;
    }

  /* Account for the height of the legend */

  if (legend && *legend)
    {
      int legend_h = 0;
      pango_layout_set_text (layout, legend, -1);

      pango_layout_get_pixel_size (layout, NULL, &legend_h);

      if (horizontal)
        h += legend_h + legend_offset;
      else
        w += legend_h + legend_offset;
    }

  g_object_unref (layout);

#ifdef __APPLE__
  w += 15;
  h += 15;
#endif

#if PROFILE
  double te = g_timer_elapsed (t, NULL);
  g_message ("axis view compute size %d: %f ms", b_color_bar->pos, te * 1000);
  g_timer_destroy (t);
#endif

  if (horizontal)
    return h;
  else
    return w;
}

static void
color_bar_measure (GtkWidget      *widget,
         GtkOrientation  orientation,
         int             for_size,
         int            *minimum_size,
         int            *natural_size,
         int            *minimum_baseline,
         int            *natural_baseline)
{
  BColorBar *a = B_COLOR_BAR (widget);
  *minimum_size=1;
  if(orientation==GTK_ORIENTATION_HORIZONTAL)
    {
      if (a->is_horizontal)
      {
        *natural_size = 20;
      }
      else
      {
        *natural_size = compute_axis_size_request (a);
        g_debug ("axis: requesting width %d", *natural_size);
      }
    }
  else
    {
      if (!a->is_horizontal)
      {
        *natural_size = 20;
      }
    else
      {
        *natural_size = compute_axis_size_request (a);
        g_debug ("axis: requesting height %d", *natural_size);
      }
    }
  //
  //*natural_size=20;
  //*minimum_baseline=0;
  //*natural_baseline=0;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
changed (BElementView * view)
{
  BColorBar *a = B_COLOR_BAR (view);
  /* don't let this run before the position is set */
  //if (a->pos == B_COMPASS_INVALID)
  //  return;
  gint thickness = compute_axis_size_request ((BColorBar *) view);
  int current_thickness;
  if (!a->is_horizontal)
    {
      current_thickness = gtk_widget_get_allocated_width (GTK_WIDGET (view));
    }
  else
    {
      current_thickness = gtk_widget_get_allocated_height (GTK_WIDGET (view));
    }
  if (thickness != current_thickness)
    gtk_widget_queue_resize (GTK_WIDGET (view));

  if (B_ELEMENT_VIEW_CLASS (parent_class)->changed)
    B_ELEMENT_VIEW_CLASS (parent_class)->changed (view);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
color_bar_draw (GtkWidget * w, cairo_t * cr)
{
  BElementView *view = B_ELEMENT_VIEW (w);

  BAxisMarkers *am;
  BViewInterval *vi;
  gboolean horizontal = TRUE;
  gchar *legend;
  BPoint pt1, pt2, pt3;
  gint i;

#if PROFILE
  GTimer *t = g_timer_new ();
#endif

  BColorBar *b_color_bar = B_COLOR_BAR (view);

  horizontal = b_color_bar->is_horizontal;

  vi =
    b_element_view_cartesian_get_view_interval ((BElementViewCartesian *) view,
                                                B_AXIS_TYPE_META);

  /* Render the edge of the bar */

  GdkPixbuf *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
           256, 1);

  int n_channels = gdk_pixbuf_get_n_channels (pixbuf);
  guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);

  double dl = 1.0 / 256.0;
  for(i=0;i<256;i++) {
    guint32 c = b_color_map_get_map(b_color_bar->map,i*dl);
    guchar red, green, blue;
    UINT_TO_RGB(c,&red,&green,&blue);
    pixels[n_channels*i]=red;
    pixels[n_channels*i+1]=green;
    pixels[n_channels*i+2]=blue;
  }

  int height;
  if(b_color_bar->is_horizontal)
    height = gtk_widget_get_allocated_width (w);
  else
    height = gtk_widget_get_allocated_height (w);

  GdkPixbuf *scaled_pixbuf =
    gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, height, 25);

  gdk_pixbuf_scale (pixbuf, scaled_pixbuf,
                          0, 0, height, 25,
      		    0.0, 0.0, ((double)height)/256.0, 25.0, GDK_INTERP_TILES);

  if(!b_color_bar->is_horizontal)
    scaled_pixbuf = gdk_pixbuf_rotate_simple(scaled_pixbuf,
                                            GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);

  if (b_color_bar->is_horizontal)
  {
    pt1.x = 0;
    pt1.y = 0;
    pt2.x = 1;
    pt2.y = 0;
  }
  else
  {
    pt1.x = 0;
    pt1.y = 0;
    pt2.x = 0;
    pt2.y = 1;
  }
  _view_conv (w, &pt1, &pt1);
  _view_conv (w, &pt2, &pt2);

  if(b_color_bar->is_horizontal)
  {

  }
  else
  {
    pt1.x+=2;
    pt2.x+=2;
  }

  cairo_set_line_width (cr, b_color_bar->edge_thickness);

  cairo_move_to (cr, pt1.x, pt1.y);
  cairo_line_to (cr, pt2.x, pt2.y);
  if(b_color_bar->is_horizontal)
  {

  }
  else
  {
    cairo_line_to (cr, pt2.x+25, pt2.y);
    cairo_line_to (cr, pt1.x+25, pt1.y);
    cairo_line_to (cr, pt1.x, pt1.y);
  }

  gdk_cairo_set_source_pixbuf (cr, scaled_pixbuf, 2, 0);
  cairo_fill_preserve(cr);

  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);

  if (b_color_bar->draw_edge)
    cairo_stroke (cr);

  /* Render our markers */

  am =
    b_element_view_cartesian_get_axis_markers ((BElementViewCartesian *) view,
					       B_AXIS_TYPE_META);

  double tick_length = 0;
  double max_offset = 0;

  PangoLayout *layout = pango_cairo_create_layout (cr);

  pango_layout_set_font_description (layout, b_color_bar->label_font);

  for (i = am ? b_axis_markers_size (am) - 1 : -1; i >= 0; --i)
    {
      const BTick *tick;
      gboolean show_tick, show_label;
      double t, length, thickness, label_offset;
      BAnchor anchor;

      tick = b_axis_markers_get (am, i);

      b_color_bar_tick_properties (B_COLOR_BAR (view), tick, &show_tick,
                                   &thickness, &length, &show_label,
                                   &label_offset);

      t = b_tick_position (tick);
      t = b_view_interval_conv (vi, t);

      if (b_color_bar->is_horizontal)
      {
        pt1.x = t;
        pt1.y = 0;
        _view_conv (w, &pt1, &pt1);

        pt2 = pt1;
        pt2.y -= length;

        pt3 = pt2;
        pt3.y -= label_offset;

        anchor = ANCHOR_BOTTOM;
      }
      else {
        pt1.x = 0;
        pt1.y = t;
        _view_conv (w, &pt1, &pt1);

        pt2 = pt1;

        pt1.x +=2+25;
        pt2.x += 2+25+length;

        pt3 = pt2;
        pt3.x += label_offset;

        anchor = ANCHOR_LEFT;
      }

      if (show_tick)
      {
        cairo_set_line_width (cr, b_color_bar->major_tick_thickness);
        cairo_move_to (cr, pt1.x, pt1.y);
        cairo_line_to (cr, pt2.x, pt2.y);
        cairo_stroke (cr);
        tick_length = MAX (tick_length, length);
      }

      if (b_tick_is_labelled (tick) && show_label)
      {
        int dw, dh;

        pango_layout_set_text (layout, b_tick_label (tick), -1);

        pango_layout_get_pixel_size (layout, &dw, &dh);

        gboolean over_edge = FALSE;

        if (horizontal)
        {
          if (pt3.x - dw / 2 < 0)
            over_edge = TRUE;
          if (pt3.x + dw / 2 > gtk_widget_get_allocated_width (w))
            over_edge = TRUE;
        }
        else
        {
          if (pt3.y - dh / 2 < 0)
            over_edge = TRUE;
          if (pt3.y + dh / 2 > gtk_widget_get_allocated_height (w))
            over_edge = TRUE;
        }

        if (!over_edge)
        {
          _string_draw_no_rotate (cr, b_color_bar->label_font, pt3, anchor, b_tick_label(tick));

          if (horizontal)
          {
            if (dh > max_offset)
            {
              max_offset = dh + label_offset;
            }
          }
          else
          {
            if (dw > max_offset)
            {
              max_offset = dw + label_offset;
            }
          }
        }
      }
    }

  g_object_unref (layout);

  legend = b_color_bar->axis_label;

  if (legend && *legend)
    {
      if (b_color_bar->is_horizontal)
        {
          pt1.x = 0.5;
          pt1.y = 0;
          _view_conv (w, &pt1, &pt1);
          pt1.y -= (max_offset + tick_length+25);
          _string_draw (cr, b_color_bar->label_font, pt1, ANCHOR_BOTTOM, ROT_0,
                        legend);
        }
      else
        {
        pt1.x = 0;
        pt1.y = 0.5;
        _view_conv (w, &pt1, &pt1);
        pt1.x += (max_offset + tick_length+25);
        _string_draw (cr, b_color_bar->label_font, pt1, ANCHOR_BOTTOM, ROT_270,
                      legend);
        }
    }

  /* draw zoom thing */
  if (b_color_bar->zoom_in_progress)
    {
      double z = b_view_interval_conv (vi, b_color_bar->op_start);
      double e = b_view_interval_conv (vi, b_color_bar->cursor_pos);

      if(b_color_bar->is_horizontal)
        {
          pt1.x = z;
          pt1.y = 0;
          pt2.x = e;
          pt2.y = 0;
        }
      else
        {
          pt1.x = 0;
          pt1.y = z;
          pt2.x = 0;
          pt2.y = e;
        }

      _view_conv (w, &pt1, &pt1);
      _view_conv (w, &pt2, &pt2);

      cairo_set_line_width (cr, b_color_bar->edge_thickness);
      cairo_set_source_rgba (cr, 0.0, 0.0, 1.0, 0.25);

      cairo_move_to (cr, pt1.x, pt1.y);
      cairo_line_to (cr, pt2.x, pt2.y);

      BPoint pt0 = pt1;

      if(b_color_bar->is_horizontal)
        {
          pt1.x = z;
          pt1.y = 1;
          pt2.x = e;
          pt2.y = 1;
        }
      else
        {
          pt1.x = 1;
          pt1.y = z;
          pt2.x = 1;
          pt2.y = e;
        }

      _view_conv (w, &pt1, &pt1);
      _view_conv (w, &pt2, &pt2);

      cairo_line_to (cr, pt2.x, pt2.y);
      cairo_line_to (cr, pt1.x, pt1.y);
      cairo_line_to (cr, pt0.x, pt0.y);

      cairo_fill (cr);
    }

#if PROFILE
  double te = g_timer_elapsed (t, NULL);
  g_message ("axis view draw %d: %f ms", b_color_bar->pos, te * 1000);
  g_timer_destroy (t);
#endif

  return TRUE;
}

static void
color_bar_snapshot (GtkWidget   *w,
                      GtkSnapshot *s)
{
  graphene_rect_t bounds;
  if (gtk_widget_compute_bounds(w,w,&bounds)) {
    cairo_t *cr = gtk_snapshot_append_cairo (s, &bounds);
    color_bar_draw(w, cr);
  }
}

static gboolean
color_bar_scroll_event (GtkEventControllerScroll * controller, double dx, double dy, gpointer user_data)
{
  BColorBar *view = (BColorBar *) user_data;

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

  BViewInterval *vi =
    b_element_view_cartesian_get_view_interval ((BElementViewCartesian *)
						view,
						B_AXIS_TYPE_META);

  double scale = direction ? 0.8 : 1.0 / 0.8;

  b_view_interval_rescale_around_point (vi, view->cursor_pos,
					scale);

  return FALSE;
}

#if 0
static void
b_color_bar_do_popup_menu (GtkWidget * my_widget, GdkEventButton * event)
{
  BColorBar *view = (BColorBar *) my_widget;

  GtkWidget *menu;

  menu = gtk_menu_new ();

  GtkWidget *autoscale =
    _y_create_autoscale_menu_check_item ((BElementViewCartesian *) view,
				      B_AXIS_TYPE_META, "Autoscale axis");
  gtk_widget_show (autoscale);

  gtk_menu_shell_append (GTK_MENU_SHELL (menu), autoscale);

  gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}
#endif

static gboolean
color_bar_press_event (GtkGestureClick *gesture,
               int n_press,
                                 double x, double y,
               gpointer         user_data)
{
  BColorBar *view = (BColorBar *) user_data;
  /* Ignore double-clicks and triple-clicks */
  if(n_press != 1)
    return FALSE;

  BViewInterval *vi =
        b_element_view_cartesian_get_view_interval ((BElementViewCartesian *)
						    view,
						    B_AXIS_TYPE_META);

  BPoint ip,evp;
  evp.x = x;
  evp.y = y;

  _view_invconv (GTK_WIDGET(view), &evp, &ip);
  GdkModifierType t =gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));

  if (b_element_view_get_zooming (B_ELEMENT_VIEW (view)))
    {
      double z = view->is_horizontal ? ip.x : ip.y;
      view->op_start = b_view_interval_unconv (vi, z);
      view->zoom_in_progress = TRUE;
    }
  else if ((t & GDK_SHIFT_MASK))
    {
      double z = view->is_horizontal ? ip.x : ip.y;

      b_view_interval_recenter_around_point (vi,
					     b_view_interval_unconv (vi,
									z));
    }
  else if (b_element_view_get_panning (B_ELEMENT_VIEW (view)))
    {
      b_view_interval_set_ignore_preferred_range (vi, TRUE);

      double z = view->is_horizontal ? ip.x : ip.y;
      view->op_start = b_view_interval_unconv (vi, z);
      /* this is the position where the pan started */

      view->pan_in_progress = TRUE;
    }
  return FALSE;
}

static gboolean
color_bar_motion_notify_event (GtkEventControllerMotion *controller, double x, double y, gpointer user_data)
{
  BColorBar *view = (BColorBar *) user_data;

  BViewInterval *vi =
        b_element_view_cartesian_get_view_interval ((BElementViewCartesian *)
						    view,
						    B_AXIS_TYPE_META);

  BPoint ip, evp;
  evp.x = x;
  evp.y = y;

  _view_invconv (GTK_WIDGET(view), &evp, &ip);
  double pos = b_view_interval_unconv (vi, view->is_horizontal ? ip.x : ip.y);
  if (pos != view->cursor_pos)
    {
      view->cursor_pos = pos;
    }

  if (view->zoom_in_progress)
    {
        gtk_widget_queue_draw (GTK_WIDGET(view));	/* for zoom box */
    }
  else if (view->pan_in_progress)
    {
      /* Calculate the translation required to put the cursor at the
       * start position. */

      double dv = view->cursor_pos - view->op_start;

      b_view_interval_translate (vi, -dv);
    }
  return FALSE;
}

static gboolean
color_bar_release_event (GtkGestureClick *gesture,
               int n_press,
                                 double x, double y,
               gpointer         user_data)
{
  BColorBar *view = (BColorBar *) user_data;

  BPoint ip,evp;
  evp.x = x;
  evp.y = y;

  _view_invconv (GTK_WIDGET(view), &evp, &ip);

  if (view->zoom_in_progress)
    {
      BViewInterval *vi =
        b_element_view_cartesian_get_view_interval ((BElementViewCartesian *)
						    view,
						    B_AXIS_TYPE_META);

      double z = view->is_horizontal ? ip.x : ip.y;
      double zoom_end = b_view_interval_unconv (vi, z);

      b_view_interval_set_ignore_preferred_range (vi, TRUE);
      if (view->op_start != zoom_end)
      {
        b_view_interval_set (vi, view->op_start, zoom_end);
      }
      else
      {
        GdkModifierType t =gtk_event_controller_get_current_event_state(GTK_EVENT_CONTROLLER(gesture));
        if(t & GDK_ALT_MASK) {
          b_view_interval_rescale_around_point (vi, zoom_end, 1.0/0.8);
        }
        else {
          b_view_interval_rescale_around_point (vi, zoom_end, 0.8);
        }
      }

      view->zoom_in_progress = FALSE;
    }
  else if (view->pan_in_progress)
    {
      view->pan_in_progress = FALSE;
    }
  return FALSE;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
b_color_bar_set_property (GObject * object,
			  guint property_id,
			  const GValue * value, GParamSpec * pspec)
{
  BColorBar *self = (BColorBar *) object;

  switch (property_id)
    {
    case COLOR_BAR_DRAW_EDGE:
      {
        self->draw_edge = g_value_get_boolean (value);
        break;
      }
    case COLOR_BAR_EDGE_THICKNESS:
      {
        self->edge_thickness = g_value_get_double (value);
        break;
      }
    case COLOR_BAR_ORIENTATION:
      {
        //self->pos = g_value_get_enum (value);
        break;
      }
    case COLOR_BAR_DRAW_LABEL:
      {
        self->draw_label = g_value_get_boolean (value);
        break;
      }
    case COLOR_BAR_LABEL_OFFSET:
      {
        self->label_offset = g_value_get_double (value);
        break;
      }
    case COLOR_BAR_LABEL:
      {
        //g_free (self->label);
        self->axis_label = g_value_dup_string (value);
        break;
      }
    case COLOR_BAR_SHOW_MAJOR_TICKS:
      {
        self->show_major_ticks = g_value_get_boolean (value);
      }
      break;
    case COLOR_BAR_MAJOR_TICK_THICKNESS:
      {
        self->major_tick_thickness = g_value_get_double (value);
      }
      break;
    case COLOR_BAR_MAJOR_TICK_LENGTH:
      {
        self->major_tick_length = g_value_get_double (value);
        break;
      }
    case COLOR_BAR_MINOR_TICK_LENGTH:
      {
        self->minor_tick_length = g_value_get_double (value);
        break;
      }
    case COLOR_BAR_SHOW_MINOR_TICKS:
      {
        self->show_minor_ticks = g_value_get_boolean (value);
      }
      break;
    case COLOR_BAR_SHOW_MAJOR_LABELS:
      {
        self->show_major_labels = g_value_get_boolean (value);
      }
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
  if (property_id != COLOR_BAR_ORIENTATION)
    {
      b_element_view_changed (B_ELEMENT_VIEW (self));
    }
}

static void
b_color_bar_get_property (GObject * object,
			  guint property_id,
			  GValue * value, GParamSpec * pspec)
{
  BColorBar *self = (BColorBar *) object;
  switch (property_id)
    {
    case COLOR_BAR_DRAW_EDGE:
      {
        g_value_set_boolean (value, self->draw_edge);
      }
      break;
    case COLOR_BAR_EDGE_THICKNESS:
      {
        g_value_set_double (value, self->edge_thickness);
      }
      break;
    case COLOR_BAR_ORIENTATION:
      {
        g_value_set_int (value, GTK_ORIENTATION_VERTICAL);
      }
      break;
    case COLOR_BAR_DRAW_LABEL:
      {
        g_value_set_boolean (value, self->draw_label);
      }
      break;
    case COLOR_BAR_LABEL_OFFSET:
      {
        g_value_set_double (value, self->label_offset);
      }
      break;
    case COLOR_BAR_LABEL:
      {
        g_value_set_string (value, self->axis_label);
      }
      break;
    case COLOR_BAR_SHOW_MAJOR_TICKS:
      {
        g_value_set_boolean (value, self->show_major_ticks);
      }
      break;
    case COLOR_BAR_MAJOR_TICK_THICKNESS:
      {
        g_value_set_double (value, self->major_tick_thickness);
      }
      break;
    case COLOR_BAR_MAJOR_TICK_LENGTH:
      {
        g_value_set_double (value, self->major_tick_length);
      }
      break;
    case COLOR_BAR_MINOR_TICK_LENGTH:
        {
          g_value_set_double (value, self->minor_tick_length);
        }
      break;
    case COLOR_BAR_SHOW_MINOR_TICKS:
      {
        g_value_set_boolean (value, self->show_minor_ticks);
      }
      break;
    case COLOR_BAR_SHOW_MAJOR_LABELS:
      {
        g_value_set_boolean (value, self->show_major_labels);
      }
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static GObject *
b_color_bar_constructor (GType gtype,
			 guint n_properties,
			 GObjectConstructParam * properties)
{
  GObject *obj;

  {
    /* Always chain up to the parent constructor */
    obj =
      G_OBJECT_CLASS (parent_class)->constructor (gtype, n_properties,
						  properties);
  }

  /* update the object state depending on constructor properties */

  BColorBar *ax = B_COLOR_BAR (obj);

  if (ax->is_horizontal)
    {
      gtk_widget_set_halign (GTK_WIDGET (obj), GTK_ALIGN_FILL);
      gtk_widget_set_hexpand (GTK_WIDGET (obj), TRUE);
    }
  else
    {
      gtk_widget_set_valign (GTK_WIDGET (obj), GTK_ALIGN_FILL);
      gtk_widget_set_vexpand (GTK_WIDGET (obj), TRUE);
    }

  return obj;
}

#define DEFAULT_DRAW_EDGE (TRUE)
#define DEFAULT_LINE_THICKNESS 1
#define DEFAULT_DRAW_LABEL (TRUE)
#define DEFAULT_SHOW_MAJOR_TICKS (TRUE)
#define DEFAULT_SHOW_MINOR_TICKS (TRUE)
#define DEFAULT_SHOW_MAJOR_LABELS (TRUE)

static void
b_color_bar_class_init (BColorBarClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->set_property = b_color_bar_set_property;
  object_class->get_property = b_color_bar_get_property;
  object_class->constructor = b_color_bar_constructor;
  object_class->finalize = color_bar_finalize;

  BElementViewClass *view_class = B_ELEMENT_VIEW_CLASS (klass);

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->snapshot = color_bar_snapshot;
  widget_class->measure = color_bar_measure;
  widget_class->get_request_mode = color_bar_get_request_mode;

  parent_class = g_type_class_peek_parent (klass);

  /* properties */

  g_object_class_install_property (object_class, COLOR_BAR_DRAW_EDGE,
				   g_param_spec_boolean ("draw-edge", "Draw Edge",
							 "Whether to draw the axis edge",
							 DEFAULT_DRAW_EDGE,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, COLOR_BAR_EDGE_THICKNESS,
				   g_param_spec_double ("edge-thickness", "Edge Thickness",
							"The thickness of the axis edge in pixels",
							0, 10,
							DEFAULT_LINE_THICKNESS,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, COLOR_BAR_ORIENTATION,
				   g_param_spec_int ("orientation", "Orientation",
						     "Whether the colorbar is horizontal or vertical",
						     GTK_ORIENTATION_HORIZONTAL, GTK_ORIENTATION_VERTICAL, GTK_ORIENTATION_HORIZONTAL,
						     G_PARAM_READWRITE |
						     G_PARAM_CONSTRUCT_ONLY |
						     G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, COLOR_BAR_DRAW_LABEL,
				   g_param_spec_boolean ("draw-label", "Draw Label",
							 "Whether to draw an axis label",
							 DEFAULT_DRAW_LABEL,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, COLOR_BAR_LABEL_OFFSET,
				   g_param_spec_double ("label-offset", "Label Offset",
							"The gap between ticks and labels in pixels",
							0, 10, 2,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, COLOR_BAR_LABEL,
				   g_param_spec_string ("bar-label", "Bar Label",
							"Set color bar label", "",
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, COLOR_BAR_SHOW_MAJOR_TICKS,
				   g_param_spec_boolean ("show-major-ticks", "Show major ticks",
							 "Whether to draw major ticks",
							 DEFAULT_SHOW_MAJOR_TICKS,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
				   COLOR_BAR_MAJOR_TICK_THICKNESS,
				   g_param_spec_double
				   ("major-tick-thickness", "Major Tick Thickness",
				    "The thickness of major ticks in pixels",
				    0, 10, DEFAULT_LINE_THICKNESS,
				    G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
				    G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, COLOR_BAR_MAJOR_TICK_LENGTH,
				   g_param_spec_double ("major-tick-length", "Major Tick Length",
							"The length of major ticks in pixels",
							0, 100, 5.0,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, COLOR_BAR_SHOW_MINOR_TICKS,
				   g_param_spec_boolean ("show-minor-ticks", "Show minor ticks",
							 "Whether to draw minor ticks",
							 DEFAULT_SHOW_MINOR_TICKS,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class, COLOR_BAR_MINOR_TICK_LENGTH,
						g_param_spec_double ("minor-tick-length", "Minor Tick Length",
						 							"The length of minor ticks in pixels",
						 							0, 100, 3.0,
						 							G_PARAM_READWRITE |
						 							G_PARAM_CONSTRUCT |
						 							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, COLOR_BAR_SHOW_MAJOR_LABELS,
				   g_param_spec_boolean ("show-major-labels", "Show major labels",
							 "Whether to draw major labels",
							 DEFAULT_SHOW_MAJOR_LABELS,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

  view_class->changed = changed;
}

static void
b_color_bar_init (BColorBar * obj)
{
  obj->label_font = pango_font_description_from_string ("Sans 12");

  GtkEventController *motion_controller = gtk_event_controller_motion_new();
  gtk_widget_add_controller(GTK_WIDGET(obj), motion_controller);

  g_signal_connect(motion_controller, "motion", G_CALLBACK(color_bar_motion_notify_event), obj);

  GtkGesture *click_controller = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click_controller),1);
  gtk_widget_add_controller(GTK_WIDGET(obj), GTK_EVENT_CONTROLLER(click_controller));

  g_signal_connect(click_controller, "pressed", G_CALLBACK(color_bar_press_event), obj);
  g_signal_connect(click_controller, "released", G_CALLBACK(color_bar_release_event), obj);

  GtkEventController *scroll_controller = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
  gtk_widget_add_controller(GTK_WIDGET(obj), GTK_EVENT_CONTROLLER(scroll_controller));

  g_signal_connect(scroll_controller, "scroll", G_CALLBACK(color_bar_scroll_event), obj);

  b_element_view_cartesian_add_view_interval ((BElementViewCartesian *) obj,
					      B_AXIS_TYPE_META);
}

/**
 * b_color_bar_new:
 * @o: either GTK_ORIENTATION_HORIZONTAL or GTK_ORIENTATION_VERTICAL
 * @m: a colormap to use
 *
 * Convenience function to create a new #BColorBar.
 *
 * Returns: the new color bar.
 **/
BColorBar *
b_color_bar_new (GtkOrientation o, BColorMap *m)
{
  BColorBar *a = g_object_new (B_TYPE_COLOR_BAR, "orientation", o, NULL);
  a->map = g_object_ref(m);

  return a;
}

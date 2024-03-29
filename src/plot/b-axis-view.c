/*
 * b-axis-view.c
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
#include "b-plot-enums.h"
#include "plot/b-axis-view.h"
#include "plot/b-axis-markers.h"

/**
 * SECTION: b-axis-view
 * @short_description: Widget for displaying a linear or logarithmic axis.
 *
 * This widget is used to display axes along the edges of a #BScatterLineView or
 * #BDensityView. The orientation of the axis and position of the axis edge are
 * controlled by the "position" property, which indicates on which side of the
 * plot the axis should be placed.
 *
 * The axis type to use to get/set the view interval and axis markers is
 * #B_AXIS_TYPE_META.
 *
 * The color used for the edge and tick marks are controlled
 * using a CSS stylesheet. Classes called "edge", "major-ticks", "minor-ticks"
 * can be used to set the color.
 *
 */

#define PROFILE 0

static GObjectClass *parent_class = NULL;

enum
{
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

struct _BAxisView
{
  BElementViewCartesian base;
  BCompass pos;
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

G_DEFINE_TYPE (BAxisView, b_axis_view, B_TYPE_ELEMENT_VIEW_CARTESIAN);

static void
axis_view_finalize(GObject *obj)
{
  BAxisView *ax = (BAxisView *) obj;

  g_clear_pointer(&ax->axis_label,g_free);
  pango_font_description_free(ax->label_font);

  if (parent_class->finalize)
    parent_class->finalize (obj);
}

static gboolean
get_horizontal (BAxisView * b_axis_view)
{
  gboolean horizontal = FALSE;

  switch (b_axis_view->pos)
    {
    case B_COMPASS_NORTH:
    case B_COMPASS_SOUTH:
      horizontal = TRUE;
      break;

    case B_COMPASS_EAST:
    case B_COMPASS_WEST:
      horizontal = FALSE;
      break;
    case B_COMPASS_INVALID:
      g_assert_not_reached ();
      break;
    }

  return horizontal;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
b_axis_view_tick_properties (BAxisView * view,
                             const BTick * tick,
                             gboolean * show_tick,
                             double *thickness,
                             double *length,
                             gboolean * show_label,
                             double *label_offset)
{
  g_return_if_fail (B_IS_AXIS_VIEW (view));

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
      if (show_label)
        *show_label = view->show_major_labels;
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
      /*g_object_get (G_OBJECT (view),
         "show_minor_ticks",     show_tick,
         //"minor_tick_color",     color,
         //"minor_tick_thickness", thickness,
         //"minor_tick_length",    length,
         //"show_minor_labels",    show_label,
         NULL); */
      break;

    case B_TICK_MICRO:
    case B_TICK_MICRO_RULE:
      *show_tick = TRUE;
      *length = 2.0;
      /*g_object_get (G_OBJECT (view),
         "show_micro_ticks",     show_tick,
         "micro_tick_thickness", thickness,
         "micro_tick_length",    length,
         "show_micro_labels",    show_label,
         NULL); */
      break;

    default:

      g_assert_not_reached ();

    }
}

static int
compute_axis_size_request (BAxisView * b_axis_view)
{
  BAxisMarkers *am;
  BElementViewCartesian *cart = (BElementViewCartesian *) b_axis_view;
  g_return_val_if_fail (B_IS_AXIS_VIEW (b_axis_view), 1);

#if PROFILE
  GTimer *t = g_timer_new ();
#endif

  gboolean horizontal = get_horizontal (b_axis_view);
  double edge_thickness = 0, legend_offset = 0;
  gchar *legend = b_axis_view->axis_label;
  int w = 0, h = 0;
  int i;

  am = b_element_view_cartesian_get_axis_markers (cart, B_AXIS_TYPE_META);

  /* Account for the size of the axis labels */

  PangoContext *context = gtk_widget_get_pango_context(GTK_WIDGET(b_axis_view));
  PangoLayout *layout = pango_layout_new (context);

  pango_layout_set_font_description (layout, b_axis_view->label_font);

  for (i = am ? b_axis_markers_size (am) - 1 : -1; i >= 0; --i)
    {

      const BTick *tick;
      gboolean show_tick, show_label;
      double length, label_offset, thickness;
      int tick_w = 0, tick_h = 0;

      tick = b_axis_markers_get (am, i);

      b_axis_view_tick_properties (b_axis_view, tick, &show_tick, &thickness,
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

  /* Account for the edge thickness */

  if (b_axis_view->draw_edge)
    {
      if (horizontal)
        h += edge_thickness;
      else
        w += edge_thickness;
    }

  /* Account for the height of the axis label */

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

#if PROFILE
  double te = g_timer_elapsed (t, NULL);
  g_message ("axis view compute size %d: %f ms", b_axis_view->pos, te * 1000);
  g_timer_destroy (t);
#endif

#ifdef __APPLE__
  h+=15;
  w+=15;
#endif

  if (horizontal)
    return h;
  else
    return w;
}

static GtkSizeRequestMode
axis_view_get_request_mode(GtkWidget *widget)
{
  return GTK_SIZE_REQUEST_CONSTANT_SIZE;
}

static void
axis_view_measure (GtkWidget      *widget,
         GtkOrientation  orientation,
         int             for_size,
         int            *minimum_size,
         int            *natural_size,
         int            *minimum_baseline,
         int            *natural_baseline)
{
  BAxisView *a = B_AXIS_VIEW (widget);
  *minimum_size=1;
  if(orientation==GTK_ORIENTATION_HORIZONTAL)
    {
      if (get_horizontal(a))
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
      if (!get_horizontal(a))
      {
        *natural_size = 20;
      }
    else
      {
        *natural_size = compute_axis_size_request (a);
        g_debug ("axis: requesting height %d", *natural_size);
      }
    }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
changed (BElementView * view)
{
  BAxisView *a = B_AXIS_VIEW (view);
  /* don't let this run before the position is set */
  if (a->pos == B_COMPASS_INVALID)
    return;
  gint thickness = compute_axis_size_request (a);
  int current_thickness;
  if (a->pos == B_COMPASS_EAST || a->pos == B_COMPASS_WEST)
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
axis_view_draw (GtkWidget * w, cairo_t * cr)
{
  BElementView *view = B_ELEMENT_VIEW (w);
  BElementViewCartesian *cart = (BElementViewCartesian *) w;
  BAxisView *b_axis_view = B_AXIS_VIEW (view);

  BAxisMarkers *am;
  BViewInterval *vi;
  gboolean horizontal = TRUE;
  gchar *legend;
  BPoint pt1, pt2, pt3;
  gint i;

  GtkStyleContext *stc = gtk_widget_get_style_context (w);

#if PROFILE
  GTimer *t = g_timer_new ();
#endif

  horizontal = get_horizontal (b_axis_view);

  /*cairo_move_to(cr, 0,0);
     cairo_line_to(cr,0,view->alloc_height);
     cairo_line_to(cr,view->alloc_width,view->alloc_height);
     cairo_line_to(cr,view->alloc_width,0);
     cairo_line_to(cr,0,0);
     cairo_stroke(cr); */

  vi = b_element_view_cartesian_get_view_interval (cart, B_AXIS_TYPE_META);

  /* Render the edge */

  if (b_axis_view->draw_edge)
    {
      switch (b_axis_view->pos)
	{

	case B_COMPASS_NORTH:
	  pt1.x = 0;
	  pt1.y = 0;
	  pt2.x = 1;
	  pt2.y = 0;
	  break;

	case B_COMPASS_SOUTH:
	  pt1.x = 0;
	  pt1.y = 1;
	  pt2.x = 1;
	  pt2.y = 1;
	  break;

	case B_COMPASS_EAST:
	  pt1.x = 0;
	  pt1.y = 0;
	  pt2.x = 0;
	  pt2.y = 1;
	  break;

	case B_COMPASS_WEST:
	  pt1.x = 1;
	  pt1.y = 0;
	  pt2.x = 1;
	  pt2.y = 1;
	  break;

	default:
	  g_assert_not_reached ();
	}

      _view_conv (w, &pt1, &pt1);
      _view_conv (w, &pt2, &pt2);

      cairo_save(cr);

      gtk_style_context_add_class(stc,"edge");

      //GdkRGBA color;
      //gtk_style_context_get_color(stc,gtk_style_context_get_state(stc),&color);

      //gdk_cairo_set_source_rgba(cr,&color);

      /* factor of 2 below counters cropping because drawing is done near the edge */
      cairo_set_line_width (cr, 2*b_axis_view->edge_thickness);

      cairo_move_to (cr, pt1.x, pt1.y);
      cairo_line_to (cr, pt2.x, pt2.y);
      cairo_stroke (cr);

      gtk_style_context_remove_class(stc,"edge");
      cairo_restore(cr);
    }

  /* Render our markers */

  am = b_element_view_cartesian_get_axis_markers (cart, B_AXIS_TYPE_META);

  double max_offset = 0;

  for (i = am ? b_axis_markers_size (am) - 1 : -1; i >= 0; --i)
    {
      const BTick *tick;
      gboolean show_tick, show_label;
      double t, length, thickness, label_offset;
      BAnchor anchor;

      tick = b_axis_markers_get (am, i);

      b_axis_view_tick_properties (b_axis_view, tick, &show_tick, &thickness,
                                   &length, &show_label, &label_offset);

      t = b_tick_position (tick);
      t = b_view_interval_conv (vi, t);

      switch (b_axis_view->pos)
	{
	case B_COMPASS_NORTH:
	  pt1.x = t;
	  pt1.y = 0;
	  _view_conv (w, &pt1, &pt1);

	  pt2 = pt1;
	  pt2.y -= length;

	  pt3 = pt2;
	  pt3.y -= label_offset;

	  anchor = ANCHOR_BOTTOM;
	  break;

	case B_COMPASS_SOUTH:
	  pt1.x = t;
	  pt1.y = 1;
	  _view_conv (w, &pt1, &pt1);

	  pt2 = pt1;
	  pt2.y += length;

	  pt3 = pt2;
	  pt3.y += label_offset;

	  anchor = ANCHOR_TOP;
	  break;

	case B_COMPASS_EAST:
	  pt1.x = 0;
	  pt1.y = t;
	  _view_conv (w, &pt1, &pt1);

	  pt2 = pt1;
	  pt2.x += length;

	  pt3 = pt2;
	  pt3.x += label_offset;

	  anchor = ANCHOR_LEFT;
	  break;

	case B_COMPASS_WEST:
	  pt1.x = 1;
	  pt1.y = t;
	  _view_conv (w, &pt1, &pt1);

	  pt2 = pt1;
	  pt2.x -= length;

	  pt3 = pt2;
	  pt3.x -= label_offset;

	  anchor = ANCHOR_RIGHT;
	  break;

	default:
	  g_assert_not_reached ();

	}

      if (show_tick)
      {
        cairo_save(cr);
        gchar *class = NULL;

        switch (b_tick_type (tick))
          {
          case B_TICK_MAJOR:
          case B_TICK_MAJOR_RULE:
            class = "major-ticks";
            break;
          case B_TICK_MINOR:
          case B_TICK_MINOR_RULE:
            class = "minor-ticks";
            break;
          case B_TICK_MICRO:
          case B_TICK_MICRO_RULE:
              class = "minor-ticks";
              break;
          case B_TICK_NONE:
              g_assert_not_reached();
              break;
          default:
            break;
          }

        gtk_style_context_add_class(stc,class);

        //GdkRGBA color;
        //gtk_style_context_get_color(stc,gtk_style_context_get_state(stc),&color);

        //gdk_cairo_set_source_rgba(cr,&color);

        cairo_set_line_width (cr, b_axis_view->major_tick_thickness);
        cairo_move_to (cr, pt1.x, pt1.y);
        cairo_line_to (cr, pt2.x, pt2.y);
        cairo_stroke (cr);

        gtk_style_context_remove_class(stc,class);
        cairo_restore(cr);
      }

      PangoLayout *layout = pango_cairo_create_layout (cr);

      pango_layout_set_font_description (layout, b_axis_view->label_font);

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
              _string_draw_no_rotate (cr, b_axis_view->label_font, pt3, anchor, b_tick_label (tick));

              if (horizontal)
                {
                  if (length + dh + label_offset > max_offset)
                    {
                      max_offset = length + dh + label_offset;
                    }
                }
              else
                {
                  if (length + dw + label_offset > max_offset)
                    {
                      max_offset = length + dw + label_offset;
                    }
                }
            }
        }
        g_object_unref (layout);
    }

  legend = b_axis_view->axis_label;

  if (legend && *legend)
    {
      switch (b_axis_view->pos)
	{
	case B_COMPASS_NORTH:
	  pt1.x = 0.5;
	  pt1.y = 0;
	  _view_conv (w, &pt1, &pt1);
	  pt1.y -= max_offset;
	  _string_draw (cr, b_axis_view->label_font, pt1, ANCHOR_BOTTOM, ROT_0,
		       legend);
	  break;
	case B_COMPASS_SOUTH:
	  pt1.x = 0.5;
	  pt1.y = 1;
	  _view_conv (w, &pt1, &pt1);
	  pt1.y += max_offset;
	  _string_draw (cr, b_axis_view->label_font, pt1, ANCHOR_TOP, ROT_0,
		       legend);
	  break;
	case B_COMPASS_EAST:
	  pt1.x = 0;
	  pt1.y = 0.5;
	  _view_conv (w, &pt1, &pt1);
	  pt1.x += max_offset;
	  _string_draw (cr, b_axis_view->label_font, pt1, ANCHOR_BOTTOM,
		       ROT_270, legend);
	  break;
	case B_COMPASS_WEST:
	  pt1.x = 1;
	  pt1.y = 0.5;
	  _view_conv (w, &pt1, &pt1);
	  pt1.x -= max_offset;
	  _string_draw (cr, b_axis_view->label_font, pt1, ANCHOR_BOTTOM,
		       ROT_90, legend);
	  break;
	default:
	  g_assert_not_reached ();
	}
    }

  /* draw zoom thing */
  if (b_axis_view->zoom_in_progress)
    {
      double z = b_view_interval_conv (vi, b_axis_view->op_start);
      double e = b_view_interval_conv (vi, b_axis_view->cursor_pos);

      switch (b_axis_view->pos)
	{

	case B_COMPASS_NORTH:
	  pt1.x = z;
	  pt1.y = 0;
	  pt2.x = e;
	  pt2.y = 0;
	  break;

	case B_COMPASS_SOUTH:
	  pt1.x = z;
	  pt1.y = 1;
	  pt2.x = e;
	  pt2.y = 1;
	  break;

	case B_COMPASS_EAST:
	  pt1.x = 0;
	  pt1.y = z;
	  pt2.x = 0;
	  pt2.y = e;
	  break;

	case B_COMPASS_WEST:
	  pt1.x = 1;
	  pt1.y = z;
	  pt2.x = 1;
	  pt2.y = e;
	  break;

	default:
	  g_assert_not_reached ();
	}

      _view_conv (w, &pt1, &pt1);
      _view_conv (w, &pt2, &pt2);

      cairo_set_line_width (cr, b_axis_view->edge_thickness);
      cairo_set_source_rgba (cr, 0.0, 0.0, 1.0, 0.25);

      cairo_move_to (cr, pt1.x, pt1.y);
      cairo_line_to (cr, pt2.x, pt2.y);

      BPoint pt0 = pt1;

      switch (b_axis_view->pos)
	{

	case B_COMPASS_NORTH:
	  pt1.x = z;
	  pt1.y = 1;
	  pt2.x = e;
	  pt2.y = 1;
	  break;

	case B_COMPASS_SOUTH:
	  pt1.x = z;
	  pt1.y = 0;
	  pt2.x = e;
	  pt2.y = 0;
	  break;

	case B_COMPASS_EAST:
	  pt1.x = 1;
	  pt1.y = z;
	  pt2.x = 1;
	  pt2.y = e;
	  break;

	case B_COMPASS_WEST:
	  pt1.x = 0;
	  pt1.y = z;
	  pt2.x = 0;
	  pt2.y = e;
	  break;

	default:
	  g_assert_not_reached ();
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
  g_message ("axis view draw %d: %f ms", b_axis_view->pos, te * 1000);
  g_timer_destroy (t);
#endif

  return TRUE;
}

static void
b_axis_view_snapshot (GtkWidget   *w,
                      GtkSnapshot *s)
{
  graphene_rect_t bounds;
  if (gtk_widget_compute_bounds(w,w,&bounds)) {
    cairo_t *cr = gtk_snapshot_append_cairo (s, &bounds);
    axis_view_draw(w, cr);
  }
}

static gboolean
b_axis_view_scroll_event (GtkEventControllerScroll * controller, double dx, double dy, gpointer user_data)
{
  BAxisView *view = (BAxisView *) user_data;

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
b_axis_view_do_popup_menu (GtkWidget * my_widget, GdkEventButton * event)
{
  BAxisView *view = (BAxisView *) my_widget;

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
b_axis_view_press_event (GtkGestureClick *gesture,
               int n_press,
                                 double x, double y,
               gpointer         user_data)
{
  BAxisView *view = (BAxisView *) user_data;
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
      double z = get_horizontal (view) ? ip.x : ip.y;
      view->op_start = b_view_interval_unconv (vi, z);
      view->zoom_in_progress = TRUE;
    }
  else if ((t & GDK_SHIFT_MASK) && b_element_view_get_panning (B_ELEMENT_VIEW (view)))
    {
      double z = get_horizontal (view) ? ip.x : ip.y;

      b_view_interval_recenter_around_point (vi,
					     b_view_interval_unconv (vi,z));
    }
  else if (b_element_view_get_panning (B_ELEMENT_VIEW (view)))
    {
      b_view_interval_set_ignore_preferred_range (vi, TRUE);

      double z = get_horizontal (view) ? ip.x : ip.y;
      view->op_start = b_view_interval_unconv (vi, z);
      /* this is the position where the pan started */

      view->pan_in_progress = TRUE;
    }
  return FALSE;
}

static gboolean
b_axis_view_motion_notify_event (GtkEventControllerMotion *controller, double x, double y, gpointer user_data)
{
  BAxisView *view = (BAxisView *) user_data;

  BViewInterval *vi =
        b_element_view_cartesian_get_view_interval ((BElementViewCartesian *)
						    view,
						    B_AXIS_TYPE_META);

  BPoint ip, evp;
  evp.x = x;
  evp.y = y;

  _view_invconv (GTK_WIDGET(view), &evp, &ip);

  double pos = b_view_interval_unconv (vi, get_horizontal (view) ? ip.x : ip.y);
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
b_axis_view_release_event (GtkGestureClick *gesture,
               int n_press,
                                 double x, double y,
               gpointer         user_data)
{
  BAxisView *view = (BAxisView *) user_data;

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

      double z = get_horizontal (view) ? ip.x : ip.y;
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
b_axis_view_set_property (GObject * object,
			  guint property_id,
			  const GValue * value, GParamSpec * pspec)
{
  BAxisView *self = (BAxisView *) object;

  switch (property_id)
    {
    case AXIS_VIEW_DRAW_EDGE:
      {
	self->draw_edge = g_value_get_boolean (value);
	break;
      }
    case AXIS_VIEW_EDGE_THICKNESS:
      {
	self->edge_thickness = g_value_get_double (value);
	break;
      }
    case AXIS_VIEW_POSITION:
      {
        self->pos = g_value_get_enum (value);
        break;
      }
    case AXIS_VIEW_DRAW_LABEL:
      {
	self->draw_label = g_value_get_boolean (value);
	break;
      }
    case AXIS_VIEW_LABEL_OFFSET:
      {
	self->label_offset = g_value_get_double (value);
	break;
      }
    case AXIS_VIEW_AXIS_LABEL:
      {
	//g_free (self->label);
	self->axis_label = g_value_dup_string (value);
	break;
      }
    case AXIS_VIEW_SHOW_MAJOR_TICKS:
      {
	self->show_major_ticks = g_value_get_boolean (value);
      }
      break;
    case AXIS_VIEW_MAJOR_TICK_THICKNESS:
      {
	self->major_tick_thickness = g_value_get_double (value);
	break;
      }
    case AXIS_VIEW_MAJOR_TICK_LENGTH:
      {
	self->major_tick_length = g_value_get_double (value);
	break;
      }
			case AXIS_VIEW_MINOR_TICK_LENGTH:
	      {
		self->minor_tick_length = g_value_get_double (value);
		break;
	      }
    case AXIS_VIEW_SHOW_MINOR_TICKS:
      {
	self->show_minor_ticks = g_value_get_boolean (value);
      }
      break;
    case AXIS_VIEW_SHOW_MAJOR_LABELS:
      {
	self->show_major_labels = g_value_get_boolean (value);
      }
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
  if (property_id != AXIS_VIEW_POSITION)
    {
      b_element_view_changed (B_ELEMENT_VIEW (self));
    }
}

static void
b_axis_view_get_property (GObject * object,
			  guint property_id,
			  GValue * value, GParamSpec * pspec)
{
  BAxisView *self = (BAxisView *) object;
  switch (property_id)
    {
    case AXIS_VIEW_DRAW_EDGE:
      {
	g_value_set_boolean (value, self->draw_edge);
      }
      break;
    case AXIS_VIEW_EDGE_THICKNESS:
      {
	g_value_set_double (value, self->edge_thickness);
      }
      break;
    case AXIS_VIEW_POSITION:
      {
        g_value_set_enum (value, self->pos);
      }
      break;
    case AXIS_VIEW_DRAW_LABEL:
      {
	g_value_set_boolean (value, self->draw_label);
      }
      break;
    case AXIS_VIEW_LABEL_OFFSET:
      {
	g_value_set_double (value, self->label_offset);
      }
      break;
    case AXIS_VIEW_AXIS_LABEL:
      {
        g_value_set_string (value, self->axis_label);
      }
      break;
    case AXIS_VIEW_SHOW_MAJOR_TICKS:
      {
	g_value_set_boolean (value, self->show_major_ticks);
      }
      break;
    case AXIS_VIEW_MAJOR_TICK_THICKNESS:
      {
	g_value_set_double (value, self->major_tick_thickness);
      }
      break;
    case AXIS_VIEW_MAJOR_TICK_LENGTH:
      {
	g_value_set_double (value, self->major_tick_length);
      }
      break;
			case AXIS_VIEW_MINOR_TICK_LENGTH:
	      {
		g_value_set_double (value, self->minor_tick_length);
	      }
	      break;
    case AXIS_VIEW_SHOW_MINOR_TICKS:
      {
	g_value_set_boolean (value, self->show_minor_ticks);
      }
      break;
    case AXIS_VIEW_SHOW_MAJOR_LABELS:
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
b_axis_view_constructor (GType gtype,
			 guint n_properties,
			 GObjectConstructParam * properties)
{
  GObject *obj;

  {
    obj =
      G_OBJECT_CLASS (parent_class)->constructor (gtype, n_properties,
						  properties);
  }

  /* update the object state depending on constructor properties */

  BAxisView *ax = B_AXIS_VIEW (obj);

  if (ax->pos == B_COMPASS_SOUTH || ax->pos == B_COMPASS_NORTH)
    {
      gtk_widget_set_halign (GTK_WIDGET (obj), GTK_ALIGN_FILL);
      gtk_widget_set_hexpand (GTK_WIDGET (obj), TRUE);
    }
  else
    {
      gtk_widget_set_valign (GTK_WIDGET (obj), GTK_ALIGN_FILL);
      gtk_widget_set_vexpand (GTK_WIDGET (obj), TRUE);
    }

  if (ax->pos == B_COMPASS_SOUTH)
    {
      g_object_set (obj, "valign", GTK_ALIGN_START, NULL);
    }
  else if (ax->pos == B_COMPASS_WEST)
    {
      g_object_set (obj, "halign", GTK_ALIGN_END, NULL);
    }
  else if (ax->pos == B_COMPASS_EAST)
    {
      g_object_set (obj, "halign", GTK_ALIGN_START, NULL);
    }
  else if (ax->pos == B_COMPASS_NORTH)
    {
      g_object_set (obj, "valign", GTK_ALIGN_END, NULL);
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
b_axis_view_class_init (BAxisViewClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->set_property = b_axis_view_set_property;
  object_class->get_property = b_axis_view_get_property;
  object_class->constructor = b_axis_view_constructor;
  object_class->finalize = axis_view_finalize;

  BElementViewClass *view_class = B_ELEMENT_VIEW_CLASS (klass);

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->snapshot = b_axis_view_snapshot;
  widget_class->measure = axis_view_measure;
  widget_class->get_request_mode = axis_view_get_request_mode;
  //widget_class->get_preferred_width = get_preferred_width;
  //widget_class->get_preferred_height = get_preferred_height;

  parent_class = g_type_class_peek_parent (klass);

  /* properties */

  g_object_class_install_property (object_class, AXIS_VIEW_DRAW_EDGE,
				   g_param_spec_boolean ("draw-edge",
							 "Draw Edge",
							 "Whether to draw the axis edge next to the main plot",
							 DEFAULT_DRAW_EDGE,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, AXIS_VIEW_EDGE_THICKNESS,
				   g_param_spec_double ("edge-thickness",
							"Edge Thickness",
							"The thickness of the axis edge in pixels",
							0, 10,
							DEFAULT_LINE_THICKNESS,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, AXIS_VIEW_POSITION,
				   g_param_spec_enum ("position",
						     "Axis position",
						     "The position of the axis with respect to a plot",
						     B_TYPE_COMPASS, B_COMPASS_WEST,
						     G_PARAM_READWRITE |
						     G_PARAM_CONSTRUCT_ONLY |
						     G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, AXIS_VIEW_DRAW_LABEL,
				   g_param_spec_boolean ("draw-label",
							 "Draw Label",
							 "Whether to draw an axis label",
							 DEFAULT_DRAW_LABEL,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, AXIS_VIEW_LABEL_OFFSET,
				   g_param_spec_double ("label-offset",
							"Label Offset",
							"The gap between ticks and labels in pixels",
							0, 10, 2,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, AXIS_VIEW_AXIS_LABEL,
				   g_param_spec_string ("axis-label",
							"Axis Label",
							"Label for the axis", "",
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, AXIS_VIEW_SHOW_MAJOR_TICKS,
				   g_param_spec_boolean ("show-major-ticks",
							 "Show major ticks",
							 "Whether to draw major ticks",
							 DEFAULT_SHOW_MAJOR_TICKS,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
				   AXIS_VIEW_MAJOR_TICK_THICKNESS,
				   g_param_spec_double
				   ("major-tick-thickness",
				    "Major Tick Thickness",
				    "The thickness of major ticks in pixels",
				    0, 10, DEFAULT_LINE_THICKNESS,
				    G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
				    G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, AXIS_VIEW_MAJOR_TICK_LENGTH,
				   g_param_spec_double ("major-tick-length",
							"Major Tick Length",
							"The length of major ticks in pixels",
							0, 100, 5.0,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, AXIS_VIEW_SHOW_MINOR_TICKS,
				   g_param_spec_boolean ("show-minor-ticks",
							 "Show minor ticks",
							 "Whether to draw minor ticks",
							 DEFAULT_SHOW_MINOR_TICKS,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class, AXIS_VIEW_MINOR_TICK_LENGTH,
						g_param_spec_double ("minor-tick-length",
						 							"Minor Tick Length",
						 							"The length of minor ticks in pixels",
						 							0, 100, 3.0,
						 							G_PARAM_READWRITE |
						 							G_PARAM_CONSTRUCT |
						 							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, AXIS_VIEW_SHOW_MAJOR_LABELS,
				   g_param_spec_boolean ("show-major-labels",
							 "Show major labels",
							 "Whether to draw labels next to major ticks",
							 DEFAULT_SHOW_MAJOR_LABELS,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

  view_class->changed = changed;

	gtk_widget_class_set_css_name (widget_class, "axis");
}

static void
b_axis_view_init (BAxisView * obj)
{
  GtkStyleContext *stc;
  GtkCssProvider *cssp = gtk_css_provider_new ();
  gchar *css = g_strdup_printf ("axis.edge { color:%s; } axis.major-ticks { color:%s; } axis.minor-ticks { color:%s; }","#000000","#000000","#000000");
  gtk_css_provider_load_from_data (cssp, css, -1);
  stc = gtk_widget_get_style_context (GTK_WIDGET (obj));
  gtk_style_context_add_provider (stc, GTK_STYLE_PROVIDER (cssp),
                                  GTK_STYLE_PROVIDER_PRIORITY_THEME);
  g_free (css);

  obj->label_font = pango_font_description_from_string ("Sans 12");

  GtkEventController *motion_controller = gtk_event_controller_motion_new();
  gtk_widget_add_controller(GTK_WIDGET(obj), motion_controller);

  g_signal_connect(motion_controller, "motion", G_CALLBACK(b_axis_view_motion_notify_event), obj);

  GtkGesture *click_controller = gtk_gesture_click_new();
  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click_controller),1);
  gtk_widget_add_controller(GTK_WIDGET(obj), GTK_EVENT_CONTROLLER(click_controller));

  g_signal_connect(click_controller, "pressed", G_CALLBACK(b_axis_view_press_event), obj);
  g_signal_connect(click_controller, "released", G_CALLBACK(b_axis_view_release_event), obj);

  GtkEventController *scroll_controller = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
  gtk_widget_add_controller(GTK_WIDGET(obj), GTK_EVENT_CONTROLLER(scroll_controller));

  g_signal_connect(scroll_controller, "scroll", G_CALLBACK(b_axis_view_scroll_event), obj);

  b_element_view_cartesian_add_view_interval ((BElementViewCartesian *) obj,
					      B_AXIS_TYPE_META);
}

/**
 * b_axis_view_new:
 * @t: axis type
 *
 * Convenience function to create a new #BAxisView.
 *
 * Returns: the position of the axis (placement of axis with respect to its
 * plot view).
 **/
BAxisView *
b_axis_view_new (BCompass t)
{
  BAxisView *a = g_object_new (B_TYPE_AXIS_VIEW, "position", t, NULL);

  return a;
}

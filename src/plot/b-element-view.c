/*
 * b-element-view.c
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

#include "plot/b-element-view.h"
#include <math.h>
#include <string.h>

/**
 * SECTION: b-element-view
 * @short_description: Base class for plot objects.
 *
 * Abstract base class for plot classes #BScatterLineView, #BAxisView, and others.
 *
 */

typedef struct
{
  gint freeze_count;
  GtkLabel *status_label;

  gboolean pending_change;
  gboolean zooming;
  gboolean panning;
} BElementViewPrivate;

enum
{
  CHANGED,
  LAST_SIGNAL
};

static guint view_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (BElementView, b_element_view,
				     GTK_TYPE_DRAWING_AREA);

static void
b_element_view_finalize (GObject * obj)
{
  g_debug ("finalizing b_element_view");

  BElementView *v = (BElementView *) obj;
  BElementViewPrivate *p = b_element_view_get_instance_private (v);

  g_clear_object(&p->status_label);

  GObjectClass *obj_class = G_OBJECT_CLASS (b_element_view_parent_class);

  if (obj_class->finalize)
    obj_class->finalize (obj);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
changed (BElementView * view)
{
  gtk_widget_queue_draw (GTK_WIDGET (view));
}

/**
 * b_element_view_changed :
 * @view: #BElementView
 *
 * Causes the #BElementView to emit a "changed" signal, unless it is frozen,
 * in which case the signal will be emitted when the view is thawed.
 **/
void
b_element_view_changed (BElementView * view)
{
  BElementViewPrivate *p;

  g_return_if_fail (B_IS_ELEMENT_VIEW (view));

  p = b_element_view_get_instance_private (view);

  if (p->freeze_count > 0)
    p->pending_change = TRUE;
  else
    {
      g_signal_emit (view, view_signals[CHANGED], 0);
      p->pending_change = FALSE;
    }
}

/**
 * b_element_view_freeze :
 * @view: #BElementView
 *
 * Freezes a #BElementView, so that it no longer emits a "changed" signal when
 * b_element_view_changed() is called. Multiple calls of this function will
 * increase the freeze count. If b_element_view_changed() is called while the
 * view is frozen, it will emit a "changed" signal when the freeze count drops
 * to zero.
 **/
void
b_element_view_freeze (BElementView * view)
{
  BElementViewClass *klass;

  g_return_if_fail (B_IS_ELEMENT_VIEW (view));

  BElementViewPrivate *p = b_element_view_get_instance_private (view);

  g_return_if_fail (p->freeze_count >= 0);
  ++p->freeze_count;

  klass = B_ELEMENT_VIEW_CLASS (G_OBJECT_GET_CLASS (view));
  if (klass->freeze)
    klass->freeze (view);
}

/**
 * b_element_view_thaw :
 * @view: #BElementView
 *
 * Decreases the freeze count of a #BElementView. See b_element_view_freeze()
 * for details.
 **/
void
b_element_view_thaw (BElementView * view)
{
  BElementViewClass *klass;

  g_return_if_fail (B_IS_ELEMENT_VIEW (view));

  BElementViewPrivate *p = b_element_view_get_instance_private (view);

  if (p->freeze_count <= 0)
    return;

  --p->freeze_count;

  klass = B_ELEMENT_VIEW_CLASS (G_OBJECT_GET_CLASS (view));
  if (klass->thaw)
    klass->thaw (view);

  if (p->freeze_count == 0)
    {
      if (p->pending_change)
        b_element_view_changed (view);
    }
}

static gboolean
ev_leave_notify (GtkWidget *w, GdkEventCrossing *ev)
{
  BElementView *v = (BElementView *)w;
  BElementViewPrivate *p = b_element_view_get_instance_private (v);

  if(p->status_label)
    gtk_label_set_text(p->status_label,"");

  return FALSE;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
b_element_view_class_init (BElementViewClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;

  klass->changed = changed;

  object_class->finalize = b_element_view_finalize;

  widget_class->leave_notify_event = ev_leave_notify;

  view_signals[CHANGED] =
    g_signal_new ("changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BElementViewClass, changed),
		  NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
b_element_view_init (BElementView * view)
{
  gtk_widget_add_events (GTK_WIDGET (view), GDK_LEAVE_NOTIFY_MASK);
}

/* functions for converting between view coordinates (0 to 1) and Widget
 * coordinates */
void
_view_conv (GtkWidget * widget, const BPoint * t, BPoint * p)
{
  int width = gtk_widget_get_allocated_width (widget);
  int height = gtk_widget_get_allocated_height (widget);

  p->x = t->x * width;
  p->y = (1.0 - t->y) * height;
}

void
_view_invconv (GtkWidget * widget, const BPoint * t, BPoint * p)
{
  g_return_if_fail (t != NULL);
  g_return_if_fail (p != NULL);

  int width = gtk_widget_get_allocated_width (widget);
  int height = gtk_widget_get_allocated_height (widget);

  p->x = (t->x) / width;
  p->y = 1.0 - (t->y) / height;
}

void
_view_conv_bulk (GtkWidget * widget, const BPoint * t, BPoint * p, gsize N)
{
  double w, h;
  gsize i;

  g_return_if_fail (t != NULL);
  g_return_if_fail (p != NULL);

  int width = gtk_widget_get_allocated_width (widget);
  int height = gtk_widget_get_allocated_height (widget);

  w = width;
  h = height;

  for (i = 0; i < N; ++i)
    {
      p[i].x = t[i].x * w;
      p[i].y = (1.0 - t[i].y) * h;
    }
}

/**
 * b_element_view_set_status_label :
 * @v: a #BScatterLineView
 * @status_label: a #GtkLabel
 *
 * Connect a label to the view that will show status information, such as
 * the coordinates of the pointer.
 **/
void
b_element_view_set_status_label (BElementView * v, GtkLabel * status_label)
{
  BElementViewPrivate *p = b_element_view_get_instance_private (v);
  p->status_label = g_object_ref (status_label);
}

/**
 * b_element_view_get_status_label :
 * @v: a #BScatterLineView
 *
 * Get the status label.
 *
 * Returns: (transfer none): the label
 **/
GtkLabel * b_element_view_get_status_label(BElementView *v)
{
  BElementViewPrivate *p = b_element_view_get_instance_private (v);
  return p->status_label;
}

/**
 * b_element_view_set_status :
 * @v: a #BScatterLineView
 * @status: a string
 *
 * Set the string to be displayed on the status label, if present.
 **/
void
b_element_view_set_status (BElementView * v, const gchar *status)
{
  BElementViewPrivate *p = b_element_view_get_instance_private (v);
  if(p->status_label)
    {
      gtk_label_set_markup (p->status_label, status);
    }
}

/**
 * b_element_view_set_zooming :
 * @view: #BElementView
 * @b: %TRUE or %FALSE
 *
 * If @b is true, set @v to zoom mode, where the view interval(s) can be increased
 * or decreased using the mouse.
 **/
void
b_element_view_set_zooming (BElementView * view, gboolean b)
{
  BElementViewPrivate *p = b_element_view_get_instance_private (view);
  p->zooming = b;
  GtkWidget *w = (GtkWidget *) view;
  GdkWindow *window = gtk_widget_get_window (w);
  GdkCursor *cursor = NULL;
  if(p->zooming)
    {
      GdkDisplay *display = gtk_widget_get_display (w);
      cursor = gdk_cursor_new_from_name (display, "crosshair");
    }
  gdk_window_set_cursor (window, cursor);
}

/**
 * b_element_view_set_panning :
 * @view: #BElementView
 * @b: %TRUE or %FALSE
 *
 * If @b is true, set @v to pan mode, where the view interval(s) can be translated
 * using the mouse.
 **/
void
b_element_view_set_panning (BElementView * view, gboolean b)
{
  BElementViewPrivate *p = b_element_view_get_instance_private (view);
  p->panning = b;
  GtkWidget *w = (GtkWidget *) view;
  GdkWindow *window = gtk_widget_get_window (w);
  GdkCursor *cursor = NULL;
  if(p->panning)
    {
      GdkDisplay *display = gtk_widget_get_display (w);
      cursor = gdk_cursor_new_from_name (display, "grab");
    }
  gdk_window_set_cursor (window, cursor);
}

/**
 * b_element_view_get_zooming :
 * @view: #BElementView
 *
 * Return %TRUE if the #BElementView is in zoom mode.
 *
 * Returns: a boolean
 **/
gboolean
b_element_view_get_zooming (BElementView * view)
{
  BElementViewPrivate *p = b_element_view_get_instance_private (view);
  return p->zooming;
}

/**
 * b_element_view_get_panning :
 * @view: #BElementView
 *
 * Return %TRUE if the #BElementView is in pan mode.
 *
 * Returns: a boolean
 **/
gboolean
b_element_view_get_panning (BElementView * view)
{
  BElementViewPrivate *p = b_element_view_get_instance_private (view);
  return p->panning;
}

BPoint _view_event_point (GtkWidget *widget, GdkEvent *event)
{
  BPoint evp, ip;
  gboolean found = gdk_event_get_coords(event,&evp.x,&evp.y);
  g_return_val_if_fail(found,evp);

  _view_invconv (widget, &evp, &ip);
  return ip;
}

/************************************/
/* internally used functions for drawing strings */

void
_string_draw (cairo_t * context, PangoFontDescription * font,
	     const BPoint position, BAnchor anchor, BRotation rot,
	     const char *string)
{
  PangoLayout *layout;

  cairo_close_path (context);

  cairo_save (context);

  layout = pango_cairo_create_layout (context);
  pango_layout_set_markup (layout, string, -1);
  pango_layout_set_font_description (layout, font);

  int pwidth = 0;
  int pheight = 0;
  pango_layout_get_pixel_size (layout, &pwidth, &pheight);
  if (pwidth == 0 || pheight == 0)
    return;

  double width = (double) pwidth;
  double height = (double) pheight;

  double dx = 0;
  double dy = 0;

  switch (anchor)
    {
    case ANCHOR_TOP:
      dx = -width / 2;
      break;

    case ANCHOR_UPPER_LEFT:
      /* do nothing */
      break;

    case ANCHOR_LEFT:
      dy = -height / 2;
      break;

    case ANCHOR_LOWER_LEFT:
      dy = -height;
      break;

    case ANCHOR_BOTTOM:
      dx = -width / 2;
      dy = -height;
      break;

    case ANCHOR_LOWER_RIGHT:
      dx = -width;
      dy = -height;
      break;

    case ANCHOR_RIGHT:
      dx = -width;
      dy = -height / 2;
      break;

    case ANCHOR_UPPER_RIGHT:
      dx = -width;
      break;

    case ANCHOR_CENTER:
      dx = -width / 2;
      dy = -height / 2;
      break;

    default:
      break;
    }

  cairo_translate (context, position.x, position.y);

  if (rot == ROT_90)
    {
      cairo_rotate (context, -90 * G_PI / 180);
      pango_cairo_update_layout (context, layout);
    }
  else if (rot == ROT_270)
    {
      cairo_rotate (context, -270 * G_PI / 180);
      pango_cairo_update_layout (context, layout);
    }

  cairo_translate (context, dx, dy);

  pango_cairo_show_layout (context, layout);

  g_object_unref (layout);

  cairo_restore (context);
}

G_GNUC_INTERNAL
void
_string_draw_no_rotate (cairo_t * context, const BPoint position, BAnchor anchor,
		       PangoLayout * layout)
{
  cairo_save (context);

  int pwidth = 0;
  int pheight = 0;
  pango_layout_get_pixel_size (layout, &pwidth, &pheight);
  if (pwidth == 0 || pheight == 0)
    return;

  double width = (double) pwidth;
  double height = (double) pheight;

  double dx = 0;
  double dy = 0;

  switch (anchor)
    {
    case ANCHOR_TOP:
      dx = -width / 2;
      break;

    case ANCHOR_UPPER_LEFT:
      /* do nothing */
      break;

    case ANCHOR_LEFT:
      dy = -height / 2;
      break;

    case ANCHOR_LOWER_LEFT:
      dy = -height;
      break;

    case ANCHOR_BOTTOM:
      dx = -width / 2;
      dy = -height;
      break;

    case ANCHOR_LOWER_RIGHT:
      dx = -width;
      dy = -height;
      break;

    case ANCHOR_RIGHT:
      dx = -width;
      dy = -height / 2;
      break;

    case ANCHOR_UPPER_RIGHT:
      dx = -width;
      break;

    case ANCHOR_CENTER:
      dx = -width / 2;
      dy = -height / 2;
      break;

    default:
      break;
    }

  double cx = position.x + dx;
  double cy = position.y + dy;

  cairo_translate (context, cx, cy);

  pango_cairo_show_layout (context, layout);

  cairo_restore (context);
}

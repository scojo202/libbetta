/*
 * y-element-view.c
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

#include "plot/y-element-view.h"
#include <math.h>
#include <string.h>

/**
 * SECTION: y-element-view
 * @short_description: Base class for plot objects.
 *
 * Abstract base class for plot classes #YScatterLineView, #YAxisView, and others.
 *
 */

typedef struct
{
  gint freeze_count;

  gboolean pending_change;
  gboolean zooming;
  gboolean panning;
} YElementViewPrivate;

enum
{
  CHANGED,
  LAST_SIGNAL
};

static guint view_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (YElementView, y_element_view,
				     GTK_TYPE_DRAWING_AREA);

static void
y_element_view_finalize (GObject * obj)
{
  g_debug ("finalizing y_element_view");

  GObjectClass *obj_class = G_OBJECT_CLASS (y_element_view_parent_class);

  if (obj_class->finalize)
    obj_class->finalize (obj);
}


/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
changed (YElementView * view)
{
  gtk_widget_queue_draw (GTK_WIDGET (view));
}

/**
 * y_element_view_changed :
 * @view: #YElementView
 *
 * Causes the #YElementView to emit a "changed" signal, unless it is frozen,
 * in which case the signal will be emitted when the view is thawed.
 **/
void
y_element_view_changed (YElementView * view)
{
  YElementViewPrivate *p;

  g_return_if_fail (Y_IS_ELEMENT_VIEW (view));

  p = y_element_view_get_instance_private (view);

  if (p->freeze_count > 0)
    p->pending_change = TRUE;
  else
    {
      g_signal_emit (view, view_signals[CHANGED], 0);
      p->pending_change = FALSE;
    }
}

/**
 * y_element_view_freeze :
 * @view: #YElementView
 *
 * Freezes a #YElementView, so that it no longer emits a "changed" signal when
 * y_element_view_changed() is called. Multiple calls of this function will
 * increase the freeze count. If y_element_view_changed() is called while the
 * view is frozen, it will emit a "changed" signal when the freeze count drops
 * to zero.
 **/
void
y_element_view_freeze (YElementView * view)
{
  YElementViewClass *klass;

  g_return_if_fail (Y_IS_ELEMENT_VIEW (view));

  YElementViewPrivate *p = y_element_view_get_instance_private (view);

  g_return_if_fail (p->freeze_count >= 0);
  ++p->freeze_count;

  klass = Y_ELEMENT_VIEW_CLASS (G_OBJECT_GET_CLASS (view));
  if (klass->freeze)
    klass->freeze (view);
}

/**
 * y_element_view_thaw :
 * @view: #YElementView
 *
 * Decreases the freeze count of a #YElementView. See y_element_view_freeze()
 * for details.
 **/
void
y_element_view_thaw (YElementView * view)
{
  YElementViewClass *klass;

  g_return_if_fail (Y_IS_ELEMENT_VIEW (view));

  YElementViewPrivate *p = y_element_view_get_instance_private (view);

  if (p->freeze_count <= 0)
    return;

  --p->freeze_count;

  klass = Y_ELEMENT_VIEW_CLASS (G_OBJECT_GET_CLASS (view));
  if (klass->thaw)
    klass->thaw (view);

  if (p->freeze_count == 0)
    {
      if (p->pending_change)
        y_element_view_changed (view);
    }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
y_element_view_class_init (YElementViewClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  klass->changed = changed;

  object_class->finalize = y_element_view_finalize;

  view_signals[CHANGED] =
    g_signal_new ("changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (YElementViewClass, changed),
		  NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
y_element_view_init (YElementView * view)
{
}

/* functions for converting between view coordinates (0 to 1) and Widget
 * coordinates */
void
_view_conv (GtkWidget * widget, const YPoint * t, YPoint * p)
{
  int width = gtk_widget_get_allocated_width (widget);
  int height = gtk_widget_get_allocated_height (widget);

  p->x = t->x * width;
  p->y = (1 - t->y) * height;
}

void
_view_invconv (GtkWidget * widget, const YPoint * t, YPoint * p)
{
  g_return_if_fail (t != NULL);
  g_return_if_fail (p != NULL);

  int width = gtk_widget_get_allocated_width (widget);
  int height = gtk_widget_get_allocated_height (widget);

  p->x = (t->x) / width;
  p->y = 1 - (t->y) / height;
}

void
_view_conv_bulk (GtkWidget * widget, const YPoint * t, YPoint * p, gsize N)
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
      p[i].y = (1 - t[i].y) * h;
    }
}

/**
 * y_element_view_set_zooming :
 * @view: #YElementView
 * @b: #TRUE or #FALSE
 *
 * If @b is true, set @v to zoom mode, where the view interval(s) can be increased
 * or decreased using the mouse.
 **/
void
y_element_view_set_zooming (YElementView * view, gboolean b)
{
  YElementViewPrivate *p = y_element_view_get_instance_private (view);
  p->zooming = b;
}

/**
 * y_element_view_set_panning :
 * @view: #YElementView
 * @b: %TRUE or %FALSE
 *
 * If @b is true, set @v to pan mode, where the view interval(s) can be translated
 * using the mouse.
 **/
void
y_element_view_set_panning (YElementView * view, gboolean b)
{
  YElementViewPrivate *p = y_element_view_get_instance_private (view);
  p->panning = b;
}

/**
 * y_element_view_get_zooming :
 * @view: #YElementView
 *
 * Return %TRUE if the #YElementView is in zoom mode.
 *
 * Returns: a boolean
 **/
gboolean
y_element_view_get_zooming (YElementView * view)
{
  YElementViewPrivate *p = y_element_view_get_instance_private (view);
  return p->zooming;
}

/**
 * y_element_view_get_panning :
 * @view: #YElementView
 *
 * Return %TRUE if the #YElementView is in pan mode.
 *
 * Returns: a boolean
 **/
gboolean
y_element_view_get_panning (YElementView * view)
{
  YElementViewPrivate *p = y_element_view_get_instance_private (view);
  return p->panning;
}

/************************************/
/* internally used functions for drawing strings */
void
_string_draw (cairo_t * context, PangoFontDescription * font,
	     const YPoint position, YAnchor anchor, YRotation rot,
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
_string_draw_no_rotate (cairo_t * context, const YPoint position, YAnchor anchor,
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

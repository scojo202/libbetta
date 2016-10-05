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

#include "y-element-view.h"
#include <math.h>
#include <string.h>

/* provides "changed" signal (could be replaced with "queue draw"),
   freeze/thaw (do we need this?),
 */

typedef struct _YElementViewPrivate YElementViewPrivate;
struct _YElementViewPrivate {
  gint freeze_count;

  gboolean pending_change;
};

#define priv(x) ((x)->priv)

static GObjectClass *parent_class = NULL;

enum {
  CHANGED,
  LAST_SIGNAL
};

static guint view_signals[LAST_SIGNAL] = { 0 };

static void
y_element_view_finalize (GObject *obj)
{
  YElementView *view = Y_ELEMENT_VIEW (obj);
  YElementViewPrivate *p = priv (view);
  
  g_debug("finalizing y_element_view");

  g_slice_free (YElementViewPrivate, view->priv);

  if (parent_class->finalize)
    parent_class->finalize (obj);
}


/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static
void changed (YElementView *view)
{
  gtk_widget_queue_draw(GTK_WIDGET(view));
}

void
y_element_view_changed (YElementView *view)
{
  YElementViewPrivate *p;

  g_return_if_fail (Y_IS_ELEMENT_VIEW (view));

  p = priv (view);

  if (p->freeze_count > 0)
    p->pending_change = TRUE;
  else {
    g_signal_emit (view, view_signals[CHANGED], 0);
    p->pending_change = FALSE;
  }
}

void
y_element_view_freeze (YElementView *view)
{
  YElementViewClass *klass;

  g_return_if_fail (Y_IS_ELEMENT_VIEW (view));

  g_return_if_fail (view->priv->freeze_count >= 0);
  ++view->priv->freeze_count;

  klass = Y_ELEMENT_VIEW_CLASS (G_OBJECT_GET_CLASS (view));
  if (klass->freeze)
    klass->freeze (view);
}

void
y_element_view_thaw (YElementView *view)
{
  YElementViewClass *klass;

  g_return_if_fail (Y_IS_ELEMENT_VIEW (view));

  if(view->priv->freeze_count <= 0)
    return;

  --view->priv->freeze_count;

  klass = Y_ELEMENT_VIEW_CLASS (G_OBJECT_GET_CLASS (view));
  if (klass->thaw)
    klass->thaw (view);

  if (view->priv->freeze_count == 0) {

    if (view->priv->pending_change)
      y_element_view_changed (view);
  }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
y_element_view_class_init (YElementViewClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  klass->changed = changed;

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = y_element_view_finalize;

  view_signals[CHANGED] =
    g_signal_new ("changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (YElementViewClass, changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
}

static void
y_element_view_init (YElementView *view)
{
  view->draw_pending = FALSE;

  view->used_width = -1;
  view->used_height = -1;

  YElementViewPrivate *p;

  p = view->priv = g_slice_new0 (YElementViewPrivate);
}

G_DEFINE_ABSTRACT_TYPE (YElementView, y_element_view, GTK_TYPE_DRAWING_AREA);

void
view_conv (GtkWidget *widget,
			 const Point   *t,
			 Point         *p)
{
  int width = gtk_widget_get_allocated_width(widget);
  int height = gtk_widget_get_allocated_height(widget);

  p->x = t->x * width;
  p->y = (1-t->y) * height;
}

void
view_invconv (GtkWidget *widget,
			 const Point   *t,
			 Point         *p)
{
  g_return_if_fail (t != NULL);
  g_return_if_fail (p != NULL);

  int width = gtk_widget_get_allocated_width(widget);
  int height = gtk_widget_get_allocated_height(widget);

  p->x = (t->x)/width;
  p->y = 1 - (t->y)/height;
}

void
view_conv_bulk (GtkWidget *widget,
			      const Point   *t,
			      Point         *p,
			      gsize             N)
{
  double x0, y0, w, h;
  gsize i;

  g_return_if_fail (t != NULL);
  g_return_if_fail (p != NULL);

  int width = gtk_widget_get_allocated_width(widget);
  int height = gtk_widget_get_allocated_height(widget);

  w = width;
  h = height;

  for (i = 0; i < N; ++i) {
    p[i].x = t[i].x * w;
    p[i].y = (1-t[i].y) * h;
  }
}

void
string_draw (cairo_t * context, PangoFontDescription *font, const Point position, Anchor anchor, Rotation rot, const char *string)
{
  PangoLayout * layout;

  cairo_close_path(context);

  cairo_save (context);

  layout = pango_cairo_create_layout(context);
  pango_layout_set_markup (layout, string, -1);
  pango_layout_set_font_description(layout,font);

  int pwidth=0;
  int pheight=0;
  pango_layout_get_pixel_size(layout, &pwidth, &pheight);
  if (pwidth == 0 || pheight == 0)
    return;

  double width = (double) pwidth;
  double height = (double) pheight;

  double dx=0;
  double dy=0;

  switch (anchor) {
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

  if (rot == ROT_90) {
    cairo_rotate (context, -90 * G_PI/180);
    pango_cairo_update_layout(context,layout);
  }
  else if (rot == ROT_270) {
    cairo_rotate (context, -270 * G_PI/180);
    pango_cairo_update_layout (context,layout);
  }

  cairo_translate(context,dx,dy);

  pango_cairo_show_layout (context, layout);

  g_object_unref(layout);

  cairo_restore(context);
}

void
string_draw_no_rotate (cairo_t * context, const Point position, Anchor anchor, PangoLayout *layout)
{
  cairo_save (context);

  int pwidth=0;
  int pheight=0;
  pango_layout_get_pixel_size(layout, &pwidth, &pheight);
  if (pwidth == 0 || pheight == 0)
    return;

  double width = (double) pwidth;
  double height = (double) pheight;

  double dx=0;
  double dy=0;

  switch (anchor) {
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

  cairo_restore(context);
}

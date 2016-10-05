/*
 * y-element-view.h
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

#include <gtk/gtk.h>

#ifndef _INC_ELEMENT_VIEW_H
#define _INC_ELEMENT_VIEW_H

G_BEGIN_DECLS

typedef enum {
  ROT_0,
  ROT_90,
  ROT_180,
  ROT_270
} Rotation;

typedef enum {
  ANCHOR_TOP,
  ANCHOR_UPPER_LEFT,
  ANCHOR_LEFT,
  ANCHOR_LOWER_LEFT,
  ANCHOR_BOTTOM,
  ANCHOR_LOWER_RIGHT,
  ANCHOR_RIGHT,
  ANCHOR_UPPER_RIGHT,
  ANCHOR_CENTER
} Anchor;

typedef struct _Point Point;

struct _Point {
  double x, y;
};

typedef struct _YElementView YElementView;
typedef struct _YElementViewClass YElementViewClass;
struct _YElementViewPrivate;

struct _YElementView {
  GtkDrawingArea parent;
  
  int used_width, used_height;
  gboolean draw_pending;
  struct _YElementViewPrivate *priv;
};

struct _YElementViewClass {
  GtkDrawingAreaClass parent_class;

  /* VTable */

  /* Freeze/thaw */

  void (*freeze)       (YElementView *);
  void (*thaw)         (YElementView *);

  /* Signals */
  void (*changed)           (YElementView *);
};

#define Y_TYPE_ELEMENT_VIEW (y_element_view_get_type())
#define Y_ELEMENT_VIEW(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),Y_TYPE_ELEMENT_VIEW,YElementView))
#define Y_ELEMENT_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),Y_TYPE_ELEMENT_VIEW,YElementViewClass))
#define Y_IS_ELEMENT_VIEW(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), Y_TYPE_ELEMENT_VIEW))
#define Y_IS_ELEMENT_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), Y_TYPE_ELEMENT_VIEW))

GType y_element_view_get_type (void);

void y_element_view_changed (YElementView *);
void y_element_view_freeze  (YElementView *);
void y_element_view_thaw    (YElementView *);

void y_element_view_set_debug_bg_color (YElementView *, guint32);

#define y_refcounting_assign(x, y) \
{ if ((x) != (y)) { g_object_ref((y)); if(x) g_object_unref((x)); (x) = (y); } }
#define y_unref0(x) { if(x) g_object_unref((x)); (x) = NULL; }

void string_draw (cairo_t * context, PangoFontDescription *font, const Point position, Anchor anchor, Rotation rot, const char *string);
void
string_draw_no_rotate (cairo_t * context, const Point position, Anchor anchor, PangoLayout *layout);

void view_conv      (GtkWidget *, const Point *t, Point *p);
void view_conv_bulk (GtkWidget *view, const Point   *t, Point         *p, gsize             N);

void view_invconv (GtkWidget *view, const Point *t, Point *p);

G_END_DECLS

#endif


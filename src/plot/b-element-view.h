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
} YRotation;

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
} YAnchor;

/**
 * YPoint:
 *
 * Coordinates on a plane.
 **/

typedef struct _YPoint YPoint;

struct _YPoint {
  double x, y;
};

G_DECLARE_DERIVABLE_TYPE(YElementView, y_element_view, Y, ELEMENT_VIEW, GtkDrawingArea)

#define Y_TYPE_ELEMENT_VIEW (y_element_view_get_type())

/**
 * YElementViewClass:
 * @base: base class
 * @freeze: method that gets called by y_element_view_freeze()
 * @thaw: method that gets called by y_element_view_thaw()
 * @changed: default handler for "changed" signal
 *
 * Abstract base class for views, which form the elements of plots.
 **/

struct _YElementViewClass {
    GtkDrawingAreaClass base;

    /* VTable */

    /* Freeze/thaw */

    void (*freeze)       (YElementView *view);
    void (*thaw)         (YElementView *view);

    /* Signals */
    void (*changed)           (YElementView *view);
};

void y_element_view_changed (YElementView *view);
void y_element_view_freeze  (YElementView *view);
void y_element_view_thaw    (YElementView *view);

void y_element_view_set_status_label(YElementView *v, GtkLabel *status_label);
GtkLabel * y_element_view_get_status_label(YElementView *v);
void y_element_view_set_status(YElementView *v, const gchar *status);

void y_element_view_set_zooming (YElementView *view, gboolean b);
void y_element_view_set_panning (YElementView *view, gboolean b);
gboolean y_element_view_get_zooming (YElementView *view);
gboolean y_element_view_get_panning (YElementView *view);

void _string_draw (cairo_t * context, PangoFontDescription *font, const YPoint position, YAnchor anchor, YRotation rot, const char *string);

void
_string_draw_no_rotate (cairo_t * context, const YPoint position, YAnchor anchor, PangoLayout *layout);

void _view_conv      (GtkWidget *view, const YPoint *t, YPoint *p);
void _view_conv_bulk (GtkWidget *view, const YPoint *t, YPoint *p, gsize N);

void _view_invconv (GtkWidget *view, const YPoint *t, YPoint *p);

YPoint _view_event_point (GtkWidget *widget, GdkEvent *event);

G_END_DECLS

#endif

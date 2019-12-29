/*
 * b-element-view.h
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

#pragma once

G_BEGIN_DECLS

typedef enum {
  ROT_0,
  ROT_90,
  ROT_180,
  ROT_270
} BRotation;

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
} BAnchor;

/**
 * BPoint:
 * @x: x coordinate
 * @y: y coordinate
 *
 * Coordinates on a plane.
 **/

typedef struct _BPoint BPoint;

struct _BPoint {
  double x, y;
};

G_DECLARE_DERIVABLE_TYPE(BElementView, b_element_view, B, ELEMENT_VIEW, GtkDrawingArea)

#define B_TYPE_ELEMENT_VIEW (b_element_view_get_type())

/**
 * BElementViewClass:
 * @base: base class
 * @freeze: method that gets called by b_element_view_freeze()
 * @thaw: method that gets called by b_element_view_thaw()
 * @changed: default handler for "changed" signal
 *
 * Abstract base class for views, which form the elements of plots.
 **/

struct _BElementViewClass {
    GtkDrawingAreaClass base;

    /* VTable */

    /* Freeze/thaw */

    void (*freeze)       (BElementView *view);
    void (*thaw)         (BElementView *view);

    /* Signals */
    void (*changed)           (BElementView *view);
};

void b_element_view_changed (BElementView *view);
void b_element_view_freeze  (BElementView *view);
void b_element_view_thaw    (BElementView *view);

void b_element_view_set_status_label(BElementView *v, GtkLabel *status_label);
GtkLabel * b_element_view_get_status_label(BElementView *v);
void b_element_view_set_status(BElementView *v, const gchar *status);

void b_element_view_set_zooming (BElementView *view, gboolean b);
void b_element_view_set_panning (BElementView *view, gboolean b);
gboolean b_element_view_get_zooming (BElementView *view);
gboolean b_element_view_get_panning (BElementView *view);

void _string_draw (cairo_t * context, PangoFontDescription *font, const BPoint position, BAnchor anchor, BRotation rot, const char *string);

void
_string_draw_no_rotate (cairo_t * context, PangoFontDescription * font,
     const BPoint position, BAnchor anchor, const char *string);

void _view_conv      (GtkWidget *view, const BPoint *t, BPoint *p);
void _view_conv_bulk (GtkWidget *view, const BPoint *t, BPoint *p, gsize N);

void _view_invconv (GtkWidget *view, const BPoint *t, BPoint *p);

BPoint _view_event_point (GtkWidget *widget, GdkEvent *event);

G_END_DECLS

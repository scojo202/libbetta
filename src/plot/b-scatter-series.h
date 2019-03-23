/*
 * b-scatter-series.h
 *
 * Copyright (C) 2000 EMC Capital Management, Inc.
 * Copyright (C) 2018 Scott O. Johnson (scojo202@gmail.com)
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

#ifndef _INC_SCATTER_SERIES_H
#define _INC_SCATTER_SERIES_H

#include <math.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include "b-element-view.h"
#include "data/b-data-class.h"

G_BEGIN_DECLS

/**
 * BMarker:
 * @B_MARKER_NONE: no marker
 * @B_MARKER_CIRCLE: a filled circle
 * @B_MARKER_SQUARE: a filled square
 * @B_MARKER_DIAMOND: a filled diamond
 * @B_MARKER_X: a filled X
 * @B_MARKER_PLUS: a plus symbol
 * @B_MARKER_OPEN_CIRCLE: an unfilled circle
 * @B_MARKER_OPEN_SQUARE: an unfilled square
 * @B_MARKER_OPEN_DIAMOND: an unfilled diamond
 *
 * Enum values used to specify whether and what to use for the marker in a
 * scatter plot.
 */
typedef enum {
  B_MARKER_NONE,
  B_MARKER_CIRCLE,
  B_MARKER_SQUARE,
  B_MARKER_DIAMOND,
  B_MARKER_X,
  B_MARKER_PLUS,
  B_MARKER_OPEN_CIRCLE,
  B_MARKER_OPEN_SQUARE,
  B_MARKER_OPEN_DIAMOND
} BMarker;

/**
 * BDashing:
 * @B_DASHING_SOLID: solid line
 * @B_DASHING_DOTTED: a dotted line
 * @B_DASHING_DASHED: a dashed line
 * @B_DASHING_DOT_DASH: a dot-dashed line
 * @B_DASHING_CUSTOM: custom dashing (not yet implemented)
 *
 * Enum values used to specify line dashing in a
 * scatter plot.
 */
typedef enum {
  B_DASHING_SOLID,
  B_DASHING_DOTTED,
  B_DASHING_DASHED,
  B_DASHING_DOT_DASH
} BDashing;

G_DECLARE_FINAL_TYPE(BScatterSeries,b_scatter_series,B,SCATTER_SERIES,GInitiallyUnowned)

#define B_TYPE_SCATTER_SERIES (b_scatter_series_get_type())

BData *b_scatter_series_set_x_array(BScatterSeries *ss, const double *arr, guint n);
BData *b_scatter_series_set_y_array(BScatterSeries *ss, const double *arr, guint n);

void b_scatter_series_set_line_color_from_string (BScatterSeries *ss, gchar * colorstring);
void b_scatter_series_set_marker_color_from_string (BScatterSeries *ss, gchar * colorstring);

gboolean b_scatter_series_get_show(BScatterSeries *ss);
cairo_surface_t *b_scatter_series_create_legend_image(BScatterSeries *ss);

void b_dashing_set(BDashing d, double line_width, cairo_t *cr);

static inline void
_draw_marker_circle (cairo_t * cr, BPoint pos, double size, gboolean fill)
{
  cairo_arc (cr, pos.x, pos.y, size / 2, 0, 2 * G_PI);
  fill ? cairo_fill (cr) : cairo_stroke(cr);
}

static inline void
_draw_marker_square (cairo_t * cr, BPoint pos, double size, gboolean fill)
{
  cairo_move_to (cr, pos.x - size / 2, pos.y - size / 2);
  cairo_line_to (cr, pos.x - size / 2, pos.y + size / 2);
  cairo_line_to (cr, pos.x + size / 2, pos.y + size / 2);
  cairo_line_to (cr, pos.x + size / 2, pos.y - size / 2);
  cairo_line_to (cr, pos.x - size / 2, pos.y - size / 2);
  fill ? cairo_fill (cr) : cairo_stroke(cr);
}

static inline void
_draw_marker_diamond (cairo_t * cr, BPoint pos, double size, gboolean fill)
{
  cairo_move_to (cr, pos.x - M_SQRT1_2 * size, pos.y);
  cairo_line_to (cr, pos.x, pos.y + M_SQRT1_2 * size);
  cairo_line_to (cr, pos.x + M_SQRT1_2 * size, pos.y);
  cairo_line_to (cr, pos.x, pos.y - M_SQRT1_2 * size);
  cairo_line_to (cr, pos.x - M_SQRT1_2 * size, pos.y);
  fill ? cairo_fill (cr) : cairo_stroke(cr);
}

static inline void
_draw_marker_x (cairo_t * cr, BPoint pos, double size)
{
  cairo_move_to (cr, pos.x - size / 2, pos.y - size / 2);
  cairo_line_to (cr, pos.x + size / 2, pos.y + size / 2);
  cairo_stroke (cr);
  cairo_move_to (cr, pos.x + size / 2, pos.y - size / 2);
  cairo_line_to (cr, pos.x - size / 2, pos.y + size / 2);
  cairo_stroke (cr);
}

static inline void
_draw_marker_plus (cairo_t * cr, BPoint pos, double size)
{
  cairo_move_to (cr, pos.x - size / 2, pos.y);
  cairo_line_to (cr, pos.x + size / 2, pos.y);
  cairo_stroke (cr);
  cairo_move_to (cr, pos.x, pos.y - size / 2);
  cairo_line_to (cr, pos.x, pos.y + size / 2);
  cairo_stroke (cr);
}

G_END_DECLS

#endif

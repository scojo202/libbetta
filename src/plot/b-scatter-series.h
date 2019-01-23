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

#include <gtk/gtk.h>
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

G_DECLARE_FINAL_TYPE(BScatterSeries,b_scatter_series,B,SCATTER_SERIES,GObject)

#define B_TYPE_SCATTER_SERIES (b_scatter_series_get_type())

BData *b_scatter_series_set_x_array(BScatterSeries *ss, const double *arr, guint n);
BData *b_scatter_series_set_y_array(BScatterSeries *ss, const double *arr, guint n);

void b_scatter_series_set_line_color_from_string (BScatterSeries *ss, gchar * colorstring);
void b_scatter_series_set_marker_color_from_string (BScatterSeries *ss, gchar * colorstring);

G_END_DECLS

#endif

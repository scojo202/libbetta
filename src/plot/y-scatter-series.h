/*
 * y-scatter-series.h
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

#ifndef _INC_YSCATTER_SERIES_H
#define _INC_YSCATTER_SERIES_H

#include <gtk/gtk.h>
#include "data/y-data-class.h"
#include "data/y-struct.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(YScatterSeries,y_scatter_series,Y,SCATTER_SERIES,YStruct)

#define Y_TYPE_SCATTER_SERIES (y_scatter_series_get_type())

void y_scatter_series_set_label (YScatterSeries *, GtkLabel *);
void y_scatter_series_set_line_color_from_string (YScatterSeries *view, gchar * colorstring);
void y_scatter_series_set_marker_color_from_string (YScatterSeries *view, gchar * colorstring);

G_END_DECLS

#endif

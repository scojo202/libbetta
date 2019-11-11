/*
 * b-scatter-view.h
 *
 * Copyright (C) 2000 EMC Capital Management, Inc.
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

#pragma once

#include "plot/b-element-view-cartesian.h"
#include "plot/b-scatter-series.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(BScatterLineView,b_scatter_line_view,B,SCATTER_LINE_VIEW,BElementViewCartesian)

#define B_TYPE_SCATTER_LINE_VIEW (b_scatter_line_view_get_type())

void b_scatter_line_view_add_series(BScatterLineView *v, BScatterSeries *s);
GList *b_scatter_line_view_get_all_series(BScatterLineView *v);
void b_scatter_line_view_remove_series(BScatterLineView *v, const gchar *label);

G_END_DECLS

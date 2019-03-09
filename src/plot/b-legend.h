/*
 * b-legend.h
 *
 * Copyright (C) 2019 Scott O. Johnson (scojo202@gmail.com)
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
#include "plot/b-scatter-line-view.h"

#pragma once

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(BLegend,b_legend,B,LEGEND,GtkToolbar)

#define B_TYPE_LEGEND (b_legend_get_type())

BLegend *b_legend_new(BScatterLineView *view);
void b_legend_set_view(BLegend *l, BScatterLineView *view);

G_END_DECLS

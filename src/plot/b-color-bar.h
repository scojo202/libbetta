/*
 * y-color-bar.h
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

#ifndef _INC_COLOR_BAR_H
#define _INC_COLOR_BAR_H

#include <pango/pango.h>

#include "plot/y-element-view-cartesian.h"
#include "plot/y-axis-markers.h"
#include "plot/y-color-map.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(YColorBar,y_color_bar,Y,COLOR_BAR,YElementViewCartesian)

#define Y_TYPE_COLOR_BAR  (y_color_bar_get_type ())

YColorBar * y_color_bar_new(GtkOrientation o, YColorMap *m);

G_END_DECLS

#endif

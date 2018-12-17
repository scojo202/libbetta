/*
 * y-axis-view.h
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

#ifndef _INC_AXIS_VIEW_H
#define _INC_AXIS_VIEW_H

#include <pango/pango.h>

#include "plot/y-element-view-cartesian.h"
#include "plot/y-axis-markers.h"

G_BEGIN_DECLS

typedef enum {
  Y_COMPASS_INVALID = 0,
  Y_COMPASS_NORTH = 1 << 0,
  Y_COMPASS_SOUTH = 1 << 1,
  Y_COMPASS_EAST = 1 << 2,
  Y_COMPASS_WEST = 1 << 3
} YCompass;

G_DECLARE_FINAL_TYPE(YAxisView,y_axis_view,Y,AXIS_VIEW,YElementViewCartesian)

#define Y_TYPE_AXIS_VIEW  (y_axis_view_get_type ())

YAxisView * y_axis_view_new(YCompass t);

G_END_DECLS

#endif

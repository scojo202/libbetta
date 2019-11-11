/*
 * b-axis-view.h
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

#pragma once

#include <pango/pango.h>

#include "plot/b-element-view-cartesian.h"
#include "plot/b-axis-markers.h"

G_BEGIN_DECLS

/**
 * BCompass:
 * @B_COMPASS_INVALID: not used
 * @B_COMPASS_NORTH: axis is above the plot
 * @B_COMPASS_SOUTH: axis is below the plot
 * @B_COMPASS_EAST: axis is to the right of the plot
 * @B_COMPASS_WEST: axis is to the left of the plot
 *
 * Enum values used to specify the position of an axis with respect to a main
 * plot.
 */
typedef enum
{
  B_COMPASS_INVALID = 0,
  B_COMPASS_NORTH = 1,
  B_COMPASS_SOUTH = 2,
  B_COMPASS_EAST = 3,
  B_COMPASS_WEST = 4
} BCompass;

G_DECLARE_FINAL_TYPE(BAxisView,b_axis_view,B,AXIS_VIEW,BElementViewCartesian)

#define B_TYPE_AXIS_VIEW  (b_axis_view_get_type ())

BAxisView * b_axis_view_new(BCompass t);

G_END_DECLS

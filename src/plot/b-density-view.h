/*
 * b-density-view.h
 *
 * Copyright (C) 2016, 2018 Scott O. Johnson (scojo202@gmail.com)
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
#include "data/b-data-class.h"
#include "plot/b-element-view-cartesian.h"

#pragma once

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(BDensityView,b_density_view,B,DENSITY_VIEW,BElementViewCartesian)

#define B_TYPE_DENSITY_VIEW (b_density_view_get_type())

G_END_DECLS

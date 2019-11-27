/*
 * b-struct.h :
 *
 * Copyright (C) 2018 Scott O. Johnson (scojo202@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#pragma once

#include "b-data-class.h"

G_BEGIN_DECLS

G_DECLARE_DERIVABLE_TYPE(BStruct, b_struct, B, STRUCT, BData)

#define B_TYPE_STRUCT (b_struct_get_type())

struct _BStructClass {
  BDataClass base;

  /* signals */
  void (*subdata_changed) (BStruct * dat);
};

BData *b_struct_get_data(BStruct * s, const gchar * name);
void b_struct_set_data(BStruct * s, const gchar * name, BData * d);
void b_struct_foreach(BStruct * s, GHFunc f, gpointer user_data);

G_END_DECLS

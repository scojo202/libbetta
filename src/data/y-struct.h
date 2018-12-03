/*
 * y-struct.h :
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

#ifndef Y_STRUCT_H
#define Y_STRUCT_H

#include <glib-object.h>
#include "y-data-class.h"

G_BEGIN_DECLS

G_DECLARE_DERIVABLE_TYPE(YStruct, y_struct, Y, STRUCT, YData)

#define Y_TYPE_STRUCT (y_struct_get_type())

struct _YStructClass {
	YDataClass base;

	/* signals */
	void (*subdata_changed) (YStruct * dat);
};

YData *y_struct_get_data(YStruct * s, const gchar * name);
void y_struct_set_data(YStruct * s, const gchar * name, YData * d);
void y_struct_foreach(YStruct * s, GHFunc f, gpointer user_data);

G_END_DECLS

#endif				/* Y_DATA_H */

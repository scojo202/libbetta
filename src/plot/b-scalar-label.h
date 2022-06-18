/*
 * b-scalar-label.h
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

#include "data/b-data-class.h"
#include <gtk/gtk.h>

#pragma once

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (BScalarLabel, b_scalar_label, B, SCALAR_LABEL, GtkWidget)

#define B_TYPE_SCALAR_LABEL                  (b_scalar_label_get_type ())

BScalarLabel * b_scalar_label_new (const gchar * format, const gchar * prefix);
void b_scalar_label_set_source (BScalarLabel * f, BScalar * source);

G_END_DECLS

/*
 * y-rate-label.h
 *
 * Copyright (C) 2017 Scott O. Johnson (scojo202@gmail.com)
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

#include "data/y-data-class.h"
#include <gtk/gtk.h>

#ifndef __Y_RATE_LABEL_H__
#define __Y_RATE_LABEL_H__

G_DECLARE_FINAL_TYPE (YRateLabel, y_rate_label, Y, RATE_LABEL, GtkLabel)

#define Y_TYPE_RATE_LABEL                  (y_rate_label_get_type ())

YRateLabel * y_rate_label_new(const gchar * text, const gchar *suffix);
void y_rate_label_update(YRateLabel *f);
void y_rate_label_set_source(YRateLabel *f, YData *source);

#endif

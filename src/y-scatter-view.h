/*
 * y-scatter-view.h
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

#ifndef _INC_YSCATTER_VIEW_H
#define _INC_YSCATTER_VIEW_H

#include <y-element-view-cartesian.h>

G_BEGIN_DECLS

typedef struct _YScatterView YScatterView;
typedef struct _YScatterViewClass YScatterViewClass;
struct _YScatterViewPrivate;

struct _YScatterView {
  YElementViewCartesian parent;
  struct _YScatterViewPrivate *priv;
};

struct _YScatterViewClass {
  YElementViewCartesianClass parent_class;
};

#define Y_TYPE_SCATTER_VIEW (y_scatter_view_get_type())
#define Y_SCATTER_VIEW(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),Y_TYPE_SCATTER_VIEW,YScatterView))
#define Y_SCATTER_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),Y_TYPE_SCATTER_VIEW,YScatterViewClass))
#define Y_IS_SCATTER_VIEW(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), Y_TYPE_SCATTER_VIEW))
#define Y_IS_SCATTER_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), Y_TYPE_SCATTER_VIEW))

GType y_scatter_view_get_type (void);

void y_scatter_view_set_label (YScatterView *, GtkLabel *);
void y_scatter_view_set_line_color_from_string (YScatterView *view, gchar * colorstring);
void y_scatter_view_set_marker_color_from_string (YScatterView *view, gchar * colorstring);

gboolean
rectangle_contains_point (cairo_rectangle_t rect, Point * point);

G_END_DECLS

#endif


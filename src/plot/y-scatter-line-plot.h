/*
 * y-plot-widget.h
 *
 * Copyright (C) 2016 Scott O. Johnson (scojo202@gmail.com)
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
#include "data/y-data-class.h"
#include "plot/y-axis-view.h"
#include "plot/y-scatter-line-view.h"

#ifndef _INC_YSCATTER_LINE_PLOT_H
#define _INC_YSCATTER_LINE_PLOT_H

G_BEGIN_DECLS

#define Y_TYPE_SCATTER_LINE_PLOT (y_scatter_line_plot_get_type())
#define Y_SCATTER_LINE_PLOT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),Y_TYPE_SCATTER_LINE_PLOT,YScatterLinePlot))
#define Y_SCATTER_LINE_PLOT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),Y_TYPE_SCATTER_LINE_PLOT,YScatterLinePlotClass))
#define Y_IS_SCATTER_LINE_PLOT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), Y_TYPE_SCATTER_LINE_PLOT))
#define Y_IS_SCATTER_LINE_PLOT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), Y_TYPE_SCATTER_LINE_PLOT))

GType y_scatter_line_plot_get_type (void);

typedef struct _YScatterLinePlot YScatterLinePlot;
typedef struct _YScatterLinePlotClass YScatterLinePlotClass;
struct _YScatterLinePlotPrivate;

struct _YScatterLinePlot {
  GtkEventBox parent;
  struct _YScatterLinePlotPrivate *priv;

  YAxisView * north_axis;
  YAxisView * south_axis;
  YAxisView * west_axis;
  YAxisView * east_axis;

  YScatterLineView *main_view;
};

struct _YScatterLinePlotClass {
  GtkEventBoxClass parent_class;

};

void y_scatter_line_plot_freeze(YScatterLinePlot *plot);
void y_scatter_line_plot_thaw(YScatterLinePlot *plot);

void y_scatter_line_plot_set_max_frame_rate(YScatterLinePlot *plot, float rate);

G_END_DECLS

#endif

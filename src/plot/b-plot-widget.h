/*
 * y-plot-widget.h
 *
 * Copyright (C) 2018 Scott O. Johnson (scojo202@gmail.com)
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

#ifndef _INC_YPLOT_WIDGET_H
#define _INC_YPLOT_WIDGET_H

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(YPlotWidget,y_plot_widget,Y,PLOT_WIDGET,GtkGrid)

#define Y_TYPE_PLOT_WIDGET (y_plot_widget_get_type())

void y_plot_widget_add_view(YPlotWidget *obj, YElementViewCartesian *view);
YPlotWidget * y_plot_widget_new_scatter(YScatterSeries *series);
YPlotWidget * y_plot_widget_new_density();

YElementViewCartesian * y_plot_widget_get_main_view(YPlotWidget *plot);
YAxisView *y_plot_widget_get_axis_view(YPlotWidget *plot, YCompass c);

void y_plot_widget_set_x_label(YPlotWidget *plot, const gchar *label);
void y_plot_widget_set_y_label(YPlotWidget *plot, const gchar *label);

void y_plot_freeze_all (GtkContainer * c);
void y_plot_thaw_all (GtkContainer * c);

GtkToolbar *y_plot_toolbar_new(GtkContainer *c);

G_END_DECLS

#endif

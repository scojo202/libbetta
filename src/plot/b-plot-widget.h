/*
 * b-plot-widget.h
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

#include "data/b-data-class.h"
#include "plot/b-axis-view.h"
#include "plot/b-scatter-line-view.h"
#include "plot/b-legend.h"

#pragma once

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(BPlotWidget,b_plot_widget,B,PLOT_WIDGET,GtkWidget)

#define B_TYPE_PLOT_WIDGET (b_plot_widget_get_type())

void b_plot_set_background(GtkWidget *widget, const gchar *color_string);

void b_plot_widget_add_view(BPlotWidget *obj, BElementViewCartesian *view);
BPlotWidget * b_plot_widget_new_scatter(BScatterSeries *series);
BPlotWidget * b_plot_widget_new_density();

BElementViewCartesian * b_plot_widget_get_main_view(BPlotWidget *plot);
BAxisView *b_plot_widget_get_axis_view(BPlotWidget *plot, BCompass c);

void b_plot_widget_set_x_label(BPlotWidget *plot, const gchar *label);
void b_plot_widget_set_y_label(BPlotWidget *plot, const gchar *label);

//gboolean b_plot_save(GtkContainer *c, gchar *path, GError *error);

void b_plot_widget_freeze_all (BPlotWidget * c);
void b_plot_widget_thaw_all (BPlotWidget * c);

GtkBox *b_plot_widget_toolbar_new(BPlotWidget *g);
void b_plot_widget_attach_legend(BPlotWidget *plot, BLegend *leg);

G_END_DECLS

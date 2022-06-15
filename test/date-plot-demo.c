/*
 * date-plot-demo.c
 *
 * Copyright (C) 1999 EMC Capital Management, Inc.
 * Copyright (C) 2001 The Free Software Foundation
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

#include <math.h>
#include <gtk/gtk.h>
#include "b-data.h"
#include "b-plot.h"

#define DATA_COUNT 200

BPlotWidget * scatter_plot;
BScatterLineView *scatline;

BData *d1, *d2;

GTimer *timer;

static void
build_gui (GtkApplication *app)
{
  GtkApplicationWindow *window = g_object_new(GTK_TYPE_APPLICATION_WINDOW,NULL);
  gtk_window_set_default_size(GTK_WINDOW(window),640,400);
  gtk_window_set_child(GTK_WINDOW(window),GTK_WIDGET(scatter_plot));

  gtk_widget_show (GTK_WIDGET(window));
  gtk_application_add_window(app,GTK_WINDOW(window));

  g_message("built GUI: %f s",g_timer_elapsed(timer,NULL));

  //b_plot_save(GTK_CONTAINER(scatter_plot),"xy-plot.png",NULL);

  g_message("saved to file: %f s",g_timer_elapsed(timer,NULL));
}

static void
build_data (void)
{
  gint i;
  double t;
  double *x, *y;

  x=g_malloc(sizeof(double)*DATA_COUNT);
  y=g_malloc(sizeof(double)*DATA_COUNT);
  for (i=0; i<DATA_COUNT; ++i) {
    t = 736935.1 + 0.5*((double)i);
    x[i] = t;
    y[i] = 1e4*cos (t/3);
  }
  d1 = b_val_vector_new (x, DATA_COUNT, NULL);
  d2 = b_val_vector_new (y, DATA_COUNT, NULL);

  timer = g_timer_new();
}

static void
build_elements (void)
{
  BScatterSeries *series1 = g_object_new(B_TYPE_SCATTER_SERIES,"x-data",d1,"y-data",d2,NULL);

	b_scatter_series_set_line_color_from_string(series1,"#ff0000");

	g_message("created series: %f s",g_timer_elapsed(timer,NULL));

  scatter_plot = b_plot_widget_new_scatter(series1);

  BAxisView * sa = b_plot_widget_get_axis_view(scatter_plot,B_COMPASS_SOUTH);

  b_element_view_cartesian_set_axis_marker_type (B_ELEMENT_VIEW_CARTESIAN
  					 (sa), B_AXIS_TYPE_META, B_AXIS_DATE);

  //BAxisView * na = b_plot_widget_get_axis_view(scatter_plot,B_COMPASS_NORTH);

  //b_element_view_cartesian_set_axis_marker_type (B_ELEMENT_VIEW_CARTESIAN
//              					 (na), B_AXIS_TYPE_META, B_AXIS_DATE);

	g_message("created plot: %f s",g_timer_elapsed(timer,NULL));

  scatline = B_SCATTER_LINE_VIEW(b_plot_widget_get_main_view(scatter_plot));

  g_object_set(b_plot_widget_get_axis_view (scatter_plot, B_COMPASS_SOUTH),"axis_label","this is the x axis",NULL);
  g_object_set(b_plot_widget_get_axis_view (scatter_plot, B_COMPASS_WEST),"axis_label","this is the y axis",NULL);

	g_message("built elements: %f s",g_timer_elapsed(timer,NULL));
}

static void
demo_activate (GApplication *application)
{
  build_data ();

  build_elements ();

  build_gui (GTK_APPLICATION(application));
}

#include "demo-boilerplate.c"

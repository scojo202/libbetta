/*
 * demo.c
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

#define DATA_COUNT 2000

BPlotWidget * scatter_plot;
BDensityView *dens;

BData *d1;

GTimer *timer;

gint counter = 0;

static void
build_gui (GtkApplication *app)
{
  GtkApplicationWindow *window = g_object_new(GTK_TYPE_APPLICATION_WINDOW,NULL);
  gtk_window_set_default_size(GTK_WINDOW(window),640,400);
  gtk_window_set_child(GTK_WINDOW(window),GTK_WIDGET(scatter_plot));

  gtk_widget_show (GTK_WIDGET(window));
  gtk_application_add_window(app,GTK_WINDOW(window));

  g_message("built GUI: %f s",g_timer_elapsed(timer,NULL));
}

static void
build_data (void)
{
  gint i,j;
  double t;
  double *x;

  x=g_malloc(sizeof(double)*DATA_COUNT*DATA_COUNT/2);
  for (i=0; i<DATA_COUNT; ++i) {
    for (j=0; j<DATA_COUNT/2; ++j) {
      t = (i-DATA_COUNT/2)*(i-DATA_COUNT/2)+(j-DATA_COUNT/4)*(j-DATA_COUNT/4);
      x[i+j*DATA_COUNT] = 3.0*exp(-t/200.0/200.0);
    }
  }
  d1 = b_val_matrix_new (x, DATA_COUNT/2, DATA_COUNT, NULL);

  timer = g_timer_new();
}

static void
build_elements (void)
{
  scatter_plot = b_plot_widget_new_density();

  BColorMap *map = b_color_map_new();
  b_color_map_set_seismic(map);
  BColorBar *bar = b_color_bar_new(GTK_ORIENTATION_VERTICAL, map);

  GtkLayoutManager *man = gtk_widget_get_layout_manager(GTK_WIDGET(scatter_plot));

  gtk_widget_insert_before(GTK_WIDGET(bar),GTK_WIDGET(scatter_plot),NULL);
  GtkLayoutChild *main_child = gtk_layout_manager_get_layout_child(man,GTK_WIDGET(bar));
  g_object_set(main_child,"column",3,"row",1,NULL);

  b_element_view_cartesian_connect_view_intervals (b_plot_widget_get_main_view(scatter_plot), B_AXIS_TYPE_Z,
  					   B_ELEMENT_VIEW_CARTESIAN(bar), B_AXIS_TYPE_META);

  b_element_view_cartesian_connect_axis_markers (B_ELEMENT_VIEW_CARTESIAN(bar), B_AXIS_TYPE_META,
						   					 b_plot_widget_get_main_view(scatter_plot), B_AXIS_TYPE_Z);

  b_element_view_cartesian_set_axis_marker_type (B_ELEMENT_VIEW_CARTESIAN(bar), B_AXIS_TYPE_META,
                         					 B_AXIS_SCALAR);

  g_message("created plot: %f s",g_timer_elapsed(timer,NULL));

  dens = B_DENSITY_VIEW(b_plot_widget_get_main_view(scatter_plot));
  g_object_set(dens,"data",d1,"preserve-aspect",FALSE,"dx",-1.5,"dy",-1.0,NULL);
  g_object_set(dens,"color-map", map, NULL);

  g_object_set(b_plot_widget_get_axis_view (scatter_plot, B_COMPASS_SOUTH),"axis_label","this is the x axis",NULL);
  g_object_set(b_plot_widget_get_axis_view (scatter_plot, B_COMPASS_WEST),"axis_label","this is the y axis",NULL);

  g_object_set(bar,"bar_label","colorbar label",NULL);

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


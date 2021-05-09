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
BScatterLineView *scatline;

BRateLabel *label;

GdkFrameClock *frame_clock;

BData *d1, *d2, *d3;

static double phi = 0;

gint counter = 0;

static gboolean
update_plot (GdkFrameClock *clock, gpointer foo)
{
  gint i;

  double t,x,y;

  b_plot_freeze_all(GTK_CONTAINER (scatter_plot));

  double *v1 = b_val_vector_get_array(B_VAL_VECTOR(d1));
  double *v2 = b_val_vector_get_array(B_VAL_VECTOR(d2));
  for (i=0; i<DATA_COUNT; ++i) {
    t = 2*G_PI*i/(double)DATA_COUNT;
    x = phi+2*sin (4*t+phi);
    y = cos (3*t);
    v1[i]=x;
    v2[i]=y;
  }

  b_data_emit_changed(d1);
  b_data_emit_changed(d2);

  gchar b[100];
  sprintf(b,"frame %d",counter);
  g_object_set(b_plot_widget_get_axis_view (scatter_plot, B_COMPASS_NORTH),"axis_label",b,NULL);

  b_plot_thaw_all(GTK_CONTAINER(scatter_plot));

  counter++;

  phi+=0.05;
  //if(phi>3) exit(0);

  return TRUE;
}

static
gboolean tick_callback (GtkWidget *widget,
                    GdkFrameClock *frame_clock,
                    gpointer user_data)
{
  b_rate_label_update(label);

  return G_SOURCE_CONTINUE;
}

static void
build_gui (GtkApplication *app)
{
  GtkApplicationWindow *window = g_object_new(GTK_TYPE_WINDOW,NULL);
  gtk_window_set_default_size(GTK_WINDOW(window),300,500);

  label = b_rate_label_new("Update rate","fps");
  //gtk_grid_attach(GTK_GRID(scatter_plot),GTK_WIDGET(label),0,4,3,1);

  gtk_window_set_child(GTK_WINDOW(window),GTK_WIDGET(scatter_plot));

  gtk_widget_show_all (GTK_WIDGET(window));
  gtk_application_add_window(app,GTK_WINDOW(window));

	gtk_widget_add_tick_callback (GTK_WIDGET(scatter_plot),
                              tick_callback,
                              NULL,
                              NULL);

  GdkFrameClock *frame_clock = gdk_window_get_frame_clock(gtk_widget_get_window(GTK_WIDGET(scatter_plot)));
  g_signal_connect(frame_clock,"update",G_CALLBACK(update_plot),NULL);

  gdk_frame_clock_begin_updating(frame_clock);
}

static void
build_data (void)
{
  gint i;
  double t;
  double *x, *y, *z;

  x=g_malloc(sizeof(double)*DATA_COUNT);
  y=g_malloc(sizeof(double)*DATA_COUNT);
  z=g_malloc(sizeof(double)*DATA_COUNT);
  for (i=0; i<DATA_COUNT; ++i) {
    t = 2*G_PI*i/(double)DATA_COUNT;
    x[i] = 2*sin (4*t);
    y[i] = cos (3*t);
    z[i] = cos (5*t);
  }
  d1 = b_val_vector_new (x, DATA_COUNT, NULL);
  d2 = b_val_vector_new (y, DATA_COUNT, NULL);
  d3 = b_val_vector_new (z, DATA_COUNT, NULL);
}

static void
build_elements (void)
{
  BScatterSeries *series1 = g_object_new(B_TYPE_SCATTER_SERIES,"x-data",d1,"y-data",d2,NULL);
  BScatterSeries *series2 = g_object_new(B_TYPE_SCATTER_SERIES,"x-data",d1,"y-data",d3,NULL);

  b_scatter_series_set_line_color_from_string (series2, "#ff0000");

  scatter_plot = b_plot_widget_new_scatter(series1);

  scatline = B_SCATTER_LINE_VIEW(b_plot_widget_get_main_view (scatter_plot));

  b_scatter_line_view_add_series(scatline,series2);

  g_object_set(b_plot_widget_get_axis_view (scatter_plot, B_COMPASS_SOUTH),"axis_label","this is the x axis",NULL);
  g_object_set(b_plot_widget_get_axis_view (scatter_plot, B_COMPASS_WEST),"axis_label","this is the y axis",NULL);
  g_object_set(b_plot_widget_get_axis_view (scatter_plot, B_COMPASS_EAST),"axis_label","this is the y axis",NULL);
}

static void
demo_activate (GApplication *application)
{
  build_data ();

  build_elements ();

  build_gui (GTK_APPLICATION(application));
}

#include "demo-boilerplate.c"


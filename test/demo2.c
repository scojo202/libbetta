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
#include "data/y-data-simple.h"
#include "plot/y-plot-widget.h"
#include "plot/y-axis-view.h"
#include "plot/y-scatter-series.h"
#include "plot/y-scatter-line-view.h"
#include "plot/y-scatter-line-plot.h"

#define DATA_COUNT 5000

GtkWidget *window;
YScatterLinePlot * scatter_plot;
YScatterLineView *scatline;

GdkFrameClock *frame_clock;

YData *d1, *d2, *d3;

GTimer *timer;

static double phi = 0;

gint counter = 0;

static void
init (gint argc, gchar *argv[])
{
  gtk_init(&argc, &argv);
}

static gboolean
update_plot (GdkFrameClock *clock, gpointer foo)
{
  gint i;

  double t,x,y;

  y_plot_widget_freeze(scatter_plot);

  double start_update = g_timer_elapsed(timer, NULL);

  double *v1 = y_val_vector_get_array(Y_VAL_VECTOR(d1));
  double *v2 = y_val_vector_get_array(Y_VAL_VECTOR(d2));
  for (i=0; i<DATA_COUNT; ++i) {
    t = 2*G_PI*i/(double)DATA_COUNT;
    x = 2*sin (4*t+phi);
    y = cos (3*t);
    v1[i]=x;
    v2[i]=y;
  }

  double interval2 = g_timer_elapsed(timer, NULL);

  y_data_emit_changed(d1);
  y_data_emit_changed(d2);

  gchar b[100];
  sprintf(b,"frame %d",counter);
  g_object_set(scatter_plot->north_axis,"axis_label",b,NULL);

  y_plot_widget_thaw(scatter_plot);

  gdk_window_process_all_updates();

  counter++;

  phi+=0.05;
  //if(phi>3) exit(0);

  double interval = g_timer_elapsed(timer, NULL);
  g_timer_start(timer);

  printf("frame rate: %f, %f\%% spent on update, %f\%% spent on data\n",1/interval,(interval-start_update)/interval*100,(interval2-start_update)/interval*100);

  return TRUE;
}

static void
quit (GtkWidget *w, GdkEventAny *ev, gpointer closure)
{
  //g_timeout_remove (timeout);
  gtk_widget_destroy (window);

  gtk_main_quit ();
}

static void
build_gui (void)
{
  window = g_object_new(GTK_TYPE_WINDOW,NULL);
  gtk_window_set_default_size(GTK_WINDOW(window),300,500);
  gtk_container_add(GTK_CONTAINER(window),GTK_WIDGET(scatter_plot));

  g_signal_connect (G_OBJECT (window),
		      "delete_event",
		      G_CALLBACK (quit),
		      NULL);

  gtk_widget_show_all (window);

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
  d1 = y_val_vector_new (x, DATA_COUNT, NULL);
  d2 = y_val_vector_new (y, DATA_COUNT, NULL);
  d3 = y_val_vector_new (z, DATA_COUNT, NULL);

  timer = g_timer_new();
}

static void
build_elements (void)
{
  YScatterSeries *series1 = g_object_new(Y_TYPE_SCATTER_SERIES,NULL);
  y_struct_set_data(Y_STRUCT(series1),"x",d1);
  y_struct_set_data(Y_STRUCT(series1),"y",d2);

  YScatterSeries *series2 = g_object_new(Y_TYPE_SCATTER_SERIES,NULL);
  y_struct_set_data(Y_STRUCT(series2),"x",d1);
  y_struct_set_data(Y_STRUCT(series2),"y",d3);

  GdkRGBA c;
  gboolean success = gdk_rgba_parse (&c,"#ff0000");
  g_object_set(series2,"line-color",&c, NULL);

  scatter_plot = g_object_new (Y_TYPE_SCATTER_LINE_PLOT, NULL);

  scatline = scatter_plot->main_view;

  y_scatter_line_view_add_series(scatline,series1);
  y_scatter_line_view_add_series(scatline,series2);

  g_object_set(scatter_plot->south_axis,"axis_label","this is the x axis",NULL);
  g_object_set(scatter_plot->west_axis,"axis_label","this is the y axis",NULL);
  g_object_set(scatter_plot->east_axis,"axis_label","this is the y axis",NULL);
}

int
main (int argc, char *argv[])
{

  init (argc, argv);

  g_message ("building data");
  build_data ();

  g_message ("building elements");
  build_elements ();

  g_message ("building gui");
  build_gui ();

  gtk_main ();

  return 0;
}

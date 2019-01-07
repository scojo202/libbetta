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

#define DATA_COUNT 20000

GtkWidget *window;
YPlotWidget * scatter_plot;
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

  y_plot_freeze_all(GTK_CONTAINER (scatter_plot));

  double *v1 = y_val_vector_get_array(Y_VAL_VECTOR(d1));
  double *v2 = y_val_vector_get_array(Y_VAL_VECTOR(d2));
  for (i=0; i<DATA_COUNT; ++i) {
    t = 2*G_PI*i/(double)DATA_COUNT;
    x = phi+2*sin (4*t+phi);
    y = cos (3*t);
    v1[i]=x;
    v2[i]=y;
  }

  y_data_emit_changed(d1);
  y_data_emit_changed(d2);

  gchar b[100];
  sprintf(b,"frame %d",counter);
  g_object_set(scatter_plot->north_axis,"axis_label",b,NULL);

  y_plot_thaw_all(GTK_CONTAINER(scatter_plot));

  counter++;

  phi+=0.05;
  //if(phi>3) exit(0);

  return TRUE;
}

static void
quit (GtkWidget *w, GdkEventAny *ev, gpointer closure)
{
  //g_timeout_remove (timeout);
  gtk_widget_destroy (window);

  gtk_main_quit ();
}

static
gboolean tick_callback (GtkWidget *widget,
                    GdkFrameClock *frame_clock,
                    gpointer user_data)
{
  double interval = g_timer_elapsed(timer, NULL);
  g_timer_start(timer);

  printf("frame rate: %f\n",1/interval);

  return G_SOURCE_CONTINUE;
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
  d1 = y_val_vector_new (x, DATA_COUNT, NULL);
  d2 = y_val_vector_new (y, DATA_COUNT, NULL);
  d3 = y_val_vector_new (z, DATA_COUNT, NULL);

  timer = g_timer_new();
}

static void
build_elements (void)
{
  YScatterSeries *series1 = g_object_new(Y_TYPE_SCATTER_SERIES,"x-data",d1,"y-data",d2,NULL);
  YScatterSeries *series2 = g_object_new(Y_TYPE_SCATTER_SERIES,"x-data",d1,"y-data",d3,NULL);

  y_scatter_series_set_line_color_from_string (series2, "#ff0000");

  scatter_plot = y_plot_widget_new_scatter(series1);

  scatline = Y_SCATTER_LINE_VIEW(scatter_plot->main_view);

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

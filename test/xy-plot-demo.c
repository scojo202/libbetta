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

#define DATA_COUNT 200

GtkWidget *window;
BPlotWidget * scatter_plot;
BScatterLineView *scatline;

BData *d1, *d2, *d3;

GTimer *timer;

gint counter = 0;

static void
init (gint argc, gchar *argv[])
{
  gtk_init(&argc, &argv);
}

static void
quit (GtkWidget *w, GdkEventAny *ev, gpointer closure)
{
  //g_timeout_remove (timeout);
  gtk_widget_destroy (window);
  g_timer_destroy(timer);

  gtk_main_quit ();
}

static void
build_gui (void)
{
  window = g_object_new(GTK_TYPE_WINDOW,NULL);
  gtk_window_set_default_size(GTK_WINDOW(window),640,400);
  gtk_container_add(GTK_CONTAINER(window),GTK_WIDGET(scatter_plot));

  g_signal_connect (G_OBJECT (window),
		      "delete_event",
		      G_CALLBACK (quit),
		      NULL);

  gtk_widget_show_all (window);

	g_message("built GUI: %f s",g_timer_elapsed(timer,NULL));

  b_plot_save(GTK_CONTAINER(scatter_plot),"xy-plot.png",NULL);

  g_message("saved to file: %f s",g_timer_elapsed(timer,NULL));
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
    x[i] = 1e4*2*sin (4*t);
    y[i] = 1e4*cos (3*t);
    z[i] = 1e4*2*cos (5*t);
  }
	/* test handling of NAN's */
	x[31] = NAN;
	x[32] = NAN;
	//y[51] = NAN;
	//y[52] = NAN;
	y[53] = NAN;
  d1 = b_val_vector_new (x, DATA_COUNT, g_free);
  d2 = b_val_vector_new (y, DATA_COUNT, g_free);
  d3 = b_val_vector_new (z, DATA_COUNT, g_free);

  timer = g_timer_new();
}

static void
build_elements (void)
{
  BScatterSeries *series1 = g_object_new(B_TYPE_SCATTER_SERIES,"x-data",d1,"y-data",d2,"label","foo","dashing", B_DASHING_DOTTED, NULL);
  BScatterSeries *series2 = g_object_new(B_TYPE_SCATTER_SERIES,"x-data",d1,"y-data",d3,"marker",B_MARKER_OPEN_DIAMOND,"label","bar",NULL);
	//g_object_set(series2,"y-err",b_val_scalar_new(1000),NULL);
	GRand *r = g_rand_new();
	BVector *v = B_VECTOR(b_val_vector_new_alloc(DATA_COUNT));
	double *vd = b_val_vector_get_array(B_VAL_VECTOR(v));
	for(int i=0;i<DATA_COUNT;i++) {
		vd[i]=g_rand_double_range(r,500.0,1500.0);
	}
  g_rand_free(r);
	g_object_set(series2,"x-err",v,NULL);

	b_scatter_series_set_line_color_from_string(series1,"#ff0000");
	b_scatter_series_set_marker_color_from_string(series2,"#0000ff");

	g_message("created series: %f s",g_timer_elapsed(timer,NULL));

  scatter_plot = b_plot_widget_new_scatter(series1);

	g_message("created plot: %f s",g_timer_elapsed(timer,NULL));

  scatline = B_SCATTER_LINE_VIEW(b_plot_widget_get_main_view(scatter_plot));

  b_scatter_line_view_add_series(scatline,series2);

	g_object_set(scatline, "v-cursor-pos", 0.0, "h-cursor-pos", 1000.0, NULL);

  g_object_set(b_plot_widget_get_axis_view (scatter_plot, B_COMPASS_SOUTH),
               "axis_label", "this is the x axis", NULL);
  g_object_set(b_plot_widget_get_axis_view (scatter_plot, B_COMPASS_WEST),
               "axis_label", "this is the y axis", NULL);

	BLegend *l = b_legend_new(scatline);
	gtk_grid_attach(GTK_GRID(scatter_plot),GTK_WIDGET(l),0,4,3,1);

	g_message("built elements: %f s",g_timer_elapsed(timer,NULL));
}

int
main (int argc, char *argv[])
{

  init (argc, argv);

  build_data ();

  build_elements ();

  build_gui ();

  gtk_main ();

  return 0;
}

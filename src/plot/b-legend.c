/*
 * b-plot-widget.c
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

#include "config.h"
#include <cairo.h>
#include "plot/b-legend.h"

/**
 * SECTION: b-legend
 * @short_description: Widget showing a list of series in a #BScatterLineView.
 *
 *
 *
 */

static GObjectClass *parent_class = NULL;

struct _BLegend
{
  GtkToolbar base;

  BScatterLineView *view;
};

static void
b_legend_class_init (BLegendClass * klass)
{
  //GObjectClass *object_class = (GObjectClass *) klass;

  parent_class = g_type_class_peek_parent (klass);
}

static void
b_legend_init (BLegend * obj)
{
  //GtkGrid *grid = GTK_GRID (obj);

}

G_DEFINE_TYPE (BLegend, b_legend, GTK_TYPE_TOOLBAR);

static void
attach_control (gpointer data, gpointer user_data)
{
  BScatterSeries *s = (BScatterSeries *) data;
  GtkToolbar *g = GTK_TOOLBAR(user_data);

  GtkToolItem *i = gtk_tool_item_new();

  gchar *label;
  g_object_get(s,"label",&label,NULL);
  GtkWidget *l = gtk_label_new(label);
  gtk_container_add(GTK_CONTAINER(i),l);
  gtk_toolbar_insert(g,i,-1);

  i = gtk_tool_item_new();

  cairo_surface_t *surf = b_scatter_series_create_legend_image(s);
  GtkWidget *im = gtk_image_new_from_surface(surf) ;
  gtk_container_add(GTK_CONTAINER(i),im);
  gtk_toolbar_insert(g,i,-1);
}

/**
 * b_legend_set_view:
 * @l: a #BLegend
 * @view: a view
 *
 * Set legend from view.
 **/
void b_legend_set_view(BLegend *l, BScatterLineView *view)
{
  if(l->view != NULL) {
    /* remove old controls */
  }
  l->view = g_object_ref(view);
  GList *s = b_scatter_line_view_get_all_series(view);
  g_list_foreach(s,attach_control,l);
}

/**
 * b_legend_new:
 * @view: a view
 *
 * Create a legend for @view.
 **/
BLegend *b_legend_new(BScatterLineView *view)
{
  BLegend *l = g_object_new(B_TYPE_LEGEND,NULL);
  b_legend_set_view(l,view);
  return l;
}

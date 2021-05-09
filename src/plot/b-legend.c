/*
 * b-legend.c
 *
 * Copyright (C) 2019 Scott O. Johnson (scojo202@gmail.com)
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
  GtkBox base;

  BScatterLineView *view;
};

static void
legend_finalize (GObject * obj)
{
  BLegend *v = B_LEGEND (obj);
  g_clear_object(&v->view);

  if (parent_class->finalize)
    parent_class->finalize (obj);
}

static void
b_legend_class_init (BLegendClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  object_class->finalize = legend_finalize;

  parent_class = g_type_class_peek_parent (klass);
}

static void
b_legend_init (BLegend * obj)
{
}

G_DEFINE_TYPE (BLegend, b_legend, GTK_TYPE_BOX);

static void
attach_control (gpointer data, gpointer user_data)
{
  BScatterSeries *s = (BScatterSeries *) data;
  GtkBox *g = GTK_BOX(user_data);

  gchar *label, *tooltip;
  g_object_get(s,"label",&label,"tooltip", &tooltip, NULL);

  GtkWidget *b = gtk_toggle_button_new();
  GtkWidget *i = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,4);

  GtkWidget *l = gtk_label_new(label);

  gtk_widget_set_tooltip_text(GTK_WIDGET(b),tooltip);

  g_free(label);
  g_free(tooltip);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(b),b_scatter_series_get_show(s));
  g_object_bind_property(s,"show",b,"active", G_BINDING_BIDIRECTIONAL);

  cairo_surface_t *surf = _b_scatter_series_create_legend_image(s);
  GdkPixbuf *pb =gdk_pixbuf_get_from_surface(surf,0,0,cairo_image_surface_get_width(surf),cairo_image_surface_get_height(surf));
  GtkWidget *im = gtk_picture_new_for_pixbuf(pb);
  cairo_surface_destroy(surf);

  gtk_box_append(GTK_BOX(i),im);
  gtk_box_append(GTK_BOX(i),l);
  gtk_button_set_child(GTK_BUTTON(b),i);
  gtk_box_append(g,b);
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

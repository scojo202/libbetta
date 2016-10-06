/*
 * y-plot-widget.h
 *
 * Copyright (C) 2016 Scott O. Johnson (scojo202@gmail.com)
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
#include "y-plot-widget.h"

static GObjectClass *parent_class = NULL;

enum {
  PLOT_WIDGET_FRAME_RATE = 1,
  N_PROPERTIES
};

typedef struct _YPlotWidgetPrivate YPlotWidgetPrivate;
struct _YPlotWidgetPrivate {
    double max_frame_rate; // negative or zero if disabled
    guint frame_rate_timer;
    
    GtkGrid *grid;
    GtkOverlay * overlay;
};

static gboolean
thaw_timer(gpointer data)
{
  YPlotWidget *plot = Y_PLOT_WIDGET(data);
  if(plot==NULL)
    return FALSE;
    
  if(plot->priv->max_frame_rate <=0) {
    y_plot_widget_thaw(plot);
    return FALSE;
  }
  
  y_plot_widget_thaw(plot);
  y_plot_widget_freeze(plot);
  
  return TRUE;
}

static
void y_plot_widget_finalize(GObject *obj)
{
  YPlotWidget * pw = (YPlotWidget *) obj;
  
  if(pw->priv->frame_rate_timer)
    g_source_remove_by_user_data(pw);
  
  if (parent_class->finalize)
    parent_class->finalize (obj);
}

static void
y_plot_widget_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
    YPlotWidget *plot = (YPlotWidget *) object;
    
    switch (property_id) {
    case PLOT_WIDGET_FRAME_RATE: {
      plot->priv->max_frame_rate = g_value_get_double (value);
      y_plot_widget_freeze(plot);
      plot->priv->frame_rate_timer = g_timeout_add(1000.0/fabs(plot->priv->max_frame_rate),thaw_timer,plot);

    }
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
      break;
    }
}


static void
y_plot_widget_get_property (GObject      *object,
                        guint         property_id,
                        GValue       *value,
                        GParamSpec   *pspec)
{
    YPlotWidget *self = (YPlotWidget *) object;
    switch (property_id) {
    case PLOT_WIDGET_FRAME_RATE: {
      g_value_set_double (value, self->priv->max_frame_rate);
    }
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
      break;
    }
}


static void
y_plot_widget_class_init (YPlotWidgetClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  
  object_class->set_property = y_plot_widget_set_property;
  object_class->get_property = y_plot_widget_get_property;
  
  g_object_class_install_property (object_class, PLOT_WIDGET_FRAME_RATE, 
                    g_param_spec_double ("max_frame_rate", "Maximum frame rate", "Maximum frame rate",
                                        -1, 100.0, 0.0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  
  /* properties */

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = y_plot_widget_finalize;
}

static void
y_plot_widget_init (YPlotWidget * obj)
{
  YPlotWidgetPrivate *p;
  p = obj->priv = g_new0 (YPlotWidgetPrivate, 1);
  
  obj->priv->grid = GTK_GRID(gtk_grid_new());
  gtk_container_add(GTK_CONTAINER(obj),GTK_WIDGET(obj->priv->grid));
  GtkGrid *grid = GTK_GRID(obj->priv->grid);
  
  obj->series = NULL;
  
  GtkStyleContext *stc;
  GtkCssProvider *cssp = gtk_css_provider_new();
  gchar *css = g_strdup_printf("GtkEventBox {background:none; background-color:%s; }","#ffffff");
  gtk_css_provider_load_from_data(cssp, css, -1, NULL);
  stc = gtk_widget_get_style_context(GTK_WIDGET(obj));
  gtk_style_context_add_provider(stc, GTK_STYLE_PROVIDER(cssp), GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_free(css);
    
  gtk_grid_insert_column(grid,0);
  gtk_grid_insert_column(grid,1);
  gtk_grid_insert_column(grid,2);
  
  obj->west_axis = y_axis_view_new(WEST);
  obj->south_axis = y_axis_view_new(SOUTH);
  obj->east_axis = y_axis_view_new(EAST);
  obj->north_axis = y_axis_view_new(NORTH);
  
  obj->main_view = NULL;
  
  gtk_grid_attach(grid,GTK_WIDGET(obj->north_axis),1,0,1,1);
  gtk_grid_attach(grid,GTK_WIDGET(obj->west_axis),0,1,1,1);
  gtk_grid_attach(grid,GTK_WIDGET(obj->south_axis),1,2,1,1);
  gtk_grid_attach(grid,GTK_WIDGET(obj->east_axis),2,1,1,1);
  
  g_object_set(obj,"vexpand",FALSE,"hexpand",FALSE,"halign",GTK_ALIGN_START,"valign",GTK_ALIGN_START,NULL);
  g_object_set(grid,"vexpand",FALSE,"hexpand",FALSE,"halign",GTK_ALIGN_START,"valign",GTK_ALIGN_START,NULL);
  
  g_object_set(obj->north_axis,"show_major_labels",FALSE,NULL);
  g_object_set(obj->east_axis,"show_major_labels",FALSE,NULL);
  
  obj->priv->overlay = GTK_OVERLAY(gtk_overlay_new());
  gtk_grid_attach(GTK_GRID(grid),GTK_WIDGET(obj->priv->overlay),1,1,1,1);
}

G_DEFINE_TYPE (YPlotWidget, y_plot_widget, GTK_TYPE_EVENT_BOX);

void y_plot_widget_add_view(YPlotWidget *self, YElementViewCartesian *view)
{
  g_assert(gtk_bin_get_child(GTK_BIN(self->priv->overlay))==NULL);
  
  gtk_container_add(GTK_CONTAINER(self->priv->overlay),GTK_WIDGET(view));
  
  if(self->main_view == NULL) self->main_view = view;
  
  y_element_view_cartesian_add_view_interval (view, X_AXIS);
  y_element_view_cartesian_add_view_interval (view, Y_AXIS);
    
  y_element_view_cartesian_connect_view_intervals (view, Y_AXIS,
    					     Y_ELEMENT_VIEW_CARTESIAN(self->east_axis), META_AXIS);
  y_element_view_cartesian_connect_view_intervals (view, Y_AXIS,
    					     Y_ELEMENT_VIEW_CARTESIAN(self->west_axis), META_AXIS);
  y_element_view_cartesian_connect_view_intervals (view, X_AXIS,
    					     Y_ELEMENT_VIEW_CARTESIAN(self->north_axis), META_AXIS);
  y_element_view_cartesian_connect_view_intervals (view, X_AXIS,
    					     Y_ELEMENT_VIEW_CARTESIAN(self->south_axis), META_AXIS);
  
  y_element_view_cartesian_connect_axis_markers (Y_ELEMENT_VIEW_CARTESIAN(self->south_axis), META_AXIS, view, X_AXIS );
  y_element_view_cartesian_set_axis_marker_type (Y_ELEMENT_VIEW_CARTESIAN(self->south_axis),
                                META_AXIS, Y_AXIS_SCALAR);
  
  y_element_view_cartesian_connect_axis_markers (Y_ELEMENT_VIEW_CARTESIAN(self->north_axis), META_AXIS, view, X_AXIS );
  y_element_view_cartesian_set_axis_marker_type (Y_ELEMENT_VIEW_CARTESIAN(self->north_axis),
                                META_AXIS, Y_AXIS_SCALAR);
                                
  y_element_view_cartesian_connect_axis_markers (Y_ELEMENT_VIEW_CARTESIAN(self->west_axis), META_AXIS, view, Y_AXIS );
  y_element_view_cartesian_set_axis_marker_type (Y_ELEMENT_VIEW_CARTESIAN(self->west_axis),
                                META_AXIS, Y_AXIS_SCALAR);
                                
  y_element_view_cartesian_connect_axis_markers (Y_ELEMENT_VIEW_CARTESIAN(self->east_axis), META_AXIS, view, Y_AXIS );
  y_element_view_cartesian_set_axis_marker_type (Y_ELEMENT_VIEW_CARTESIAN(self->east_axis),
                                META_AXIS, Y_AXIS_SCALAR);
}

YScatterView * y_plot_widget_add_line_data (YPlotWidget * plot, YVector * x, YVector * y)
{
    SeqPair * pair = g_slice_new (SeqPair);
    pair->xdata = x;
    pair->ydata = y;
    
    // refs?
    plot->series = g_slist_append (plot->series, pair);
    
    YScatterView * scatter = g_object_new (Y_TYPE_SCATTER_VIEW, "xdata", x, "ydata", y, NULL );
    y_scatter_view_set_line_color_from_string (scatter, "#0000FF");
    
    pair->view = scatter;
    
    YElementViewCartesian * cart = Y_ELEMENT_VIEW_CARTESIAN(scatter);
    
    if (g_slist_length (plot->series) == 1) {
        y_element_view_cartesian_connect_view_intervals (
    					     Y_ELEMENT_VIEW_CARTESIAN(plot->east_axis), META_AXIS, cart, Y_AXIS);
        y_element_view_cartesian_connect_view_intervals (
    					     Y_ELEMENT_VIEW_CARTESIAN(plot->west_axis), META_AXIS, cart, Y_AXIS);
        y_element_view_cartesian_connect_view_intervals (
    					     Y_ELEMENT_VIEW_CARTESIAN(plot->north_axis), META_AXIS, cart, X_AXIS);
        y_element_view_cartesian_connect_view_intervals (
    					     Y_ELEMENT_VIEW_CARTESIAN(plot->south_axis), META_AXIS, cart, X_AXIS);
        
        y_element_view_cartesian_set_axis_marker_type (Y_ELEMENT_VIEW_CARTESIAN(plot->east_axis), 
                            META_AXIS, Y_AXIS_SCALAR);
        y_element_view_cartesian_set_axis_marker_type (Y_ELEMENT_VIEW_CARTESIAN(plot->west_axis),
                                META_AXIS, Y_AXIS_SCALAR);
        y_element_view_cartesian_set_axis_marker_type (Y_ELEMENT_VIEW_CARTESIAN(plot->north_axis),
                                META_AXIS, Y_AXIS_SCALAR);
        y_element_view_cartesian_set_axis_marker_type (Y_ELEMENT_VIEW_CARTESIAN(plot->south_axis),
                                META_AXIS, Y_AXIS_SCALAR);
                                
        gtk_container_add(GTK_CONTAINER(plot->priv->overlay),GTK_WIDGET(scatter));
    }
    else {
      gtk_overlay_add_overlay(plot->priv->overlay, GTK_WIDGET(scatter));
    }
    
    y_element_view_cartesian_connect_axis_markers (Y_ELEMENT_VIEW_CARTESIAN(plot->east_axis), META_AXIS, cart, Y_AXIS );
    y_element_view_cartesian_connect_axis_markers (Y_ELEMENT_VIEW_CARTESIAN(plot->west_axis), META_AXIS, cart, Y_AXIS );
    y_element_view_cartesian_connect_axis_markers (Y_ELEMENT_VIEW_CARTESIAN(plot->north_axis), META_AXIS, cart, X_AXIS );
    y_element_view_cartesian_connect_axis_markers (Y_ELEMENT_VIEW_CARTESIAN(plot->south_axis), META_AXIS, cart, X_AXIS );
    
    return scatter;
}

static
void freeze_view (gpointer data, gpointer user_data)
{
  SeqPair *p = (SeqPair *) data;
  y_element_view_freeze(Y_ELEMENT_VIEW(p->view));
}

static
void thaw_view (gpointer data, gpointer user_data)
{
  SeqPair *p = (SeqPair *) data;
  y_element_view_thaw(Y_ELEMENT_VIEW(p->view));
}

void y_plot_widget_freeze(YPlotWidget *plot)
{
  y_element_view_freeze(Y_ELEMENT_VIEW(plot->south_axis));
  y_element_view_freeze(Y_ELEMENT_VIEW(plot->north_axis));
  y_element_view_freeze(Y_ELEMENT_VIEW(plot->west_axis));
  y_element_view_freeze(Y_ELEMENT_VIEW(plot->east_axis));
  g_slist_foreach(plot->series,freeze_view, NULL);
}

void y_plot_widget_thaw(YPlotWidget *plot)
{
  y_element_view_thaw(Y_ELEMENT_VIEW(plot->south_axis));
  y_element_view_thaw(Y_ELEMENT_VIEW(plot->north_axis));
  y_element_view_thaw(Y_ELEMENT_VIEW(plot->west_axis));
  y_element_view_thaw(Y_ELEMENT_VIEW(plot->east_axis));
  g_slist_foreach(plot->series,thaw_view, NULL);
}

gboolean y_plot_widget_draw_pending(YPlotWidget *plot)
{
  gboolean d = FALSE;
  GSList *s = plot->series;
  if(s==NULL)
    return FALSE;
  SeqPair *p = (SeqPair*) s->data;
  d = d || Y_ELEMENT_VIEW(p->view)->draw_pending;
  while(s = g_slist_next(s)) {
    SeqPair *p = (SeqPair*) s->data;
    if(s->data)
      d = d || Y_ELEMENT_VIEW(p->view)->draw_pending;
  }
  d = d || Y_ELEMENT_VIEW(plot->south_axis)->draw_pending;
  d = d || Y_ELEMENT_VIEW(plot->north_axis)->draw_pending;
  d = d || Y_ELEMENT_VIEW(plot->west_axis)->draw_pending;
  d = d || Y_ELEMENT_VIEW(plot->east_axis)->draw_pending;
  return d;
}

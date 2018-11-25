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
#include "y-scatter-line-plot.h"

static GObjectClass *parent_class = NULL;

enum {
  SCATTER_LINE_PLOT_FRAME_RATE = 1,
  N_PROPERTIES
};

typedef struct _YScatterLinePlotPrivate YScatterLinePlotPrivate;
struct _YScatterLinePlotPrivate {
  double max_frame_rate; // negative or zero if disabled
  guint frame_rate_timer;
  gboolean view_intervals_connected;
  gboolean zooming;
  gboolean panning;

  GtkGrid *grid;
  GtkLabel *pos_label;
};

static gboolean
thaw_timer(gpointer data)
{
  YScatterLinePlot *plot = Y_SCATTER_LINE_PLOT(data);
  if(plot==NULL)
    return FALSE;

  if(plot->priv->max_frame_rate <=0) {
    y_scatter_line_plot_thaw(plot);
    return FALSE;
  }

  y_scatter_line_plot_thaw(plot);
  y_scatter_line_plot_freeze(plot);

  return TRUE;
}

static
void y_scatter_line_plot_finalize(GObject *obj)
{
  YScatterLinePlot * pw = (YScatterLinePlot *) obj;

  if(pw->priv->frame_rate_timer)
    g_source_remove_by_user_data(pw);

  if (parent_class->finalize)
    parent_class->finalize (obj);
}

static void
y_scatter_line_plot_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
    YScatterLinePlot *plot = (YScatterLinePlot *) object;

    switch (property_id) {
    case SCATTER_LINE_PLOT_FRAME_RATE: {
      plot->priv->max_frame_rate = g_value_get_double (value);
      y_scatter_line_plot_freeze(plot);
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
y_scatter_line_plot_get_property (GObject      *object,
                        guint         property_id,
                        GValue       *value,
                        GParamSpec   *pspec)
{
    YScatterLinePlot *self = (YScatterLinePlot *) object;
    switch (property_id) {
    case SCATTER_LINE_PLOT_FRAME_RATE: {
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
y_scatter_line_plot_class_init (YScatterLinePlotClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->set_property = y_scatter_line_plot_set_property;
  object_class->get_property = y_scatter_line_plot_get_property;

  g_object_class_install_property (object_class, SCATTER_LINE_PLOT_FRAME_RATE,
                    g_param_spec_double ("max-frame-rate", "Maximum frame rate", "Maximum frame rate",
                                        -1, 100.0, 0.0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  /* properties */

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = y_scatter_line_plot_finalize;
}

static
void zoom_toggled(GtkToggleToolButton *toggle_tool_button,
               gpointer             user_data)
{
  YScatterLinePlot *plot = (YScatterLinePlot *) user_data;
  y_element_view_set_zooming(Y_ELEMENT_VIEW(plot->south_axis),gtk_toggle_tool_button_get_active(toggle_tool_button));
  y_element_view_set_zooming(Y_ELEMENT_VIEW(plot->north_axis),gtk_toggle_tool_button_get_active(toggle_tool_button));
  y_element_view_set_zooming(Y_ELEMENT_VIEW(plot->west_axis),gtk_toggle_tool_button_get_active(toggle_tool_button));
  y_element_view_set_zooming(Y_ELEMENT_VIEW(plot->east_axis),gtk_toggle_tool_button_get_active(toggle_tool_button));
  y_element_view_set_zooming(Y_ELEMENT_VIEW(plot->main_view),gtk_toggle_tool_button_get_active(toggle_tool_button));
}

static
void pan_toggled(GtkToggleToolButton *toggle_tool_button,
               gpointer             user_data)
{

}

static void
y_scatter_line_plot_init (YScatterLinePlot * obj)
{
  obj->priv = g_new0 (YScatterLinePlotPrivate, 1);

  obj->priv->grid = GTK_GRID(gtk_grid_new());
  gtk_container_add(GTK_CONTAINER(obj),GTK_WIDGET(obj->priv->grid));
  GtkGrid *grid = GTK_GRID(obj->priv->grid);

  GtkStyleContext *stc;
  GtkCssProvider *cssp = gtk_css_provider_new();
  gchar *css = g_strdup_printf("grid {background-color:%s; }","#ffffff");
  gtk_css_provider_load_from_data(cssp, css, -1, NULL);
  stc = gtk_widget_get_style_context(GTK_WIDGET(grid));
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

  g_object_set(obj->north_axis,"show-major-labels",FALSE,NULL);
  g_object_set(obj->east_axis,"show-major-labels",FALSE,NULL);

  YScatterLineView *view = g_object_new(Y_TYPE_SCATTER_LINE_VIEW, NULL);

  obj->main_view = view;

  y_element_view_cartesian_add_view_interval (Y_ELEMENT_VIEW_CARTESIAN(view), X_AXIS);
  y_element_view_cartesian_add_view_interval (Y_ELEMENT_VIEW_CARTESIAN(view), Y_AXIS);

  y_element_view_cartesian_connect_view_intervals (Y_ELEMENT_VIEW_CARTESIAN(view), Y_AXIS,
    					     Y_ELEMENT_VIEW_CARTESIAN(obj->east_axis), META_AXIS);
  y_element_view_cartesian_connect_view_intervals (Y_ELEMENT_VIEW_CARTESIAN(view), Y_AXIS,
    					     Y_ELEMENT_VIEW_CARTESIAN(obj->west_axis), META_AXIS);
  y_element_view_cartesian_connect_view_intervals (Y_ELEMENT_VIEW_CARTESIAN(view), X_AXIS,
    					     Y_ELEMENT_VIEW_CARTESIAN(obj->north_axis), META_AXIS);
  y_element_view_cartesian_connect_view_intervals (Y_ELEMENT_VIEW_CARTESIAN(view), X_AXIS,
    					     Y_ELEMENT_VIEW_CARTESIAN(obj->south_axis), META_AXIS);

  y_element_view_cartesian_connect_axis_markers (Y_ELEMENT_VIEW_CARTESIAN(obj->south_axis), META_AXIS, Y_ELEMENT_VIEW_CARTESIAN(view), X_AXIS );
  y_element_view_cartesian_set_axis_marker_type (Y_ELEMENT_VIEW_CARTESIAN(obj->south_axis),
                                META_AXIS, Y_AXIS_SCALAR);

  y_element_view_cartesian_connect_axis_markers (Y_ELEMENT_VIEW_CARTESIAN(obj->north_axis), META_AXIS, Y_ELEMENT_VIEW_CARTESIAN(view), X_AXIS );
  y_element_view_cartesian_set_axis_marker_type (Y_ELEMENT_VIEW_CARTESIAN(obj->north_axis),
                                META_AXIS, Y_AXIS_SCALAR);

  y_element_view_cartesian_connect_axis_markers (Y_ELEMENT_VIEW_CARTESIAN(obj->west_axis), META_AXIS, Y_ELEMENT_VIEW_CARTESIAN(view), Y_AXIS );
  y_element_view_cartesian_set_axis_marker_type (Y_ELEMENT_VIEW_CARTESIAN(obj->west_axis),
                                META_AXIS, Y_AXIS_SCALAR);

  y_element_view_cartesian_connect_axis_markers (Y_ELEMENT_VIEW_CARTESIAN(obj->east_axis), META_AXIS, Y_ELEMENT_VIEW_CARTESIAN(view), Y_AXIS );
  y_element_view_cartesian_set_axis_marker_type (Y_ELEMENT_VIEW_CARTESIAN(obj->east_axis),
                                META_AXIS, Y_AXIS_SCALAR);

  gtk_grid_attach(GTK_GRID(grid),GTK_WIDGET(obj->main_view),1,1,1,1);

  /* create toolbar */
  GtkWidget *toolbar = gtk_toolbar_new();

  GtkToolItem *zoom_button = gtk_toggle_tool_button_new();
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(zoom_button),"Zoom");
  gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(zoom_button),"edit-find");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),zoom_button,0);
  g_signal_connect(zoom_button,"toggled",G_CALLBACK(zoom_toggled),obj);

  GtkToolItem *pan_button = gtk_toggle_tool_button_new();
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(pan_button),"Pan");
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),pan_button,-1);
  g_signal_connect(pan_button,"toggled",G_CALLBACK(pan_toggled),obj);

  GtkToolItem *pos_item = GTK_TOOL_ITEM(gtk_tool_item_new());
  gtk_tool_item_set_homogeneous(pos_item,FALSE);
  obj->priv->pos_label = GTK_LABEL(gtk_label_new("()"));
  gtk_container_add(GTK_CONTAINER(pos_item),GTK_WIDGET(obj->priv->pos_label));
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),pos_item,-1);

  if(Y_IS_SCATTER_LINE_VIEW(obj->main_view)) {
    y_scatter_line_view_set_pos_label(obj->main_view,obj->priv->pos_label);
  }

  gtk_grid_attach(GTK_GRID(grid),toolbar,0,3,3,1);
}

G_DEFINE_TYPE (YScatterLinePlot, y_scatter_line_plot, GTK_TYPE_EVENT_BOX);

void y_scatter_line_plot_freeze(YScatterLinePlot *plot)
{
  y_element_view_freeze(Y_ELEMENT_VIEW(plot->south_axis));
  y_element_view_freeze(Y_ELEMENT_VIEW(plot->north_axis));
  y_element_view_freeze(Y_ELEMENT_VIEW(plot->west_axis));
  y_element_view_freeze(Y_ELEMENT_VIEW(plot->east_axis));
  y_element_view_freeze(Y_ELEMENT_VIEW(plot->main_view));
}

void y_scatter_line_plot_thaw(YScatterLinePlot *plot)
{
  y_element_view_thaw(Y_ELEMENT_VIEW(plot->south_axis));
  y_element_view_thaw(Y_ELEMENT_VIEW(plot->north_axis));
  y_element_view_thaw(Y_ELEMENT_VIEW(plot->west_axis));
  y_element_view_thaw(Y_ELEMENT_VIEW(plot->east_axis));
  y_element_view_thaw(Y_ELEMENT_VIEW(plot->main_view));
}

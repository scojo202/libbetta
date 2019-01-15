/*
 * y-plot-widget.c
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

#include <math.h>
#include "plot/y-plot-widget.h"
#include "plot/y-density-view.h"

/**
 * SECTION: y-plot-widget
 * @short_description: Widget showing a single plot surrounded by axes.
 *
 * This is a widget that shows a single plot surrounded by axes. Currently, this
 * the plot can be either an XY (scatter) plot or a density plot. The widget
 * also includes a toolbar with controls for zooming and translating (panning)
 * the region shown.
 *
 * #YPlotWidget also includes a mechanism for throttling the rate of updates in
 * response to changes in the data being plotted. This is done using the
 * "max-frame-rate" property.
 *
 */

static GObjectClass *parent_class = NULL;

enum
{
  PROP_FRAME_RATE = 1,
  PROP_SHOW_TOOLBAR,
  N_PROPERTIES
};

struct _YPlotWidget
{
  GtkGrid base;
  YAxisView * north_axis;
  YAxisView * south_axis;
  YAxisView * west_axis;
  YAxisView * east_axis;

  YElementViewCartesian *main_view;
  double max_frame_rate;	// negative or zero if disabled
  guint frame_rate_timer;
  gboolean show_toolbar;
  GtkToolbar *toolbar;

  GtkLabel *pos_label;
};

static gboolean
thaw_timer (gpointer data)
{
  YPlotWidget *plot = Y_PLOT_WIDGET (data);
  if (plot == NULL)
    return FALSE;

  if (plot->max_frame_rate <= 0)
    {
      y_plot_thaw_all (GTK_CONTAINER(plot));
      return FALSE;
    }

  y_plot_thaw_all (GTK_CONTAINER(plot));
  y_plot_freeze_all (GTK_CONTAINER(plot));

  return TRUE;
}

static void
y_plot_widget_finalize (GObject * obj)
{
  YPlotWidget *pw = (YPlotWidget *) obj;

  if (pw->frame_rate_timer)
    g_source_remove_by_user_data (pw);

  if (parent_class->finalize)
    parent_class->finalize (obj);
}

static void
y_plot_widget_set_property (GObject * object,
				  guint property_id,
				  const GValue * value, GParamSpec * pspec)
{
  YPlotWidget *plot = (YPlotWidget *) object;

  switch (property_id)
    {
    case PROP_FRAME_RATE:
      {
        plot->max_frame_rate = g_value_get_double (value);
        y_plot_freeze_all (GTK_CONTAINER(plot));
        plot->frame_rate_timer =
        g_timeout_add (1000.0 / fabs (plot->max_frame_rate),
          thaw_timer, plot);
      }
      break;
    case PROP_SHOW_TOOLBAR:
      {
        plot->show_toolbar = g_value_get_boolean (value);
        if (plot->show_toolbar)
        {
          gtk_widget_show (GTK_WIDGET (plot->toolbar));
        }
        else
        {
          gtk_widget_hide (GTK_WIDGET (plot->toolbar));
          gtk_widget_set_no_show_all (GTK_WIDGET (plot->toolbar),
					TRUE);
        }
      }
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static void
y_plot_widget_get_property (GObject * object,
				  guint property_id,
				  GValue * value, GParamSpec * pspec)
{
  YPlotWidget *self = (YPlotWidget *) object;
  switch (property_id)
    {
    case PROP_FRAME_RATE:
      {
        g_value_set_double (value, self->max_frame_rate);
      }
      break;
    case PROP_SHOW_TOOLBAR:
      {
        g_value_set_boolean (value, self->show_toolbar);
      }
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static void
y_plot_widget_class_init (YPlotWidgetClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->set_property = y_plot_widget_set_property;
  object_class->get_property = y_plot_widget_get_property;

  /* properties */

  g_object_class_install_property (object_class, PROP_FRAME_RATE,
				   g_param_spec_double ("max-frame-rate",
							"Maximum frame rate",
							"Maximum frame rate in 1/s; used to throttle refresh speed",
							-1, 100.0, 0.0,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_SHOW_TOOLBAR,
				   g_param_spec_boolean ("show-toolbar",
							 "Whether to show the toolbar",
							 "Whether the toolbar should be shown.", TRUE,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = y_plot_widget_finalize;
}

static void
y_plot_widget_init (YPlotWidget * obj)
{
  GtkGrid *grid = GTK_GRID (obj);

  GtkStyleContext *stc;
  GtkCssProvider *cssp = gtk_css_provider_new ();
  gchar *css = g_strdup_printf ("grid {background-color:%s; }", "#ffffff");
  gtk_css_provider_load_from_data (cssp, css, -1, NULL);
  stc = gtk_widget_get_style_context (GTK_WIDGET (grid));
  gtk_style_context_add_provider (stc, GTK_STYLE_PROVIDER (cssp),
				  GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_free (css);

  obj->west_axis = y_axis_view_new (Y_COMPASS_WEST);
  obj->south_axis = y_axis_view_new (Y_COMPASS_SOUTH);
  obj->east_axis = y_axis_view_new (Y_COMPASS_EAST);
  obj->north_axis = y_axis_view_new (Y_COMPASS_NORTH);

  obj->main_view = NULL;

  gtk_grid_attach (grid, GTK_WIDGET (obj->north_axis), 1, 0, 1, 1);
  gtk_grid_attach (grid, GTK_WIDGET (obj->west_axis), 0, 1, 1, 1);
  gtk_grid_attach (grid, GTK_WIDGET (obj->south_axis), 1, 2, 1, 1);
  gtk_grid_attach (grid, GTK_WIDGET (obj->east_axis), 2, 1, 1, 1);

  g_object_set (obj, "vexpand", FALSE, "hexpand", FALSE,
                     "halign", GTK_ALIGN_START, "valign", GTK_ALIGN_START, NULL);

  y_plot_freeze_all (GTK_CONTAINER(grid));

  g_object_set (obj->north_axis, "show-major-labels", FALSE, NULL);
  g_object_set (obj->east_axis, "show-major-labels", FALSE, NULL);

  /* create toolbar */
  obj->toolbar = y_plot_toolbar_new(GTK_CONTAINER(obj));

  GtkToolItem *pos_item = GTK_TOOL_ITEM (gtk_tool_item_new ());
  gtk_tool_item_set_homogeneous (pos_item, FALSE);
  obj->pos_label = GTK_LABEL (gtk_label_new ("()"));
  gtk_container_add (GTK_CONTAINER (pos_item),
		     GTK_WIDGET (obj->pos_label));
  gtk_toolbar_insert (obj->toolbar, pos_item, -1);

  gtk_grid_attach (GTK_GRID (grid), GTK_WIDGET (obj->toolbar), 0, 3, 3,
		   1);

  y_plot_thaw_all (GTK_CONTAINER(grid));
}

G_DEFINE_TYPE (YPlotWidget, y_plot_widget, GTK_TYPE_GRID);

/**
 * y_plot_widget_add_view:
 * @obj: a #YPlotWidget
 * @view: a #YElementViewCartesian, such as a scatter view or density view
 *
 * Add a main view to a plot
 **/
void y_plot_widget_add_view(YPlotWidget *obj, YElementViewCartesian *view)
{
  obj->main_view = Y_ELEMENT_VIEW_CARTESIAN(view);

  gtk_grid_attach (GTK_GRID (obj), GTK_WIDGET (obj->main_view), 1, 1, 1, 1);

  y_element_view_cartesian_connect_view_intervals (obj->main_view, Y_AXIS,
  					   Y_ELEMENT_VIEW_CARTESIAN
  					   (obj->east_axis),
  					   META_AXIS);
  y_element_view_cartesian_connect_view_intervals (obj->main_view, Y_AXIS,
  					   Y_ELEMENT_VIEW_CARTESIAN
  					   (obj->west_axis),
  					   META_AXIS);
  y_element_view_cartesian_connect_view_intervals (obj->main_view, X_AXIS,
  					   Y_ELEMENT_VIEW_CARTESIAN
  					   (obj->north_axis),
  					   META_AXIS);
  y_element_view_cartesian_connect_view_intervals (obj->main_view, X_AXIS,
  					   Y_ELEMENT_VIEW_CARTESIAN
  					   (obj->south_axis),
  					   META_AXIS);

  y_element_view_cartesian_set_axis_marker_type (Y_ELEMENT_VIEW_CARTESIAN
  					 (obj->south_axis), META_AXIS,
  					 Y_AXIS_SCALAR);
  y_element_view_cartesian_connect_axis_markers (Y_ELEMENT_VIEW_CARTESIAN
  					 (obj->south_axis), META_AXIS,
  					 obj->north_axis, META_AXIS);

  y_element_view_cartesian_set_axis_marker_type (Y_ELEMENT_VIEW_CARTESIAN
  					 (obj->west_axis), META_AXIS,
  					 Y_AXIS_SCALAR);
  y_element_view_cartesian_connect_axis_markers (Y_ELEMENT_VIEW_CARTESIAN
  					 (obj->west_axis), META_AXIS,
  					 obj->east_axis, META_AXIS);
}

/**
 * y_plot_widget_new_scatter:
 * @series: (nullable): a series to add to the plot, or %NULL
 *
 * Create a #YPlotWidget with a #YScatterLineView.
 *
 * Returns: the new plot
 **/
YPlotWidget * y_plot_widget_new_scatter(YScatterSeries *series)
{
  YPlotWidget *obj = g_object_new(Y_TYPE_PLOT_WIDGET, NULL);
  YScatterLineView *view = g_object_new (Y_TYPE_SCATTER_LINE_VIEW, NULL);

  y_plot_widget_add_view(obj,Y_ELEMENT_VIEW_CARTESIAN(view));

  y_scatter_line_view_set_pos_label (Y_SCATTER_LINE_VIEW(obj->main_view),
       obj->pos_label);

  if(Y_IS_SCATTER_SERIES (series))
    y_scatter_line_view_add_series(view,series);

  return obj;
}

/**
 * y_plot_widget_new_density:
 *
 * Create a #YPlotWidget with a #YDensityView.
 *
 * Returns: the new plot
 **/
YPlotWidget * y_plot_widget_new_density(void)
{
  YPlotWidget *obj = g_object_new(Y_TYPE_PLOT_WIDGET, NULL);
  YDensityView *view = g_object_new (Y_TYPE_DENSITY_VIEW, NULL);

  y_plot_widget_add_view(obj,Y_ELEMENT_VIEW_CARTESIAN(view));

  y_density_view_set_pos_label ( Y_DENSITY_VIEW(obj->main_view),
                                      obj->pos_label);

  return obj;
}

/**
 * y_plot_widget_get_main_view:
 * @plot: a #YPlotWidget
 *
 * Get the main view from the plot widget.
 *
 * Returns: (transfer none): the view
 **/
YElementViewCartesian * y_plot_widget_get_main_view(YPlotWidget *plot)
{
  return plot->main_view;
}

/**
 * y_plot_widget_get_axis_view:
 * @plot: a #YPlotWidget
 * @c: a compass direction
 *
 * Get one of the four axis views from the plot widget.
 *
 * Returns: (transfer none): the axis view for direction @c.
 **/
YAxisView *y_plot_widget_get_axis_view(YPlotWidget *plot, YCompass c)
{
  switch (c)
    {
    case Y_COMPASS_EAST:
      {
        return plot->east_axis;
      }
      break;
    case Y_COMPASS_WEST:
      {
        return plot->west_axis;
      }
      break;
    case Y_COMPASS_NORTH:
      {
        return plot->north_axis;
      }
      break;
    case Y_COMPASS_SOUTH:
      {
        return plot->south_axis;
      }
      break;
    case Y_COMPASS_INVALID:
      {
        g_assert_not_reached ();
        return NULL;
      }
    }
}

/**
 * y_plot_widget_set_x_label:
 * @plot: a #YPlotWidget
 * @label: a label
 *
 * Set an axis label for the X (horizontal) axis.
 **/
void y_plot_widget_set_x_label(YPlotWidget *plot, const gchar *label)
{
  g_return_if_fail(Y_IS_PLOT_WIDGET(plot));
  g_object_set(plot->south_axis,"axis_label",label, NULL);
}

/**
 * y_plot_widget_set_y_label:
 * @plot: a #YPlotWidget
 * @label: a label
 *
 * Set an axis label for the Y (vertical) axis.
 **/
void y_plot_widget_set_y_label(YPlotWidget *plot, const gchar *label)
{
  g_return_if_fail(Y_IS_PLOT_WIDGET(plot));
  g_object_set(plot->west_axis,"axis_label",label, NULL);
}

/******************************/
/* utility functions for plot GUI */

static
void freeze_child(GtkWidget *widget, gpointer data)
{
  if(Y_IS_ELEMENT_VIEW(widget)) {
    y_element_view_freeze (Y_ELEMENT_VIEW (widget));
  }
}

/**
 * y_plot_freeze_all:
 * @c: a container with #YElementViews
 *
 * Freeze all #YElementView children of @c.
 **/
void
y_plot_freeze_all (GtkContainer * c)
{
  g_return_if_fail(GTK_IS_CONTAINER(c));
  gtk_container_foreach(c,freeze_child, NULL);
}

static
void thaw_child(GtkWidget *widget, gpointer data)
{
  if(Y_IS_ELEMENT_VIEW(widget)) {
    y_element_view_thaw (Y_ELEMENT_VIEW (widget));
  }
}

/**
 * y_plot_thaw_all:
 * @c: a container with #YElementViews
 *
 * Thaw all #YElementView children of @c.
 **/
void
y_plot_thaw_all (GtkContainer * c)
{
  g_return_if_fail(GTK_IS_CONTAINER(c));
  gtk_container_foreach(c,thaw_child, NULL);
}

/* following assumes that buttons are in a specific order */

static void
autoscale_child(GtkWidget *widget, gpointer data)
{
  if(Y_IS_ELEMENT_VIEW_CARTESIAN(widget)) {
    YElementViewCartesian *cart = Y_ELEMENT_VIEW_CARTESIAN(widget);
    YViewInterval *viy = y_element_view_cartesian_get_view_interval (cart,
                     Y_AXIS);
    YViewInterval *vix = y_element_view_cartesian_get_view_interval (cart,
                     X_AXIS);
    if(vix)
      y_view_interval_set_ignore_preferred_range (vix,FALSE);
    if(viy)
      y_view_interval_set_ignore_preferred_range (viy,FALSE);
  }
}

static void
autoscale_clicked (GtkToolButton *tool_button, gpointer user_data)
{
  GtkContainer *c = (GtkContainer *) user_data;
  gtk_container_foreach(c,autoscale_child,NULL);
}

static void
set_zooming_child(GtkWidget *widget, gpointer data)
{
  if(Y_IS_ELEMENT_VIEW(widget)) {
    y_element_view_set_zooming (Y_ELEMENT_VIEW (widget),GPOINTER_TO_INT(data));
  }
}

static void
set_panning_child(GtkWidget *widget, gpointer data)
{
  if(Y_IS_ELEMENT_VIEW(widget)) {
    y_element_view_set_panning (Y_ELEMENT_VIEW (widget),GPOINTER_TO_INT(data));
  }
}

static void
pan_toggled (GtkToggleToolButton * toggle_tool_button, gpointer user_data)
{
  GtkToolbar *toolbar = GTK_TOOLBAR(gtk_widget_get_parent(GTK_WIDGET(toggle_tool_button)));
  GtkContainer *c = (GtkContainer *) user_data;
  gboolean active = gtk_toggle_tool_button_get_active (toggle_tool_button);
  GtkToggleToolButton *zoom_button = GTK_TOGGLE_TOOL_BUTTON(gtk_toolbar_get_nth_item(toolbar, 1));
  if (active && gtk_toggle_tool_button_get_active (toggle_tool_button))
    {
      gtk_toggle_tool_button_set_active (zoom_button, FALSE);
    }
  gtk_container_foreach(c,set_panning_child,GINT_TO_POINTER(active));
}

static void
zoom_toggled (GtkToggleToolButton * toggle_tool_button, gpointer user_data)
{
  GtkToolbar *toolbar = GTK_TOOLBAR(gtk_widget_get_parent(GTK_WIDGET(toggle_tool_button)));
  GtkContainer *c = (GtkContainer *) user_data;
  gboolean active = gtk_toggle_tool_button_get_active (toggle_tool_button);
  GtkToggleToolButton *pan_button = GTK_TOGGLE_TOOL_BUTTON(gtk_toolbar_get_nth_item(toolbar, 2));
  if (active && gtk_toggle_tool_button_get_active (pan_button))
    {
      gtk_toggle_tool_button_set_active (pan_button, FALSE);
    }
  gtk_container_foreach(c,set_zooming_child,GINT_TO_POINTER(active));
}

/**
 * y_plot_toolbar_new:
 * @c: a container with #YElementViews
 *
 * Create a new toolbar for a plot or collection of plots. The items in the
 * toolbar will operate on all #YElementViews in @c.
 *
 * Returns: (transfer full): The new #GtkToolbar.
 **/
GtkToolbar *y_plot_toolbar_new (GtkContainer *c)
{
  GtkToolbar *toolbar = GTK_TOOLBAR (gtk_toolbar_new ());

  GtkToolButton *autoscale_button =
    GTK_TOOL_BUTTON (gtk_tool_button_new (NULL,"Autoscale"));
  //gtk_widget_set_tooltip_text(GTK_WIDGET(autoscale_button),"Autoscale");
  gtk_toolbar_insert (toolbar,
		      GTK_TOOL_ITEM (autoscale_button), 0);
  g_signal_connect (autoscale_button, "clicked",
		    G_CALLBACK (autoscale_clicked), c);

  GtkToggleToolButton *zoom_button =
    GTK_TOGGLE_TOOL_BUTTON (gtk_toggle_tool_button_new ());
  gtk_tool_button_set_label (GTK_TOOL_BUTTON (zoom_button),
			     "Zoom");
  gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (zoom_button),
				 "edit-find");
  gtk_widget_set_tooltip_text(GTK_WIDGET(zoom_button),"Zoom");
  gtk_toolbar_insert (toolbar,
		      GTK_TOOL_ITEM (zoom_button), 1);
  g_signal_connect (zoom_button, "toggled",
		    G_CALLBACK (zoom_toggled), c);

  GtkToggleToolButton *pan_button =
    GTK_TOGGLE_TOOL_BUTTON (gtk_toggle_tool_button_new ());
  gtk_tool_button_set_label (GTK_TOOL_BUTTON (pan_button), "Pan");
  //gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(obj->priv->pan_button),"go-home");
  gtk_toolbar_insert (toolbar,
		      GTK_TOOL_ITEM (pan_button), -1);
  g_signal_connect (pan_button, "toggled",
		    G_CALLBACK (pan_toggled), c);

  return toolbar;
}

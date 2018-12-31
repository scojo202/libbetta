/*
 * y-scatter-line-plot.c
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
#include "plot/y-scatter-line-plot.h"

static GObjectClass *parent_class = NULL;

enum
{
  PROP_FRAME_RATE = 1,
  PROP_SHOW_TOOLBAR,
  N_PROPERTIES
};

typedef struct _YScatterLinePlotPrivate YScatterLinePlotPrivate;
struct _YScatterLinePlotPrivate
{
  double max_frame_rate;	// negative or zero if disabled
  guint frame_rate_timer;
  gboolean show_toolbar;
  GtkToolbar *toolbar;
  GtkToggleToolButton *zoom_button;
  GtkToggleToolButton *pan_button;

  GtkGrid *grid;
  GtkLabel *pos_label;
};

static gboolean
thaw_timer (gpointer data)
{
  YScatterLinePlot *plot = Y_SCATTER_LINE_PLOT (data);
  if (plot == NULL)
    return FALSE;

  if (plot->priv->max_frame_rate <= 0)
    {
      y_scatter_line_plot_thaw (plot);
      return FALSE;
    }

  y_scatter_line_plot_thaw (plot);
  y_scatter_line_plot_freeze (plot);

  return TRUE;
}

static void
y_scatter_line_plot_finalize (GObject * obj)
{
  YScatterLinePlot *pw = (YScatterLinePlot *) obj;

  if (pw->priv->frame_rate_timer)
    g_source_remove_by_user_data (pw);

  if (parent_class->finalize)
    parent_class->finalize (obj);
}

static void
y_scatter_line_plot_set_property (GObject * object,
				  guint property_id,
				  const GValue * value, GParamSpec * pspec)
{
  YScatterLinePlot *plot = (YScatterLinePlot *) object;

  switch (property_id)
    {
    case PROP_FRAME_RATE:
      {
	plot->priv->max_frame_rate = g_value_get_double (value);
	y_scatter_line_plot_freeze (plot);
	plot->priv->frame_rate_timer =
	  g_timeout_add (1000.0 / fabs (plot->priv->max_frame_rate),
			 thaw_timer, plot);

      }
      break;
    case PROP_SHOW_TOOLBAR:
      {
	plot->priv->show_toolbar = g_value_get_boolean (value);
	if (plot->priv->show_toolbar)
	  {
	    gtk_widget_show (GTK_WIDGET (plot->priv->toolbar));
	  }
	else
	  {
	    gtk_widget_hide (GTK_WIDGET (plot->priv->toolbar));
	    gtk_widget_set_no_show_all (GTK_WIDGET (plot->priv->toolbar),
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
y_scatter_line_plot_get_property (GObject * object,
				  guint property_id,
				  GValue * value, GParamSpec * pspec)
{
  YScatterLinePlot *self = (YScatterLinePlot *) object;
  switch (property_id)
    {
    case PROP_FRAME_RATE:
      {
	g_value_set_double (value, self->priv->max_frame_rate);
      }
      break;
    case PROP_SHOW_TOOLBAR:
      {
	g_value_set_boolean (value, self->priv->show_toolbar);
      }
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static void
y_scatter_line_plot_class_init (YScatterLinePlotClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->set_property = y_scatter_line_plot_set_property;
  object_class->get_property = y_scatter_line_plot_get_property;

  /* properties */

  g_object_class_install_property (object_class, PROP_FRAME_RATE,
				   g_param_spec_double ("max-frame-rate",
							"Maximum frame rate",
							"Maximum frame rate",
							-1, 100.0, 0.0,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_SHOW_TOOLBAR,
				   g_param_spec_boolean ("show-toolbar",
							 "Whether to show the toolbar",
							 "", TRUE,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT |
							 G_PARAM_STATIC_STRINGS));

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = y_scatter_line_plot_finalize;
}

static void
autoscale_clicked (GtkToolButton *tool_button, gpointer user_data)
{
  YScatterLinePlot *plot = (YScatterLinePlot *) user_data;
  YElementViewCartesian *cart = (YElementViewCartesian *) plot->main_view;
  YViewInterval *viy = y_element_view_cartesian_get_view_interval (cart,
                   Y_AXIS);
  YViewInterval *vix = y_element_view_cartesian_get_view_interval (cart,
                   X_AXIS);
  y_view_interval_set_ignore_preferred_range (vix,FALSE);
  y_view_interval_set_ignore_preferred_range (viy,FALSE);
}

static void
zoom_toggled (GtkToggleToolButton * toggle_tool_button, gpointer user_data)
{
  YScatterLinePlot *plot = (YScatterLinePlot *) user_data;
  gboolean active = gtk_toggle_tool_button_get_active (toggle_tool_button);
  if (active && gtk_toggle_tool_button_get_active (plot->priv->pan_button))
    {
      gtk_toggle_tool_button_set_active (plot->priv->pan_button, FALSE);
    }
  y_element_view_set_zooming (Y_ELEMENT_VIEW (plot->south_axis), active);
  y_element_view_set_zooming (Y_ELEMENT_VIEW (plot->north_axis), active);
  y_element_view_set_zooming (Y_ELEMENT_VIEW (plot->west_axis), active);
  y_element_view_set_zooming (Y_ELEMENT_VIEW (plot->east_axis), active);
  y_element_view_set_zooming (Y_ELEMENT_VIEW (plot->main_view), active);
}

static void
pan_toggled (GtkToggleToolButton * toggle_tool_button, gpointer user_data)
{
  YScatterLinePlot *plot = (YScatterLinePlot *) user_data;
  gboolean active = gtk_toggle_tool_button_get_active (toggle_tool_button);
  if (active && gtk_toggle_tool_button_get_active (plot->priv->zoom_button))
    {
      gtk_toggle_tool_button_set_active (plot->priv->zoom_button, FALSE);
    }
  y_element_view_set_panning (Y_ELEMENT_VIEW (plot->south_axis), active);
  y_element_view_set_panning (Y_ELEMENT_VIEW (plot->north_axis), active);
  y_element_view_set_panning (Y_ELEMENT_VIEW (plot->west_axis), active);
  y_element_view_set_panning (Y_ELEMENT_VIEW (plot->east_axis), active);
  y_element_view_set_panning (Y_ELEMENT_VIEW (plot->main_view), active);
}

static void
y_scatter_line_plot_init (YScatterLinePlot * obj)
{
  obj->priv = g_new0 (YScatterLinePlotPrivate, 1);

  obj->priv->grid = GTK_GRID (gtk_grid_new ());
  gtk_container_add (GTK_CONTAINER (obj), GTK_WIDGET (obj->priv->grid));
  GtkGrid *grid = GTK_GRID (obj->priv->grid);

  GtkStyleContext *stc;
  GtkCssProvider *cssp = gtk_css_provider_new ();
  gchar *css = g_strdup_printf ("grid {background-color:%s; }", "#ffffff");
  gtk_css_provider_load_from_data (cssp, css, -1, NULL);
  stc = gtk_widget_get_style_context (GTK_WIDGET (grid));
  gtk_style_context_add_provider (stc, GTK_STYLE_PROVIDER (cssp),
				  GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_free (css);

  gtk_grid_insert_column (grid, 0);
  gtk_grid_insert_column (grid, 1);
  gtk_grid_insert_column (grid, 2);

  obj->west_axis = y_axis_view_new (Y_COMPASS_WEST);
  obj->south_axis = y_axis_view_new (Y_COMPASS_SOUTH);
  obj->east_axis = y_axis_view_new (Y_COMPASS_EAST);
  obj->north_axis = y_axis_view_new (Y_COMPASS_NORTH);

  obj->main_view = NULL;

  gtk_grid_attach (grid, GTK_WIDGET (obj->north_axis), 1, 0, 1, 1);
  gtk_grid_attach (grid, GTK_WIDGET (obj->west_axis), 0, 1, 1, 1);
  gtk_grid_attach (grid, GTK_WIDGET (obj->south_axis), 1, 2, 1, 1);
  gtk_grid_attach (grid, GTK_WIDGET (obj->east_axis), 2, 1, 1, 1);

  g_object_set (obj, "vexpand", FALSE, "hexpand", FALSE, "halign",
		GTK_ALIGN_START, "valign", GTK_ALIGN_START, NULL);
  g_object_set (grid, "vexpand", FALSE, "hexpand", FALSE, "halign",
		GTK_ALIGN_START, "valign", GTK_ALIGN_START, NULL);

  YScatterLineView *view = g_object_new (Y_TYPE_SCATTER_LINE_VIEW, NULL);

  obj->main_view = view;

  y_scatter_line_plot_freeze (obj);

  g_object_set (obj->north_axis, "show-major-labels", FALSE, NULL);
  g_object_set (obj->east_axis, "show-major-labels", FALSE, NULL);

  y_element_view_cartesian_add_view_interval (Y_ELEMENT_VIEW_CARTESIAN (view),
					      X_AXIS);
  y_element_view_cartesian_add_view_interval (Y_ELEMENT_VIEW_CARTESIAN (view),
					      Y_AXIS);

  y_element_view_cartesian_connect_view_intervals (Y_ELEMENT_VIEW_CARTESIAN
						   (view), Y_AXIS,
						   Y_ELEMENT_VIEW_CARTESIAN
						   (obj->east_axis),
						   META_AXIS);
  y_element_view_cartesian_connect_view_intervals (Y_ELEMENT_VIEW_CARTESIAN
						   (view), Y_AXIS,
						   Y_ELEMENT_VIEW_CARTESIAN
						   (obj->west_axis),
						   META_AXIS);
  y_element_view_cartesian_connect_view_intervals (Y_ELEMENT_VIEW_CARTESIAN
						   (view), X_AXIS,
						   Y_ELEMENT_VIEW_CARTESIAN
						   (obj->north_axis),
						   META_AXIS);
  y_element_view_cartesian_connect_view_intervals (Y_ELEMENT_VIEW_CARTESIAN
						   (view), X_AXIS,
						   Y_ELEMENT_VIEW_CARTESIAN
						   (obj->south_axis),
						   META_AXIS);

  y_element_view_cartesian_connect_axis_markers (Y_ELEMENT_VIEW_CARTESIAN
						 (obj->south_axis), META_AXIS,
						 Y_ELEMENT_VIEW_CARTESIAN
						 (view), X_AXIS);
  y_element_view_cartesian_set_axis_marker_type (Y_ELEMENT_VIEW_CARTESIAN
						 (obj->south_axis), META_AXIS,
						 Y_AXIS_SCALAR);

  y_element_view_cartesian_connect_axis_markers (Y_ELEMENT_VIEW_CARTESIAN
						 (obj->north_axis), META_AXIS,
						 Y_ELEMENT_VIEW_CARTESIAN
						 (view), X_AXIS);
  y_element_view_cartesian_set_axis_marker_type (Y_ELEMENT_VIEW_CARTESIAN
						 (obj->north_axis), META_AXIS,
						 Y_AXIS_SCALAR);

  y_element_view_cartesian_connect_axis_markers (Y_ELEMENT_VIEW_CARTESIAN
						 (obj->west_axis), META_AXIS,
						 Y_ELEMENT_VIEW_CARTESIAN
						 (view), Y_AXIS);
  y_element_view_cartesian_set_axis_marker_type (Y_ELEMENT_VIEW_CARTESIAN
						 (obj->west_axis), META_AXIS,
						 Y_AXIS_SCALAR);

  y_element_view_cartesian_connect_axis_markers (Y_ELEMENT_VIEW_CARTESIAN
						 (obj->east_axis), META_AXIS,
						 Y_ELEMENT_VIEW_CARTESIAN
						 (view), Y_AXIS);
  y_element_view_cartesian_set_axis_marker_type (Y_ELEMENT_VIEW_CARTESIAN
						 (obj->east_axis), META_AXIS,
						 Y_AXIS_SCALAR);

  gtk_grid_attach (GTK_GRID (grid), GTK_WIDGET (obj->main_view), 1, 1, 1, 1);

  /* create toolbar */
  obj->priv->toolbar = GTK_TOOLBAR (gtk_toolbar_new ());

  GtkToolButton *autoscale_button =
    GTK_TOOL_BUTTON (gtk_tool_button_new (NULL,"Autoscale"));
  //gtk_widget_set_tooltip_text(GTK_WIDGET(autoscale_button),"Autoscale");
  gtk_toolbar_insert (obj->priv->toolbar,
		      GTK_TOOL_ITEM (autoscale_button), 0);
  g_signal_connect (autoscale_button, "clicked",
		    G_CALLBACK (autoscale_clicked), obj);

  obj->priv->zoom_button =
    GTK_TOGGLE_TOOL_BUTTON (gtk_toggle_tool_button_new ());
  gtk_tool_button_set_label (GTK_TOOL_BUTTON (obj->priv->zoom_button),
			     "Zoom");
  gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (obj->priv->zoom_button),
				 "edit-find");
  gtk_widget_set_tooltip_text(GTK_WIDGET(obj->priv->zoom_button),"Zoom");
  gtk_toolbar_insert (obj->priv->toolbar,
		      GTK_TOOL_ITEM (obj->priv->zoom_button), 1);
  g_signal_connect (obj->priv->zoom_button, "toggled",
		    G_CALLBACK (zoom_toggled), obj);

  obj->priv->pan_button =
    GTK_TOGGLE_TOOL_BUTTON (gtk_toggle_tool_button_new ());
  gtk_tool_button_set_label (GTK_TOOL_BUTTON (obj->priv->pan_button), "Pan");
  //gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(obj->priv->pan_button),"go-home");
  gtk_toolbar_insert (obj->priv->toolbar,
		      GTK_TOOL_ITEM (obj->priv->pan_button), -1);
  g_signal_connect (obj->priv->pan_button, "toggled",
		    G_CALLBACK (pan_toggled), obj);

  GtkToolItem *pos_item = GTK_TOOL_ITEM (gtk_tool_item_new ());
  gtk_tool_item_set_homogeneous (pos_item, FALSE);
  obj->priv->pos_label = GTK_LABEL (gtk_label_new ("()"));
  gtk_container_add (GTK_CONTAINER (pos_item),
		     GTK_WIDGET (obj->priv->pos_label));
  gtk_toolbar_insert (obj->priv->toolbar, pos_item, -1);

  if (Y_IS_SCATTER_LINE_VIEW (obj->main_view))
    {
      y_scatter_line_view_set_pos_label (obj->main_view,
					 obj->priv->pos_label);
    }

  gtk_grid_attach (GTK_GRID (grid), GTK_WIDGET (obj->priv->toolbar), 0, 3, 3,
		   1);

  y_scatter_line_plot_thaw (obj);
}

G_DEFINE_TYPE (YScatterLinePlot, y_scatter_line_plot, GTK_TYPE_EVENT_BOX);

void
y_scatter_line_plot_freeze (YScatterLinePlot * plot)
{
  y_element_view_freeze (Y_ELEMENT_VIEW (plot->south_axis));
  y_element_view_freeze (Y_ELEMENT_VIEW (plot->north_axis));
  y_element_view_freeze (Y_ELEMENT_VIEW (plot->west_axis));
  y_element_view_freeze (Y_ELEMENT_VIEW (plot->east_axis));
  y_element_view_freeze (Y_ELEMENT_VIEW (plot->main_view));
}

void
y_scatter_line_plot_thaw (YScatterLinePlot * plot)
{
  y_element_view_thaw (Y_ELEMENT_VIEW (plot->south_axis));
  y_element_view_thaw (Y_ELEMENT_VIEW (plot->north_axis));
  y_element_view_thaw (Y_ELEMENT_VIEW (plot->west_axis));
  y_element_view_thaw (Y_ELEMENT_VIEW (plot->east_axis));
  y_element_view_thaw (Y_ELEMENT_VIEW (plot->main_view));
}

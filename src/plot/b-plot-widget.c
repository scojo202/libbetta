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
#include <math.h>
#include <cairo.h>
#include <cairo-svg.h>
#include <cairo-pdf.h>
#include "plot/b-plot-widget.h"
#include "plot/b-density-view.h"

/**
 * SECTION: b-plot-widget
 * @short_description: Widget showing a single plot surrounded by axes.
 *
 * This is a widget that shows a single plot surrounded by axes. Currently, this
 * the plot can be either an XY (scatter) plot or a density plot. The widget
 * also includes a toolbar with controls for zooming and translating (panning)
 * the region shown.
 *
 * #BPlotWidget also includes a mechanism for throttling the rate of updates in
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

struct _BPlotWidget
{
  GtkWidget base;
  BAxisView * north_axis;
  BAxisView * south_axis;
  BAxisView * west_axis;
  BAxisView * east_axis;

  BElementViewCartesian *main_view;
  double max_frame_rate;	// negative or zero if disabled
  guint frame_rate_timer;
  gboolean show_toolbar;
  GtkBox *toolbar;
  BLegend *legend;

  GtkLabel *pos_label;
};

/*static gboolean
thaw_timer (gpointer data)
{
  BPlotWidget *plot = B_PLOT_WIDGET (data);
  if (plot == NULL)
    return FALSE;

  if (plot->max_frame_rate <= 0)
    {
      b_plot_thaw_all (GTK_CONTAINER(plot));
      return FALSE;
    }

  b_plot_thaw_all (GTK_CONTAINER(plot));
  b_plot_freeze_all (GTK_CONTAINER(plot));

  return TRUE;
}*/

static void
b_plot_widget_finalize (GObject * obj)
{
  BPlotWidget *pw = (BPlotWidget *) obj;

  if (pw->frame_rate_timer)
    g_source_remove_by_user_data (pw);

  if (parent_class->finalize)
    parent_class->finalize (obj);
}

static void
b_plot_widget_dispose (GObject * obj)
{
  BPlotWidget *pw = (BPlotWidget *) obj;

  gtk_widget_unparent(GTK_WIDGET(pw->north_axis));
  gtk_widget_unparent(GTK_WIDGET(pw->west_axis));
  gtk_widget_unparent(GTK_WIDGET(pw->south_axis));
  gtk_widget_unparent(GTK_WIDGET(pw->east_axis));

  if(pw->main_view)
    gtk_widget_unparent(GTK_WIDGET(pw->main_view));

  if(pw->toolbar)
    gtk_widget_unparent(GTK_WIDGET(pw->toolbar));

  if(pw->legend)
    gtk_widget_unparent(GTK_WIDGET(pw->legend));

  if (parent_class->dispose)
    parent_class->dispose (obj);
}


#if 0
static void
b_plot_widget_set_property (GObject * object,
				  guint property_id,
				  const GValue * value, GParamSpec * pspec)
{
  BPlotWidget *plot = (BPlotWidget *) object;

  switch (property_id)
    {
    case PROP_FRAME_RATE:
      {
        plot->max_frame_rate = g_value_get_double (value);
        b_plot_freeze_all (GTK_CONTAINER(plot));
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
          //gtk_widget_set_no_show_all (GTK_WIDGET (plot->toolbar),
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
b_plot_widget_get_property (GObject * object,
				  guint property_id,
				  GValue * value, GParamSpec * pspec)
{
  BPlotWidget *self = (BPlotWidget *) object;
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
#endif

static void
b_plot_widget_class_init (BPlotWidgetClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  //object_class->set_property = b_plot_widget_set_property;
  //object_class->get_property = b_plot_widget_get_property;

  /* properties */

  /*g_object_class_install_property (object_class, PROP_FRAME_RATE,
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
							 G_PARAM_STATIC_STRINGS));*/

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = b_plot_widget_finalize;
  object_class->dispose = b_plot_widget_dispose;
}

void b_plot_set_background(GtkWidget *widget, const gchar *color_string)
{
  GtkStyleContext *stc;
  GtkCssProvider *cssp = gtk_css_provider_new ();
  gchar *css = g_strdup_printf ("grid {background-color:%s; }", color_string);
  gtk_css_provider_load_from_data (cssp, css, -1);
  stc = gtk_widget_get_style_context (widget);
  gtk_style_context_add_provider (stc, GTK_STYLE_PROVIDER (cssp),
                                  GTK_STYLE_PROVIDER_PRIORITY_THEME);
  g_free (css);
}

static void
b_plot_widget_init (BPlotWidget * obj)
{
  GtkWidget *w = GTK_WIDGET(obj);

  GtkLayoutManager *man = gtk_grid_layout_new();
  gtk_widget_set_layout_manager(w,man);

  b_plot_set_background(w, "#ffffff");

  obj->west_axis = b_axis_view_new (B_COMPASS_WEST);
  obj->south_axis = b_axis_view_new (B_COMPASS_SOUTH);
  obj->east_axis = b_axis_view_new (B_COMPASS_EAST);
  obj->north_axis = b_axis_view_new (B_COMPASS_NORTH);

  obj->main_view = NULL;

  gtk_widget_insert_before(GTK_WIDGET(obj->west_axis),w,NULL);
  GtkLayoutChild *west_child = gtk_layout_manager_get_layout_child(man,GTK_WIDGET(obj->west_axis));
  g_object_set(west_child,"column",0,"row",1,NULL);

  gtk_widget_insert_before(GTK_WIDGET(obj->north_axis),w,NULL);
  GtkLayoutChild *north_child = gtk_layout_manager_get_layout_child(man,GTK_WIDGET(obj->north_axis));
  g_object_set(north_child,"column",1,"row",0,NULL);

  gtk_widget_insert_before(GTK_WIDGET(obj->south_axis),w,NULL);
  GtkLayoutChild *south_child = gtk_layout_manager_get_layout_child(man,GTK_WIDGET(obj->south_axis));
  g_object_set(south_child,"column",1,"row",2,NULL);

  gtk_widget_insert_before(GTK_WIDGET(obj->east_axis),w,NULL);
  GtkLayoutChild *east_child = gtk_layout_manager_get_layout_child(man,GTK_WIDGET(obj->east_axis));
  g_object_set(east_child,"column",2,"row",1,NULL);

  g_object_set (obj, "vexpand", TRUE, "hexpand", TRUE,
                     "halign", GTK_ALIGN_FILL, "valign", GTK_ALIGN_FILL, NULL);

  b_plot_freeze_all (obj);

  g_object_set (obj->north_axis, "show-major-labels", FALSE, NULL);
  g_object_set (obj->east_axis, "show-major-labels", FALSE, NULL);


  /* create toolbar */
  obj->toolbar = b_plot_toolbar_new(obj);

  obj->pos_label = GTK_LABEL (gtk_label_new (""));
  gtk_box_append (obj->toolbar,
                     GTK_WIDGET (obj->pos_label));

  gtk_widget_insert_before(GTK_WIDGET(obj->toolbar),w,NULL);
  GtkLayoutChild *toolbar_child = gtk_layout_manager_get_layout_child(man,GTK_WIDGET(obj->toolbar));
  g_object_set(toolbar_child,"column",0,"row",3,"column-span",3,NULL);

  b_plot_thaw_all (obj);
}

G_DEFINE_TYPE (BPlotWidget, b_plot_widget, GTK_TYPE_WIDGET);

/**
 * b_plot_widget_add_view:
 * @obj: a #BPlotWidget
 * @view: a #BElementViewCartesian, such as a scatter view or density view
 *
 * Add a main view to a plot
 **/
void b_plot_widget_add_view(BPlotWidget *obj, BElementViewCartesian *view)
{
  GtkWidget *w = GTK_WIDGET(obj);
  GtkLayoutManager *man = gtk_widget_get_layout_manager(w);
  obj->main_view = B_ELEMENT_VIEW_CARTESIAN(view);

  gtk_widget_insert_before(GTK_WIDGET(obj->main_view),w,NULL);
  GtkLayoutChild *main_child = gtk_layout_manager_get_layout_child(man,GTK_WIDGET(obj->main_view));
  g_object_set(main_child,"column",1,"row",1,NULL);

  b_element_view_cartesian_connect_view_intervals (obj->main_view, Y_AXIS,
  					   B_ELEMENT_VIEW_CARTESIAN
  					   (obj->east_axis),
  					   META_AXIS);
  b_element_view_cartesian_connect_view_intervals (obj->main_view, Y_AXIS,
  					   B_ELEMENT_VIEW_CARTESIAN
  					   (obj->west_axis),
  					   META_AXIS);
  b_element_view_cartesian_connect_view_intervals (obj->main_view, X_AXIS,
  					   B_ELEMENT_VIEW_CARTESIAN
  					   (obj->north_axis),
  					   META_AXIS);
  b_element_view_cartesian_connect_view_intervals (obj->main_view, X_AXIS,
  					   B_ELEMENT_VIEW_CARTESIAN
  					   (obj->south_axis),
  					   META_AXIS);

  b_element_view_cartesian_set_axis_marker_type (B_ELEMENT_VIEW_CARTESIAN
  					 (obj->south_axis), META_AXIS,
  					 B_AXIS_SCALAR);
  b_element_view_cartesian_connect_axis_markers (B_ELEMENT_VIEW_CARTESIAN
  					 (obj->south_axis), META_AXIS,
  					 B_ELEMENT_VIEW_CARTESIAN(obj->north_axis), META_AXIS);

  b_element_view_cartesian_set_axis_marker_type (B_ELEMENT_VIEW_CARTESIAN
  					 (obj->west_axis), META_AXIS,
  					 B_AXIS_SCALAR);
  b_element_view_cartesian_connect_axis_markers (B_ELEMENT_VIEW_CARTESIAN
  					 (obj->west_axis), META_AXIS,
  					 B_ELEMENT_VIEW_CARTESIAN(obj->east_axis), META_AXIS);
}

/**
 * b_plot_widget_new_scatter:
 * @series: (nullable): a series to add to the plot, or %NULL
 *
 * Create a #BPlotWidget with a #BScatterLineView.
 *
 * Returns: the new plot
 **/
BPlotWidget * b_plot_widget_new_scatter(BScatterSeries *series)
{
  BPlotWidget *obj = g_object_new(B_TYPE_PLOT_WIDGET, NULL);
  BScatterLineView *view = g_object_new (B_TYPE_SCATTER_LINE_VIEW, NULL);

  b_plot_widget_add_view(obj,B_ELEMENT_VIEW_CARTESIAN(view));

  b_element_view_set_status_label (B_ELEMENT_VIEW(obj->main_view),
       obj->pos_label);

  if(B_IS_SCATTER_SERIES (series))
    b_scatter_line_view_add_series(view,series);

  return obj;
}

/**
 * b_plot_widget_new_density:
 *
 * Create a #BPlotWidget with a #BDensityView.
 *
 * Returns: the new plot
 **/
BPlotWidget * b_plot_widget_new_density(void)
{
  BPlotWidget *obj = g_object_new(B_TYPE_PLOT_WIDGET, NULL);
  BDensityView *view = g_object_new (B_TYPE_DENSITY_VIEW, NULL);

  b_plot_widget_add_view(obj,B_ELEMENT_VIEW_CARTESIAN(view));

  b_element_view_set_status_label (B_ELEMENT_VIEW(obj->main_view),
       obj->pos_label);

  return obj;
}

/**
 * b_plot_widget_get_main_view:
 * @plot: a #BPlotWidget
 *
 * Get the main view from the plot widget.
 *
 * Returns: (transfer none): the view
 **/
BElementViewCartesian * b_plot_widget_get_main_view(BPlotWidget *plot)
{
  return plot->main_view;
}

/**
 * b_plot_widget_get_axis_view:
 * @plot: a #BPlotWidget
 * @c: a compass direction
 *
 * Get one of the four axis views from the plot widget.
 *
 * Returns: (transfer none): the axis view for direction @c.
 **/
BAxisView *b_plot_widget_get_axis_view(BPlotWidget *plot, BCompass c)
{
  switch (c)
    {
    case B_COMPASS_EAST:
      {
        return plot->east_axis;
      }
      break;
    case B_COMPASS_WEST:
      {
        return plot->west_axis;
      }
      break;
    case B_COMPASS_NORTH:
      {
        return plot->north_axis;
      }
      break;
    case B_COMPASS_SOUTH:
      {
        return plot->south_axis;
      }
      break;
    default:
      {
        g_assert_not_reached ();
        return NULL;
      }
    }
}

/**
 * b_plot_widget_set_x_label:
 * @plot: a #BPlotWidget
 * @label: a label
 *
 * Set an axis label for the X (horizontal) axis.
 **/
void b_plot_widget_set_x_label(BPlotWidget *plot, const gchar *label)
{
  g_return_if_fail(B_IS_PLOT_WIDGET(plot));
  g_object_set(plot->south_axis,"axis_label",label, NULL);
}

/**
 * b_plot_widget_set_y_label:
 * @plot: a #BPlotWidget
 * @label: a label
 *
 * Set an axis label for the Y (vertical) axis.
 **/
void b_plot_widget_set_y_label(BPlotWidget *plot, const gchar *label)
{
  g_return_if_fail(B_IS_PLOT_WIDGET(plot));
  g_object_set(plot->west_axis,"axis_label",label, NULL);
}

/******************************/
/* utility functions for plot GUI */

static
void freeze_child(GtkWidget *widget)
{
  if(B_IS_ELEMENT_VIEW(widget)) {
    b_element_view_freeze (B_ELEMENT_VIEW (widget));
  }
}

/**
 * b_plot_freeze_all:
 * @c: a container with #BElementViews
 *
 * Freeze all #BElementView children of @c.
 **/
void
b_plot_freeze_all (BPlotWidget * c)
{
  g_return_if_fail(B_IS_PLOT_WIDGET(c));
  GtkWidget *w = (GtkWidget *) c;
  GtkWidget *child;
  for(child = gtk_widget_get_first_child(w); child != NULL; child=gtk_widget_get_next_sibling(child))
    freeze_child(child);
}

static
void thaw_child(GtkWidget *widget)
{
  if(B_IS_ELEMENT_VIEW(widget)) {
    b_element_view_thaw (B_ELEMENT_VIEW (widget));
  }
}

/**
 * b_plot_thaw_all:
 * @c: a container with #BElementViews
 *
 * Thaw all #BElementView children of @c.
 **/
void
b_plot_thaw_all (BPlotWidget * c)
{
  g_return_if_fail(B_IS_PLOT_WIDGET(c));
  GtkWidget *w = (GtkWidget *) c;
  GtkWidget *child;
  for(child = gtk_widget_get_first_child(w); child != NULL; child=gtk_widget_get_next_sibling(child))
    thaw_child(child);
}

/* following assumes that buttons are in a specific order */

static void
autoscale_child(GtkWidget *widget)
{
  if(B_IS_ELEMENT_VIEW_CARTESIAN(widget)) {
    BElementViewCartesian *cart = B_ELEMENT_VIEW_CARTESIAN(widget);
    BViewInterval *viy = b_element_view_cartesian_get_view_interval (cart,
                     Y_AXIS);
    BViewInterval *vix = b_element_view_cartesian_get_view_interval (cart,
                     X_AXIS);
    if(vix)
      b_view_interval_set_ignore_preferred_range (vix,FALSE);
    if(viy)
      b_view_interval_set_ignore_preferred_range (viy,FALSE);
  }
}

static void
autoscale_clicked (GtkButton *tool_button, gpointer user_data)
{
  GtkWidget *w = (GtkWidget *) user_data;
  GtkWidget *child;
  for(child = gtk_widget_get_first_child(w); child != NULL; child=gtk_widget_get_next_sibling(child))
    autoscale_child(child);
}

static void
set_zooming_child(GtkWidget *widget, gpointer data)
{
  if(B_IS_ELEMENT_VIEW(widget)) {
    b_element_view_set_zooming (B_ELEMENT_VIEW (widget),GPOINTER_TO_INT(data));
  }
}

static void
set_panning_child(GtkWidget *widget, gpointer data)
{
  if(B_IS_ELEMENT_VIEW(widget)) {
    b_element_view_set_panning (B_ELEMENT_VIEW (widget),GPOINTER_TO_INT(data));
  }
}

static void
pan_toggled (GtkToggleButton * toggle_button, gpointer user_data)
{
  GtkWidget *w = (GtkWidget *) user_data;
  gboolean active = gtk_toggle_button_get_active (toggle_button);
  GtkToggleButton *zoom_button = GTK_TOGGLE_BUTTON(gtk_widget_get_prev_sibling(GTK_WIDGET(toggle_button)));
  if (active && gtk_toggle_button_get_active (toggle_button))
    {
      gtk_toggle_button_set_active (zoom_button, FALSE);
    }
  GtkWidget *child;
  for(child = gtk_widget_get_first_child(w); child != NULL; child=gtk_widget_get_next_sibling(child))
    set_panning_child(child,GINT_TO_POINTER(active));
}

static void
zoom_toggled (GtkToggleButton * toggle_button, gpointer user_data)
{
  GtkWidget *w = (GtkWidget *) user_data;
  gboolean active = gtk_toggle_button_get_active (toggle_button);
  GtkToggleButton *pan_button = GTK_TOGGLE_BUTTON(gtk_widget_get_next_sibling(GTK_WIDGET(toggle_button)));
  if (active && gtk_toggle_button_get_active (pan_button))
    {
      gtk_toggle_button_set_active (pan_button, FALSE);
    }
  GtkWidget *child;
  for(child = gtk_widget_get_first_child(w); child != NULL; child=gtk_widget_get_next_sibling(child))
    set_zooming_child(child,GINT_TO_POINTER(active));
}
#if 0
static void
find_size_child(GtkWidget *widget, gpointer data)
{
  if(B_IS_ELEMENT_VIEW(widget)) {
    GdkRectangle a;
    gtk_widget_get_allocation(widget,&a);
    GdkRectangle **u = (GdkRectangle **) data;
    if (*u==NULL) {
      *u = g_new0(GdkRectangle,1);
      **u = a;
    }
    else {
      gdk_rectangle_union (&a, *u, *u);
    }
  }
}

typedef struct
{
  cairo_t *cr;
  int cx,cy; /* offset for container */
} SaveInfo;

static void
draw_child(GtkWidget *widget, gpointer data)
{
  SaveInfo *si = (SaveInfo *) data;
  cairo_t *cr = si->cr;

  cairo_save(cr);

  gint x,y;
  GdkWindow *w = gtk_widget_get_window(widget);
  if(GDK_IS_WINDOW(w)) {
    /* note: this only works when the container is alone in its window. Should
     * find the coordinate of the container and subtract it off */
    gdk_window_get_position (w,&x,&y);
    cairo_translate(cr,x-si->cx,y-si->cy);
  }

  if(B_IS_ELEMENT_VIEW(widget)) {
    GtkWidgetClass *wc = (GtkWidgetClass *) G_OBJECT_GET_CLASS(widget);
    wc->draw(widget,cr);
  }

  cairo_restore(cr);
}

/**
 * b_plot_save:
 * @c: a container with #BElementViews
 * @path: a file path
 * @error: (nullable): a #GError, not currently used
 *
 * Save an image of the plot to a file. The format is determined by the file
 * extension: PDF if the basename ends in ".pdf", SVG if it ends in ".svg", and
 * PNG if the basename ends in ".png" or if there is no extension. Only
 * #BElementViews are drawn, and the size of the image is just large enough to
 * fit the #BElementViews (which may be smaller than @c's allocated size).
 *
 * Returns: %TRUE if save was successful
 **/
gboolean b_plot_save(GtkContainer *c, gchar *path, GError *error)
{
  cairo_t * context;
  cairo_surface_t * surface = NULL;

  g_return_val_if_fail(GTK_IS_CONTAINER(c),FALSE);
  if(!gtk_widget_get_realized(GTK_WIDGET(c))) {
     g_warning("saving an unrealized plot not implemented");
     return FALSE;
  }

  /* get type from extension */
  gchar *ext;
  gchar *basename = g_path_get_basename(path);
  gchar **parts = g_strsplit(basename,".",-1);
  g_free(basename);
  if(parts) {
    guint len = g_strv_length(parts);
    ext = g_strdup(parts[len-1]);
    g_strfreev (parts);
  }
  else {
    ext = g_strdup("png");
  }

  //GtkAllocation a;
  //gtk_widget_get_allocation(GTK_WIDGET(c),&a);
  //g_message("allocation %d %d %d %d",a.x,a.y,a.width,a.height);

  /* find the size required to fit all views */

  GdkRectangle *u = NULL;
  gtk_container_foreach(c,find_size_child,&u);

  if(u==NULL)
    return FALSE;

  if(u->width == 0 || u->height == 0) {
    return FALSE;
  }

  //g_message("union %d %d %d %d",u->x,u->y,u->width,u->height);

  if (!g_ascii_strncasecmp(ext,"png",3)) {
    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, u->width, u->height);
    context = cairo_create (surface);
  }
  else if (!g_ascii_strncasecmp(ext,"svg",3)) {
    cairo_surface_t * surface = cairo_svg_surface_create (path, u->width, u->height);
    context = cairo_create (surface);
  }
  else if (!g_ascii_strncasecmp(ext,"pdf",3)) {
    cairo_surface_t * surface = cairo_pdf_surface_create (path, u->width, u->height);
    context = cairo_create (surface);
  }
  else {
    g_warning("file extension %s not supported",ext); /* TODO: set error, free stuff */
    return FALSE;
  }

  /* draw to surface */

  /* paint a background */
  cairo_rectangle(context,0.0,0.0,u->width,u->height);

  cairo_save(context);
  cairo_set_source_rgb (context, 1.0, 1.0, 1.0);
  cairo_fill(context);
  cairo_restore(context);

  SaveInfo *si = g_new(SaveInfo,1);
  si->cx = u->x;
  si->cy = u->y;
  si->cr = context;

  g_clear_pointer(&u,g_free);

  gtk_container_foreach(c,draw_child,si);

  g_free(si);

  gboolean succ = TRUE;

  if (!g_ascii_strncasecmp(ext,"png",3)) {
    cairo_status_t ret = cairo_surface_write_to_png (surface, path);
    if(ret==CAIRO_STATUS_SUCCESS) {
      succ = FALSE;
    }
  }
  else if (!g_ascii_strncasecmp(ext,"pdf",3)) {
    cairo_show_page (context);
  }

  surface = cairo_get_target (context);
  cairo_surface_finish (surface);
  cairo_surface_destroy (surface);
  cairo_destroy (context);

  return succ;
}

static void
save_clicked (GtkToolButton *tool_button, gpointer user_data)
{
  GtkContainer *c = (GtkContainer *) user_data;
  GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET(tool_button));
  GtkWindow *win = NULL;
  if(GTK_IS_WINDOW(toplevel))
    win = GTK_WINDOW(toplevel);

  GtkFileChooserNative *n =
    gtk_file_chooser_native_new ("Save to file",
                             win,
                             GTK_FILE_CHOOSER_ACTION_SAVE,
                             "Save",
                             "Cancel");

  GtkFileChooser *chooser = GTK_FILE_CHOOSER (n);

  gtk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);

  gtk_file_chooser_set_current_name (chooser,
                                     "Untitled.png");

  gint res = gtk_native_dialog_run (GTK_NATIVE_DIALOG (n));
  if (res == GTK_RESPONSE_ACCEPT)
  {
    char *filename;

    filename = gtk_file_chooser_get_filename (chooser);
    b_plot_save (c, filename, NULL);
    g_free (filename);
  }

  g_object_unref (n);
}
#endif

/**
 * b_plot_toolbar_new:
 * @g: a #BPlotWidget
 *
 * Create a new toolbar for a plot or collection of plots. The items in the
 * toolbar will operate on all #BElementViews that are children of @c.
 *
 * Returns: (transfer full): The new #GtkBox.
 **/
GtkBox *b_plot_toolbar_new (BPlotWidget *g)
{
  //static gboolean initted = FALSE;

  /*if(!initted) {
    gtk_icon_theme_prepend_search_path (gtk_icon_theme_get_default (), PACKAGE_ICONDIR);
    initted = TRUE;
  }*/

  GtkBox *toolbar = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));

  GtkWidget *autoscale_button =
    gtk_button_new_with_label ("Autoscale");
  /*gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (autoscale_button),
                                 "zoom-fit-best-symbolic");
  gtk_widget_set_tooltip_text(GTK_WIDGET(autoscale_button),"Autoscale");*/
  gtk_box_append (toolbar, autoscale_button);
  g_signal_connect (autoscale_button, "clicked",
                    G_CALLBACK (autoscale_clicked), g);

  GtkWidget *zoom_button =
    gtk_toggle_button_new_with_label ("Zoom");
  /*gtk_tool_button_set_label (GTK_TOOL_BUTTON (zoom_button),
			     "Zoom");
  gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (zoom_button),
				 "edit-find-symbolic");
  gtk_widget_set_tooltip_text(GTK_WIDGET(zoom_button),"Zoom");*/
  gtk_box_append (toolbar, zoom_button);
  g_signal_connect (zoom_button, "toggled", G_CALLBACK (zoom_toggled), g);

  GtkWidget *pan_button =
    gtk_toggle_button_new_with_label ("Pan");
  /*gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(pan_button),"pointer-mode-drag-symbolic");
  gtk_widget_set_tooltip_text(GTK_WIDGET(pan_button),"Pan");*/
  gtk_box_append (toolbar, pan_button);
  g_signal_connect (pan_button, "toggled", G_CALLBACK (pan_toggled), g);

  /*GtkToolButton *save_button =
    GTK_TOOL_BUTTON (gtk_tool_button_new (NULL,"Save"));
  gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(save_button),"document-save-symbolic");
  gtk_widget_set_tooltip_text(GTK_WIDGET(save_button),"Save");
  gtk_toolbar_insert (toolbar,
		      GTK_TOOL_ITEM (save_button), -1);
  g_signal_connect (save_button, "clicked",
		    G_CALLBACK (save_clicked), c);*/

  return toolbar;
}

void b_plot_widget_attach_legend(BPlotWidget *plot, BLegend *leg)
{
  gtk_widget_insert_before(GTK_WIDGET(leg),GTK_WIDGET(plot),NULL);
  GtkLayoutManager *man = gtk_widget_get_layout_manager(GTK_WIDGET(plot));
  GtkLayoutChild *legend_child = gtk_layout_manager_get_layout_child(man,GTK_WIDGET(leg));
  g_object_set(legend_child,"column",0,"row",4,"column-span",3,NULL);
  plot->legend = leg;
}

/*
 * y-element-view-cartesian.c
 *
 * Copyright (C) 2000 EMC Capital Management, Inc.
 * Copyright (C) 2001-2002 The Free Software Foundation
 * Copyright (C) 2016 Scott O. Johnson (scojo202@gmail.com)
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

#include "y-element-view-cartesian.h"

#include <math.h>
#include <string.h>

/**
 * SECTION: y-element-view-cartesian
 * @short_description: Base class for plot objects with Cartesian coordinates.
 *
 * Abstract base class for plot classes #YScatterView, #YAxisView, and others.
 *
 */

typedef struct _ViewAxisPair ViewAxisPair;
struct _ViewAxisPair {
  YElementViewCartesian *cart;
  axis_t axis;

  /* We cache the class of the cartesian view */
  YElementViewCartesianClass *klass;
};

typedef struct {
  YViewInterval *y_view_interval[LAST_AXIS];
  guint vi_changed_handler[LAST_AXIS];
  guint vi_prefrange_handler[LAST_AXIS];
  gboolean vi_force_preferred[LAST_AXIS];
  ViewAxisPair *vi_closure[LAST_AXIS];

  gboolean forcing_preferred;
  guint pending_force_tag;

  gint axis_marker_type[LAST_AXIS];
  YAxisMarkers *axis_markers[LAST_AXIS];
  guint am_changed_handler[LAST_AXIS];
} YElementViewCartesianPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (YElementViewCartesian, y_element_view_cartesian, Y_TYPE_ELEMENT_VIEW);

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
update_axis_markers (YElementViewCartesian *cart,
		     axis_t               ax,
		     YAxisMarkers          *marks,
		     double                     range_min,
		     double                     range_max)
{
  g_assert (0 <= ax && ax < LAST_AXIS);
  
  YElementViewCartesianPrivate *priv = y_element_view_cartesian_get_instance_private(cart);
    
  g_debug("update_axis_markers: %d, %p",priv->axis_marker_type[ax], marks);

  if (marks && priv->axis_marker_type[ax] != Y_AXIS_NONE) {
    g_debug("really update_axis_markers");
    y_axis_markers_populate_generic (marks,
					 priv->axis_marker_type[ax],
					 range_min, range_max);
  }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
y_element_view_cartesian_finalize (GObject *obj)
{
  YElementViewCartesian *cart;
  YElementViewCartesianPrivate *p;
  gint i;

  cart = Y_ELEMENT_VIEW_CARTESIAN (obj);
  p = y_element_view_cartesian_get_instance_private(cart);

  /* clean up the view intervals */

  for (i=0; i<LAST_AXIS; ++i) {
    if (p->y_view_interval[i]) {
      if (p->vi_changed_handler[i])
	    g_signal_handler_disconnect (p->y_view_interval[i],
				     p->vi_changed_handler[i]);
      if (p->vi_prefrange_handler[i])
	    g_signal_handler_disconnect (p->y_view_interval[i],
				     p->vi_prefrange_handler[i]);

      g_clear_object(&p->y_view_interval[i]);
    }
    g_slice_free (ViewAxisPair,p->vi_closure[i]);
  }

  if (p->pending_force_tag)
    g_source_remove (p->pending_force_tag);
  
  /* clean up the axis markers */

  for (i = 0; i < LAST_AXIS; ++i) {
    if (p->axis_markers[i]) {
      if (p->am_changed_handler[i])
	    g_signal_handler_disconnect (p->axis_markers[i],
				     p->am_changed_handler[i]);

      g_clear_object(&p->axis_markers[i]);
    }
  }

  GObjectClass *obj_class = G_OBJECT_CLASS(y_element_view_cartesian_parent_class);
    
  if (obj_class->finalize)
    obj_class->finalize (obj);
}

static void
y_element_view_cartesian_class_init (YElementViewCartesianClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->finalize = y_element_view_cartesian_finalize;
  klass->update_axis_markers = update_axis_markers;
}


static void
y_element_view_cartesian_init (YElementViewCartesian *cart)
{
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
compute_markers (YElementViewCartesian *cart,
		 axis_t               ax)
{
  YElementViewCartesianClass *klass;
  double a=0, b=0;
  
  g_debug("computing markers");

  g_assert (0 <= ax && ax < LAST_AXIS);
    
  YElementViewCartesianPrivate *priv = y_element_view_cartesian_get_instance_private(cart);

  klass = Y_ELEMENT_VIEW_CARTESIAN_CLASS (G_OBJECT_GET_CLASS (cart));

  if (priv->axis_markers[ax] != NULL && klass->update_axis_markers != NULL) {
    g_debug("really computing markers");
    YViewInterval *vi = y_element_view_cartesian_get_view_interval (cart, ax);
    YAxisMarkers *am = priv->axis_markers[ax];

    if (vi && am) {
      g_debug("really really computing markers");
      y_view_interval_range (vi, &a, &b);
      klass->update_axis_markers (cart, ax, am, a, b);
    }
  }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

/* 
 *
 * YYViewInterval gadgetry
 *
 */

static gboolean
force_all_preferred_idle (gpointer ptr)
{
  YElementViewCartesian *cart = ptr;
  gint i;
    
  YElementViewCartesianPrivate *priv = y_element_view_cartesian_get_instance_private(cart);

  for (i = 0; i < LAST_AXIS; ++i) {
    if (priv->y_view_interval[i] && priv->vi_force_preferred[i])
      y_element_view_cartesian_set_preferred_view (cart, i);
  }
  priv->pending_force_tag = 0;

  return FALSE;
}

static void
vi_changed (YViewInterval *vi,
	    ViewAxisPair      *pair)
{
  YElementViewCartesian *cart = pair->cart;
  axis_t ax = pair->axis;

  YElementViewCartesianPrivate *p = y_element_view_cartesian_get_instance_private(cart);

  y_element_view_freeze ((YElementView *) cart);

  if (p->vi_force_preferred[ax]) {
    if (p->vi_changed_handler[ax])
      g_signal_handler_block (vi, p->vi_changed_handler[ax]);

    y_element_view_cartesian_set_preferred_view (cart, ax);
    
    if (p->vi_changed_handler[ax])
      g_signal_handler_unblock (vi, p->vi_changed_handler[ax]);
    } else if (p->forcing_preferred && p->pending_force_tag == 0) {
    
    p->pending_force_tag = g_idle_add (force_all_preferred_idle, cart);
  }
  
  g_debug("vi_changed");
  
  if (p->axis_markers[ax] != NULL)
    compute_markers (cart, ax);

  y_element_view_changed (Y_ELEMENT_VIEW (cart));

  y_element_view_thaw ((YElementView *) cart);
}

static void
vi_preferred (YViewInterval *vi, ViewAxisPair *pair)
{
  YElementViewCartesian *cart = pair->cart;
  axis_t ax = pair->axis;
  double a, b;
  
  if (pair->klass == NULL) {
    pair->klass = Y_ELEMENT_VIEW_CARTESIAN_CLASS (G_OBJECT_GET_CLASS (cart));
    g_assert (pair->klass);
  }

  /* Our 'changed' signals are automatically blocked during view
     interval range requests, so we don't need to worry about
     freeze/thaw. */

  if (pair->klass->preferred_range
      && pair->klass->preferred_range (cart, ax, &a, &b)) {
      y_view_interval_grow_to (vi, a, b);
  }
}

static void
set_y_view_interval (YElementViewCartesian *cart,
		   axis_t               ax,
		   YViewInterval         *vi)
{
  YElementViewCartesianPrivate *p = y_element_view_cartesian_get_instance_private(cart);
  gint i = (int) ax;

  g_assert (0 <= i && i < LAST_AXIS);
  
  if (p->y_view_interval[i] == vi)
    return;
  
  if (p->y_view_interval[i] && p->vi_changed_handler[i]) {
    g_signal_handler_disconnect (p->y_view_interval[i], p->vi_changed_handler[i]);
    p->vi_changed_handler[i] = 0;
  }

  if (p->vi_prefrange_handler[i]) {
    g_signal_handler_disconnect (p->y_view_interval[i], p->vi_prefrange_handler[i]);
    p->vi_prefrange_handler[i] = 0;
  }

  g_set_object(&p->y_view_interval[i], vi);

  if (vi != NULL) {
    
    if (p->vi_closure[i] == NULL) {
      p->vi_closure[i] = g_slice_new0 (ViewAxisPair);
      p->vi_closure[i]->cart = cart;
      p->vi_closure[i]->axis = ax;
    }

    p->vi_changed_handler[i] = g_signal_connect (p->y_view_interval[i],
						 "changed",
						 (GCallback) vi_changed,
						 p->vi_closure[i]);
    
    p->vi_prefrange_handler[i] = g_signal_connect (p->y_view_interval[i],
						   "preferred_range_request",
						   (GCallback) vi_preferred,
						   p->vi_closure[i]);

    g_debug("set_y_view_interval");

    compute_markers (cart, ax);
  }
  else {
    g_message("failed to set y_view_interval");
  }
}

/*** YViewInterval-related API calls ***/

void
y_element_view_cartesian_add_view_interval (YElementViewCartesian *cart,
						axis_t               ax)
{
  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart));
  g_assert (0 <= ax && ax < LAST_AXIS);
    
  YElementViewCartesianPrivate *priv = y_element_view_cartesian_get_instance_private(cart);

  if (priv->y_view_interval[ax] == NULL) {
    YViewInterval *vi = y_view_interval_new ();
    //g_message("about to set view interval");
    set_y_view_interval (cart, ax, vi);
    y_view_interval_request_preferred_range (vi);
    g_object_unref (G_OBJECT(vi));
  }
  else {
    g_message("failed to add");
  }
}

YViewInterval *
y_element_view_cartesian_get_view_interval (YElementViewCartesian *cart,
						axis_t               ax)
{
  g_return_val_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart), NULL);
  g_assert (0 <= ax && ax < LAST_AXIS);
    
  YElementViewCartesianPrivate *priv = y_element_view_cartesian_get_instance_private(cart);

  return priv->y_view_interval[ax];
}

void
y_element_view_cartesian_connect_view_intervals (YElementViewCartesian *cart1,
						     axis_t               axis1,
						     YElementViewCartesian *cart2,
						     axis_t               axis2)
{
  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart1));
  g_return_if_fail (0 <= axis1 && axis1 < LAST_AXIS);
  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart2));
  g_return_if_fail (0 <= axis2 && axis2 < LAST_AXIS);

  if (axis1 == axis2 && cart1 == cart2)
    return;
  //g_message("connect view intervals");
  set_y_view_interval (cart2, axis2,
		     y_element_view_cartesian_get_view_interval (cart1, axis1));

  y_element_view_changed (Y_ELEMENT_VIEW (cart2));
}

/***************************************************************************/

void
y_element_view_cartesian_set_preferred_view (YElementViewCartesian *cart,
						 axis_t               ax)
{
  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart));
  g_assert (0 <= ax && ax < LAST_AXIS);
    
  YElementViewCartesianPrivate *priv = y_element_view_cartesian_get_instance_private(cart);

  if (priv->y_view_interval[ax] != NULL)
    y_view_interval_request_preferred_range (priv->y_view_interval[ax]);
}

void
y_element_view_cartesian_set_preferred_view_all (YElementViewCartesian *cart)
{
  gint i;
  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart));
    
  YElementViewCartesianPrivate *priv = y_element_view_cartesian_get_instance_private(cart);

  /* Grossly inefficient, and wrong */

  for (i = 0; i < LAST_AXIS; ++i) {
    if (priv->y_view_interval[i] != NULL)
      y_element_view_cartesian_set_preferred_view (cart, (axis_t) i);
  }
}

void
y_element_view_cartesian_force_preferred_view (YElementViewCartesian *cart,
						   axis_t               ax,
						   gboolean                   force)
{
  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart));
  g_assert (0 <= ax && ax < LAST_AXIS);
    
  YElementViewCartesianPrivate *priv = y_element_view_cartesian_get_instance_private(cart);

  priv->vi_force_preferred[ax] = force;

  if (force)
    y_element_view_cartesian_set_preferred_view (cart, ax);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

/*
 *
 * YAxisMarkers gadgetry
 *
 */

static void
am_changed (YAxisMarkers *gam,
	    ViewAxisPair     *pair)
{
  YElementView *view = Y_ELEMENT_VIEW (pair->cart);

  y_element_view_changed (view);
}

static void
set_axis_markers (YElementViewCartesian *cart,
		  axis_t               ax,
		  YAxisMarkers          *mark)
{
  YElementViewCartesianPrivate *p;

  g_assert (0 <= ax && ax < LAST_AXIS);

  p = y_element_view_cartesian_get_instance_private(cart);

  if (p->axis_markers[ax] != NULL) {
    g_signal_handler_disconnect (p->axis_markers[ax], p->am_changed_handler[ax]);
    p->am_changed_handler[ax] = 0;
  }

  g_set_object(&p->axis_markers[ax], mark);

  if (mark != NULL) {

    if (p->vi_closure[ax] == NULL) {
      p->vi_closure[ax] = g_slice_new0 (ViewAxisPair);
      p->vi_closure[ax]->cart = cart;
      p->vi_closure[ax]->axis = ax;
    }

    p->am_changed_handler[ax] = g_signal_connect (p->axis_markers[ax],
						  "changed",
						  (GCallback) am_changed,
						  p->vi_closure[ax]);
  }
}

/*** AxisMarker-related API calls */

void
y_element_view_cartesian_add_axis_markers (YElementViewCartesian *cart,
					       axis_t               ax)
{
  g_debug("add_axis_markers");
  
  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart));
  g_assert (0 <= ax && ax < LAST_AXIS);
    
  YElementViewCartesianPrivate *priv = y_element_view_cartesian_get_instance_private(cart);
 
  if (priv->axis_markers[ax] == NULL) {
    YAxisMarkers *am = y_axis_markers_new ();
    set_axis_markers (cart, ax, am);
    g_object_unref (G_OBJECT(am));
  }
}

gint
y_element_view_cartesian_get_axis_marker_type (YElementViewCartesian *cart,
						   axis_t               ax)
{
  g_return_val_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart), -1);
  g_assert (0 <= ax && ax < LAST_AXIS);
    
  YElementViewCartesianPrivate *priv = y_element_view_cartesian_get_instance_private(cart);

  return priv->axis_marker_type[ax];
}

void
y_element_view_cartesian_set_axis_marker_type (YElementViewCartesian *cart,
						   axis_t               ax,
						   gint                       code)
{
  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart));
  g_assert (0 <= ax && ax < LAST_AXIS);
    
  YElementViewCartesianPrivate *priv = y_element_view_cartesian_get_instance_private(cart);
  
  priv->axis_marker_type[ax] = code;
  y_element_view_cartesian_add_axis_markers (cart, ax);
  compute_markers (cart, ax);
}

YAxisMarkers *
y_element_view_cartesian_get_axis_markers (YElementViewCartesian *cart,
					       axis_t               ax)
{
  YAxisMarkers *gam;
  
  g_return_val_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart), NULL);
  g_assert (0 <= ax && ax < LAST_AXIS);
    
  YElementViewCartesianPrivate *priv = y_element_view_cartesian_get_instance_private(cart);

  gam = priv->axis_markers[ax];
  if (gam)
    y_axis_markers_sort (gam);
  
  return gam;
}

void
y_element_view_cartesian_connect_axis_markers (YElementViewCartesian *cart1,
						   axis_t               ax1,
						   YElementViewCartesian *cart2,
						   axis_t               ax2)
{
  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart1));
  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart2));
  g_assert (0 <= ax1 && ax1 < LAST_AXIS);
  g_assert (0 <= ax2 && ax2 < LAST_AXIS);

  y_element_view_freeze ((YElementView *) cart2);

  set_axis_markers (cart2, ax2,
		    y_element_view_cartesian_get_axis_markers (cart1, ax1));

  y_element_view_cartesian_connect_view_intervals (cart1, ax1, cart2, ax2);

  y_element_view_changed ((YElementView *) cart2);
  y_element_view_thaw ((YElementView *) cart2);
}

static void
autoscale_toggled (GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
  YViewInterval *vi = Y_VIEW_INTERVAL(user_data);
  y_view_interval_set_ignore_preferred_range(vi,!gtk_check_menu_item_get_active(checkmenuitem));
}

GtkWidget * create_autoscale_menu_check_item (const gchar *label, YElementViewCartesian *view, axis_t ax)
{
  GtkWidget *autoscale_x = gtk_check_menu_item_new_with_label(label);
  YViewInterval *vix = y_element_view_cartesian_get_view_interval (view,
						       ax);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(autoscale_x),!vix->ignore_preferred);
  g_signal_connect (autoscale_x, "toggled",
                    G_CALLBACK (autoscale_toggled), vix);
  return autoscale_x;
}

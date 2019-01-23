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

#include "plot/y-element-view-cartesian.h"

#include <math.h>
#include <string.h>

/**
 * SECTION: y-element-view-cartesian
 * @short_description: Base class for plot objects with Cartesian coordinates.
 *
 * Abstract base class for plot classes #YScatterLineView, #YAxisView, and others.
 *
 */

typedef struct _ViewAxisPair ViewAxisPair;
struct _ViewAxisPair
{
  YElementViewCartesian *cart;
  YAxisType axis;

  /* We cache the class of the cartesian view */
  YElementViewCartesianClass *klass;
};

typedef struct
{
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

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (YElementViewCartesian,
				     y_element_view_cartesian,
				     Y_TYPE_ELEMENT_VIEW);

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
update_axis_markers (YElementViewCartesian * cart,
		     YAxisType ax,
		     YAxisMarkers * marks, double range_min, double range_max)
{
  g_assert (0 <= ax && ax < LAST_AXIS);

  YElementViewCartesianPrivate *priv =
    y_element_view_cartesian_get_instance_private (cart);

  g_debug ("update_axis_markers: %d, %p", priv->axis_marker_type[ax], marks);

  if (marks && priv->axis_marker_type[ax] != Y_AXIS_NONE)
    {
      g_debug ("really update_axis_markers");
      y_axis_markers_populate_generic (marks,
				       priv->axis_marker_type[ax],
				       range_min, range_max);
    }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
y_element_view_cartesian_finalize (GObject * obj)
{
  YElementViewCartesian *cart;
  YElementViewCartesianPrivate *p;
  gint i;

  cart = Y_ELEMENT_VIEW_CARTESIAN (obj);
  p = y_element_view_cartesian_get_instance_private (cart);

  /* clean up the view intervals */

  for (i = 0; i < LAST_AXIS; ++i)
    {
      if (p->y_view_interval[i])
      {
        if (p->vi_changed_handler[i])
          g_signal_handler_disconnect (p->y_view_interval[i],
                                       p->vi_changed_handler[i]);
        if (p->vi_prefrange_handler[i])
          g_signal_handler_disconnect (p->y_view_interval[i],
                                       p->vi_prefrange_handler[i]);

        g_clear_object (&p->y_view_interval[i]);
      }
      g_slice_free (ViewAxisPair, p->vi_closure[i]);
    }

  if (p->pending_force_tag)
    g_source_remove (p->pending_force_tag);

  /* clean up the axis markers */

  for (i = 0; i < LAST_AXIS; ++i)
    {
      if (p->axis_markers[i])
      {
        if (p->am_changed_handler[i])
          g_signal_handler_disconnect (p->axis_markers[i],
                                       p->am_changed_handler[i]);

          g_clear_object (&p->axis_markers[i]);
      }
    }

  GObjectClass *obj_class =
    G_OBJECT_CLASS (y_element_view_cartesian_parent_class);

  if (obj_class->finalize)
    obj_class->finalize (obj);
}

static void
y_element_view_cartesian_class_init (YElementViewCartesianClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->finalize = y_element_view_cartesian_finalize;
  klass->update_axis_markers = update_axis_markers;
}


static void
y_element_view_cartesian_init (YElementViewCartesian * cart)
{
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
compute_markers (YElementViewCartesian * cart, YAxisType ax)
{
  YElementViewCartesianClass *klass;
  double a = 0, b = 0;

  g_debug ("computing markers");

  g_assert (0 <= ax && ax < LAST_AXIS);

  YElementViewCartesianPrivate *priv =
    y_element_view_cartesian_get_instance_private (cart);

  klass = Y_ELEMENT_VIEW_CARTESIAN_CLASS (G_OBJECT_GET_CLASS (cart));

  if (priv->axis_markers[ax] != NULL && klass->update_axis_markers != NULL)
    {
      g_debug ("really computing markers");
      YViewInterval *vi =
        y_element_view_cartesian_get_view_interval (cart, ax);
      YAxisMarkers *am = priv->axis_markers[ax];

      if (vi && am)
      {
        g_debug ("really really computing markers");
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

  YElementViewCartesianPrivate *priv =
    y_element_view_cartesian_get_instance_private (cart);

  for (i = 0; i < LAST_AXIS; ++i)
    {
      if (priv->y_view_interval[i] && priv->vi_force_preferred[i])
        y_element_view_cartesian_set_preferred_view (cart, i);
    }
  priv->pending_force_tag = 0;

  return FALSE;
}

static void
vi_changed (YViewInterval * vi, ViewAxisPair * pair)
{
  YElementViewCartesian *cart = pair->cart;
  YAxisType ax = pair->axis;

  YElementViewCartesianPrivate *p =
    y_element_view_cartesian_get_instance_private (cart);

  y_element_view_freeze ((YElementView *) cart);

  if (p->vi_force_preferred[ax])
    {
      if (p->vi_changed_handler[ax])
        g_signal_handler_block (vi, p->vi_changed_handler[ax]);

      y_element_view_cartesian_set_preferred_view (cart, ax);

      if (p->vi_changed_handler[ax])
        g_signal_handler_unblock (vi, p->vi_changed_handler[ax]);
    }
  else if (p->forcing_preferred && p->pending_force_tag == 0)
    {

      p->pending_force_tag = g_idle_add (force_all_preferred_idle, cart);
    }

  g_debug ("vi_changed");

  if (p->axis_markers[ax] != NULL)
    compute_markers (cart, ax);

  y_element_view_changed (Y_ELEMENT_VIEW (cart));

  y_element_view_thaw ((YElementView *) cart);
}

static void
vi_preferred (YViewInterval * vi, ViewAxisPair * pair)
{
  YElementViewCartesian *cart = pair->cart;
  YAxisType ax = pair->axis;
  double a, b;

  if (pair->klass == NULL)
    {
      pair->klass =
	Y_ELEMENT_VIEW_CARTESIAN_CLASS (G_OBJECT_GET_CLASS (cart));
      g_assert (pair->klass);
    }

  /* Our 'changed' signals are automatically blocked during view
     interval range requests, so we don't need to worry about
     freeze/thaw. */

  if (pair->klass->preferred_range
      && pair->klass->preferred_range (cart, ax, &a, &b))
    {
      y_view_interval_grow_to (vi, a, b);
    }
}

static void
set_view_interval (YElementViewCartesian * cart,
                     YAxisType ax, YViewInterval * vi)
{
  YElementViewCartesianPrivate *p =
    y_element_view_cartesian_get_instance_private (cart);
  gint i = (int) ax;

  g_assert (0 <= i && i < LAST_AXIS);

  if (p->y_view_interval[i] == vi)
    return;

  if (p->y_view_interval[i] && p->vi_changed_handler[i])
    {
      g_signal_handler_disconnect (p->y_view_interval[i],
				   p->vi_changed_handler[i]);
      p->vi_changed_handler[i] = 0;
    }

  if (p->vi_prefrange_handler[i])
    {
      g_signal_handler_disconnect (p->y_view_interval[i],
				   p->vi_prefrange_handler[i]);
      p->vi_prefrange_handler[i] = 0;
    }

  g_set_object (&p->y_view_interval[i], vi);

  if (vi != NULL)
    {

      if (p->vi_closure[i] == NULL)
        {
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

      g_debug ("set_view_interval");

      compute_markers (cart, ax);
    }
  else
    {
      g_warning ("failed to set y_view_interval, %p %d",cart,ax);
    }
}

/*** YViewInterval-related API calls ***/

/**
 * y_element_view_cartesian_add_view_interval :
 * @cart: #YElementViewCartesian
 * @ax: the axis
 *
 * Add a #YViewInterval to @cart for axis @ax. The view interval will
 * immediately emit a "preferred_range_request" signal.
 **/
void
y_element_view_cartesian_add_view_interval (YElementViewCartesian * cart,
					    YAxisType ax)
{
  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart));
  g_assert (0 <= ax && ax < LAST_AXIS);

  YElementViewCartesianPrivate *priv =
    y_element_view_cartesian_get_instance_private (cart);

  if (priv->y_view_interval[ax] == NULL)
    {
      YViewInterval *vi = y_view_interval_new ();
      set_view_interval (cart, ax, vi);
      y_view_interval_request_preferred_range (vi);
      g_object_unref (G_OBJECT (vi));
    }
  else
    {
      g_warning ("y_element_view_cartesian_add_view_interval: vi already present");
    }
}

/**
 * y_element_view_cartesian_get_view_interval :
 * @cart: #YElementViewCartesian
 * @ax: the axis
 *
 * Get the #YViewInterval object associated with axis @ax.
 *
 * Returns: (transfer none): the #YViewInterval
 **/
YViewInterval *
y_element_view_cartesian_get_view_interval (YElementViewCartesian * cart,
					    YAxisType ax)
{
  g_return_val_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart), NULL);
  g_assert (0 <= ax && ax < LAST_AXIS);

  YElementViewCartesianPrivate *priv =
    y_element_view_cartesian_get_instance_private (cart);

  return priv->y_view_interval[ax];
}

/**
 * y_element_view_cartesian_connect_view_intervals :
 * @cart1: a #YElementViewCartesian
 * @axis1: an axis
 * @cart2: a #YElementViewCartesian
 * @axis2: an axis
 *
 * Bind view intervals for two #YElementViewCartesian's. The #YViewInterval for
 * axis @axis2 of @cart2 is set as the #YViewInterval for axis @axis1 of @cart1.
 * This is used, for example, to connect an axis of a scatter view to an axis
 * view.
 **/
void
y_element_view_cartesian_connect_view_intervals (YElementViewCartesian *cart1,
                                                 YAxisType axis1,
                                                 YElementViewCartesian *cart2,
                                                 YAxisType axis2)
{
  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart1));
  g_return_if_fail (0 <= axis1 && axis1 < LAST_AXIS);
  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart2));
  g_return_if_fail (0 <= axis2 && axis2 < LAST_AXIS);

  if (axis1 == axis2 && cart1 == cart2)
    return;
  set_view_interval(cart2, axis2,
                      y_element_view_cartesian_get_view_interval (cart1,axis1));

  y_element_view_changed (Y_ELEMENT_VIEW (cart2));
}

/***************************************************************************/

/**
 * y_element_view_cartesian_set_preferred_view :
 * @cart: #YElementViewCartesian
 * @axis: the axis
 *
 * Have the view intervals for @ax emit a "preferred_range_request" signal.
 **/
void
y_element_view_cartesian_set_preferred_view (YElementViewCartesian * cart,
					     YAxisType axis)
{
  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart));
  g_assert (0 <= axis && axis < LAST_AXIS);

  YElementViewCartesianPrivate *priv =
    y_element_view_cartesian_get_instance_private (cart);

  if (priv->y_view_interval[axis] != NULL)
    y_view_interval_request_preferred_range (priv->y_view_interval[axis]);
}

/**
 * y_element_view_cartesian_set_preferred_view_all :
 * @cart: #YElementViewCartesian
 *
 * Have all view intervals in @cart emit a "preferred_range_request" signal.
 **/
void
y_element_view_cartesian_set_preferred_view_all (YElementViewCartesian * cart)
{
  gint i;
  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart));

  YElementViewCartesianPrivate *priv =
    y_element_view_cartesian_get_instance_private (cart);

  /* Grossly inefficient, and wrong */

  for (i = 0; i < LAST_AXIS; ++i)
    {
      if (priv->y_view_interval[i] != NULL)
        y_element_view_cartesian_set_preferred_view (cart, (YAxisType) i);
    }
}

/**
 * y_element_view_cartesian_force_preferred_view :
 * @cart: #YElementViewCartesian
 * @axis: axis
 * @force: whether to force
 *
 *
 **/
void
y_element_view_cartesian_force_preferred_view (YElementViewCartesian * cart,
					       YAxisType axis, gboolean force)
{
  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart));
  g_assert (0 <= axis && axis < LAST_AXIS);

  YElementViewCartesianPrivate *priv =
    y_element_view_cartesian_get_instance_private (cart);

  priv->vi_force_preferred[axis] = force;

  if (force)
    y_element_view_cartesian_set_preferred_view (cart, axis);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

/*
 *
 * YAxisMarkers gadgetry
 *
 */

static void
am_changed (YAxisMarkers * gam, ViewAxisPair * pair)
{
  YElementView *view = Y_ELEMENT_VIEW (pair->cart);

  y_element_view_changed (view);
}

static void
set_axis_markers (YElementViewCartesian * cart,
		  YAxisType ax, YAxisMarkers * mark)
{
  YElementViewCartesianPrivate *p;

  g_assert (0 <= ax && ax < LAST_AXIS);

  p = y_element_view_cartesian_get_instance_private (cart);

  if (p->axis_markers[ax] != NULL)
    {
      g_signal_handler_disconnect (p->axis_markers[ax],
				   p->am_changed_handler[ax]);
      p->am_changed_handler[ax] = 0;
    }

  g_set_object (&p->axis_markers[ax], mark);

  if (mark != NULL)
    {

      if (p->vi_closure[ax] == NULL)
      {
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

/**
 * y_element_view_cartesian_add_axis_markers :
 * @cart: #YElementViewCartesian
 * @axis: the axis type
 *
 * Adds axis markers to an axis, if they do not already exist.
 **/
void
y_element_view_cartesian_add_axis_markers (YElementViewCartesian * cart,
					   YAxisType axis)
{
  g_debug ("add_axis_markers");

  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart));
  g_assert (0 <= axis && axis < LAST_AXIS);

  YElementViewCartesianPrivate *priv =
    y_element_view_cartesian_get_instance_private (cart);

  if (priv->axis_markers[axis] == NULL)
    {
      YAxisMarkers *am = y_axis_markers_new ();
      set_axis_markers (cart, axis, am);
      g_object_unref (G_OBJECT (am));
    }
}

/**
 * y_element_view_cartesian_get_axis_marker_type :
 * @cart: #YElementViewCartesian
 * @axis: the axis
 *
 * Get the type of axis markers for axis @ax.
 *
 * Returns: the axis marker type
 **/
gint
y_element_view_cartesian_get_axis_marker_type (YElementViewCartesian * cart,
					       YAxisType axis)
{
  g_return_val_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart), -1);
  g_assert (0 <= axis && axis < LAST_AXIS);

  YElementViewCartesianPrivate *priv =
    y_element_view_cartesian_get_instance_private (cart);

  return priv->axis_marker_type[axis];
}

/**
 * y_element_view_cartesian_set_axis_marker_type :
 * @cart: #YElementViewCartesian
 * @axis: the axis
 * @code: the type
 *
 * Set the type of axis markers for axis @ax, and adds axis markers if they
 * do not exist for the axis type.
 **/
void
y_element_view_cartesian_set_axis_marker_type (YElementViewCartesian * cart,
					       YAxisType axis, gint code)
{
  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart));
  g_assert (0 <= axis && axis < LAST_AXIS);

  YElementViewCartesianPrivate *priv =
    y_element_view_cartesian_get_instance_private (cart);

  priv->axis_marker_type[axis] = code;
  y_element_view_cartesian_add_axis_markers (cart, axis);
  compute_markers (cart, axis);
}

/**
 * y_element_view_cartesian_get_axis_markers :
 * @cart: #YElementViewCartesian
 * @ax: the axis
 *
 * Get the #YAxisMarkers object associated with axis @ax.
 *
 * Returns: (transfer none): the #YAxisMarkers object
 **/
YAxisMarkers *
y_element_view_cartesian_get_axis_markers (YElementViewCartesian * cart,
					   YAxisType ax)
{
  YAxisMarkers *gam;

  g_return_val_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart), NULL);
  g_assert (0 <= ax && ax < LAST_AXIS);

  YElementViewCartesianPrivate *priv =
    y_element_view_cartesian_get_instance_private (cart);

  gam = priv->axis_markers[ax];
  if (gam)
    y_axis_markers_sort (gam);

  return gam;
}

/**
 * y_element_view_cartesian_connect_axis_markers :
 * @cart1: a #YElementViewCartesian
 * @axis1: an axis
 * @cart2: a #YElementViewCartesian
 * @axis2: an axis
 *
 * Bind axis markers and view intervals for two #YElementViewCartesian's. The
 * axis markers and view interval for axis @axis2 of @cart2 are set as those
 * for axis @axis1 of @cart1. This is used, for example, to connect the X axis
 * of a scatter view to an axis view.
 **/
void
y_element_view_cartesian_connect_axis_markers (YElementViewCartesian * cart1,
					       YAxisType axis1,
					       YElementViewCartesian * cart2,
					       YAxisType axis2)
{
  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart1));
  g_return_if_fail (Y_IS_ELEMENT_VIEW_CARTESIAN (cart2));
  g_assert (0 <= axis1 && axis1 < LAST_AXIS);
  g_assert (0 <= axis2 && axis2 < LAST_AXIS);

  y_element_view_freeze ((YElementView *) cart2);

  set_axis_markers (cart2, axis2,
		    y_element_view_cartesian_get_axis_markers (cart1, axis1));

  y_element_view_cartesian_connect_view_intervals (cart1, axis1, cart2, axis2);

  y_element_view_changed ((YElementView *) cart2);
  y_element_view_thaw ((YElementView *) cart2);
}

static void
autoscale_toggled (GtkCheckMenuItem * checkmenuitem, gpointer user_data)
{
  YViewInterval *vi = Y_VIEW_INTERVAL (user_data);
  y_view_interval_set_ignore_preferred_range (vi,
					      !gtk_check_menu_item_get_active
					      (checkmenuitem));
}

void y_rescale_around_val(YViewInterval *vi, double x, GdkEventButton *event)
{
  if (event->state & GDK_MOD1_MASK)
  {
    y_view_interval_rescale_around_point (vi, x, 1.0 / 0.8);
  }
  else
  {
    y_view_interval_rescale_around_point (vi, x, 0.8);
  }
}

GtkWidget *
_y_create_autoscale_menu_check_item (YElementViewCartesian * view, YAxisType ax, const gchar * label)
{
  GtkWidget *autoscale_x = gtk_check_menu_item_new_with_label (label);
  YViewInterval *vix = y_element_view_cartesian_get_view_interval (view,
								   ax);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (autoscale_x),
				  !y_view_interval_get_ignore_preferred_range(vix));
  g_signal_connect (autoscale_x, "toggled", G_CALLBACK (autoscale_toggled),
		    vix);
  return autoscale_x;
}
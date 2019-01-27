/*
 * b-element-view-cartesian.c
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

#include "plot/b-element-view-cartesian.h"

#include <math.h>
#include <string.h>

/**
 * SECTION: b-element-view-cartesian
 * @short_description: Base class for plot objects with Cartesian coordinates.
 *
 * Abstract base class for plot classes #BScatterLineView, #BAxisView, and others.
 *
 */

typedef struct _ViewAxisPair ViewAxisPair;
struct _ViewAxisPair
{
  BElementViewCartesian *cart;
  BAxisType axis;

  /* We cache the class of the cartesian view */
  BElementViewCartesianClass *klass;
};

typedef struct
{
  BViewInterval *b_view_interval[LAST_AXIS];
  guint vi_changed_handler[LAST_AXIS];
  guint vi_prefrange_handler[LAST_AXIS];
  gboolean vi_force_preferred[LAST_AXIS];
  ViewAxisPair *vi_closure[LAST_AXIS];

  gboolean forcing_preferred;
  guint pending_force_tag;

  gint axis_marker_type[LAST_AXIS];
  BAxisMarkers *axis_markers[LAST_AXIS];
  guint am_changed_handler[LAST_AXIS];
} BElementViewCartesianPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (BElementViewCartesian,
				     b_element_view_cartesian,
				     B_TYPE_ELEMENT_VIEW);

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
update_axis_markers (BElementViewCartesian * cart,
		     BAxisType ax,
		     BAxisMarkers * marks, double range_min, double range_max)
{
  g_assert (0 <= ax && ax < LAST_AXIS);

  BElementViewCartesianPrivate *priv =
    b_element_view_cartesian_get_instance_private (cart);

  g_debug ("update_axis_markers: %d, %p", priv->axis_marker_type[ax], marks);

  if (marks && priv->axis_marker_type[ax] != B_AXIS_NONE)
    {
      g_debug ("really update_axis_markers");
      b_axis_markers_populate_generic (marks,
				       priv->axis_marker_type[ax],
				       range_min, range_max);
    }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
b_element_view_cartesian_finalize (GObject * obj)
{
  BElementViewCartesian *cart;
  BElementViewCartesianPrivate *p;
  gint i;

  cart = B_ELEMENT_VIEW_CARTESIAN (obj);
  p = b_element_view_cartesian_get_instance_private (cart);

  /* clean up the view intervals */

  for (i = 0; i < LAST_AXIS; ++i)
    {
      if (p->b_view_interval[i])
      {
        if (p->vi_changed_handler[i])
          g_signal_handler_disconnect (p->b_view_interval[i],
                                       p->vi_changed_handler[i]);
        if (p->vi_prefrange_handler[i])
          g_signal_handler_disconnect (p->b_view_interval[i],
                                       p->vi_prefrange_handler[i]);

        g_clear_object (&p->b_view_interval[i]);
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
    G_OBJECT_CLASS (b_element_view_cartesian_parent_class);

  if (obj_class->finalize)
    obj_class->finalize (obj);
}

static void
b_element_view_cartesian_class_init (BElementViewCartesianClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->finalize = b_element_view_cartesian_finalize;
  klass->update_axis_markers = update_axis_markers;
}


static void
b_element_view_cartesian_init (BElementViewCartesian * cart)
{
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
compute_markers (BElementViewCartesian * cart, BAxisType ax)
{
  BElementViewCartesianClass *klass;
  double a = 0, b = 0;

  g_debug ("computing markers");

  g_assert (0 <= ax && ax < LAST_AXIS);

  BElementViewCartesianPrivate *priv =
    b_element_view_cartesian_get_instance_private (cart);

  klass = B_ELEMENT_VIEW_CARTESIAN_CLASS (G_OBJECT_GET_CLASS (cart));

  if (priv->axis_markers[ax] != NULL && klass->update_axis_markers != NULL)
    {
      g_debug ("really computing markers");
      BViewInterval *vi =
        b_element_view_cartesian_get_view_interval (cart, ax);
      BAxisMarkers *am = priv->axis_markers[ax];

      if (vi && am)
      {
        g_debug ("really really computing markers");
        b_view_interval_range (vi, &a, &b);
        klass->update_axis_markers (cart, ax, am, a, b);
      }
    }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

/*
 *
 * YBViewInterval gadgetry
 *
 */

static gboolean
force_all_preferred_idle (gpointer ptr)
{
  BElementViewCartesian *cart = ptr;
  gint i;

  BElementViewCartesianPrivate *priv =
    b_element_view_cartesian_get_instance_private (cart);

  for (i = 0; i < LAST_AXIS; ++i)
    {
      if (priv->b_view_interval[i] && priv->vi_force_preferred[i])
        b_element_view_cartesian_set_preferred_view (cart, i);
    }
  priv->pending_force_tag = 0;

  return FALSE;
}

static void
vi_changed (BViewInterval * vi, ViewAxisPair * pair)
{
  BElementViewCartesian *cart = pair->cart;
  BAxisType ax = pair->axis;

  BElementViewCartesianPrivate *p =
    b_element_view_cartesian_get_instance_private (cart);

  b_element_view_freeze ((BElementView *) cart);

  if (p->vi_force_preferred[ax])
    {
      if (p->vi_changed_handler[ax])
        g_signal_handler_block (vi, p->vi_changed_handler[ax]);

      b_element_view_cartesian_set_preferred_view (cart, ax);

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

  b_element_view_changed (B_ELEMENT_VIEW (cart));

  b_element_view_thaw ((BElementView *) cart);
}

static void
vi_preferred (BViewInterval * vi, ViewAxisPair * pair)
{
  BElementViewCartesian *cart = pair->cart;
  BAxisType ax = pair->axis;
  double a, b;

  if (pair->klass == NULL)
    {
      pair->klass =
	B_ELEMENT_VIEW_CARTESIAN_CLASS (G_OBJECT_GET_CLASS (cart));
      g_assert (pair->klass);
    }

  /* Our 'changed' signals are automatically blocked during view
     interval range requests, so we don't need to worry about
     freeze/thaw. */

  if (pair->klass->preferred_range
      && pair->klass->preferred_range (cart, ax, &a, &b))
    {
      b_view_interval_grow_to (vi, a, b);
    }
}

static void
set_view_interval (BElementViewCartesian * cart,
                     BAxisType ax, BViewInterval * vi)
{
  BElementViewCartesianPrivate *p =
    b_element_view_cartesian_get_instance_private (cart);
  gint i = (int) ax;

  g_assert (0 <= i && i < LAST_AXIS);

  if (p->b_view_interval[i] == vi)
    return;

  if (p->b_view_interval[i] && p->vi_changed_handler[i])
    {
      g_signal_handler_disconnect (p->b_view_interval[i],
				   p->vi_changed_handler[i]);
      p->vi_changed_handler[i] = 0;
    }

  if (p->vi_prefrange_handler[i])
    {
      g_signal_handler_disconnect (p->b_view_interval[i],
				   p->vi_prefrange_handler[i]);
      p->vi_prefrange_handler[i] = 0;
    }

  g_set_object (&p->b_view_interval[i], vi);

  if (vi != NULL)
    {

      if (p->vi_closure[i] == NULL)
        {
          p->vi_closure[i] = g_slice_new0 (ViewAxisPair);
          p->vi_closure[i]->cart = cart;
          p->vi_closure[i]->axis = ax;
        }

      p->vi_changed_handler[i] = g_signal_connect (p->b_view_interval[i],
						   "changed",
						   (GCallback) vi_changed,
						   p->vi_closure[i]);

      p->vi_prefrange_handler[i] = g_signal_connect (p->b_view_interval[i],
						     "preferred_range_request",
						     (GCallback) vi_preferred,
						     p->vi_closure[i]);

      g_debug ("set_view_interval");

      compute_markers (cart, ax);
    }
  else
    {
      g_warning ("failed to set b_view_interval, %p %d",cart,ax);
    }
}

/*** BViewInterval-related API calls ***/

/**
 * b_element_view_cartesian_add_view_interval :
 * @cart: #BElementViewCartesian
 * @ax: the axis
 *
 * Add a #BViewInterval to @cart for axis @ax. The view interval will
 * immediately emit a "preferred_range_request" signal.
 **/
void
b_element_view_cartesian_add_view_interval (BElementViewCartesian * cart,
					    BAxisType ax)
{
  g_return_if_fail (B_IS_ELEMENT_VIEW_CARTESIAN (cart));
  g_assert (0 <= ax && ax < LAST_AXIS);

  BElementViewCartesianPrivate *priv =
    b_element_view_cartesian_get_instance_private (cart);

  if (priv->b_view_interval[ax] == NULL)
    {
      BViewInterval *vi = b_view_interval_new ();
      set_view_interval (cart, ax, vi);
      b_view_interval_request_preferred_range (vi);
      g_object_unref (G_OBJECT (vi));
    }
  else
    {
      g_warning ("b_element_view_cartesian_add_view_interval: vi already present");
    }
}

/**
 * b_element_view_cartesian_get_view_interval :
 * @cart: #BElementViewCartesian
 * @ax: the axis
 *
 * Get the #BViewInterval object associated with axis @ax.
 *
 * Returns: (transfer none): the #BViewInterval
 **/
BViewInterval *
b_element_view_cartesian_get_view_interval (BElementViewCartesian * cart,
					    BAxisType ax)
{
  g_return_val_if_fail (B_IS_ELEMENT_VIEW_CARTESIAN (cart), NULL);
  g_assert (0 <= ax && ax < LAST_AXIS);

  BElementViewCartesianPrivate *priv =
    b_element_view_cartesian_get_instance_private (cart);

  return priv->b_view_interval[ax];
}

/**
 * b_element_view_cartesian_connect_view_intervals :
 * @cart1: a #BElementViewCartesian
 * @axis1: an axis
 * @cart2: a #BElementViewCartesian
 * @axis2: an axis
 *
 * Bind view intervals for two #BElementViewCartesian's. The #BViewInterval for
 * axis @axis2 of @cart2 is set as the #BViewInterval for axis @axis1 of @cart1.
 * This is used, for example, to connect an axis of a scatter view to an axis
 * view.
 **/
void
b_element_view_cartesian_connect_view_intervals (BElementViewCartesian *cart1,
                                                 BAxisType axis1,
                                                 BElementViewCartesian *cart2,
                                                 BAxisType axis2)
{
  g_return_if_fail (B_IS_ELEMENT_VIEW_CARTESIAN (cart1));
  g_return_if_fail (0 <= axis1 && axis1 < LAST_AXIS);
  g_return_if_fail (B_IS_ELEMENT_VIEW_CARTESIAN (cart2));
  g_return_if_fail (0 <= axis2 && axis2 < LAST_AXIS);

  if (axis1 == axis2 && cart1 == cart2)
    return;
  set_view_interval(cart2, axis2,
                      b_element_view_cartesian_get_view_interval (cart1,axis1));

  b_element_view_changed (B_ELEMENT_VIEW (cart2));
}

/***************************************************************************/

/**
 * b_element_view_cartesian_set_preferred_view :
 * @cart: #BElementViewCartesian
 * @axis: the axis
 *
 * Have the view intervals for @ax emit a "preferred_range_request" signal.
 **/
void
b_element_view_cartesian_set_preferred_view (BElementViewCartesian * cart,
					     BAxisType axis)
{
  g_return_if_fail (B_IS_ELEMENT_VIEW_CARTESIAN (cart));
  g_assert (0 <= axis && axis < LAST_AXIS);

  BElementViewCartesianPrivate *priv =
    b_element_view_cartesian_get_instance_private (cart);

  if (priv->b_view_interval[axis] != NULL)
    b_view_interval_request_preferred_range (priv->b_view_interval[axis]);
}

/**
 * b_element_view_cartesian_set_preferred_view_all :
 * @cart: #BElementViewCartesian
 *
 * Have all view intervals in @cart emit a "preferred_range_request" signal.
 **/
void
b_element_view_cartesian_set_preferred_view_all (BElementViewCartesian * cart)
{
  gint i;
  g_return_if_fail (B_IS_ELEMENT_VIEW_CARTESIAN (cart));

  BElementViewCartesianPrivate *priv =
    b_element_view_cartesian_get_instance_private (cart);

  /* Grossly inefficient, and wrong */

  for (i = 0; i < LAST_AXIS; ++i)
    {
      if (priv->b_view_interval[i] != NULL)
        b_element_view_cartesian_set_preferred_view (cart, (BAxisType) i);
    }
}

/**
 * b_element_view_cartesian_force_preferred_view :
 * @cart: #BElementViewCartesian
 * @axis: axis
 * @force: whether to force
 *
 *
 **/
void
b_element_view_cartesian_force_preferred_view (BElementViewCartesian * cart,
					       BAxisType axis, gboolean force)
{
  g_return_if_fail (B_IS_ELEMENT_VIEW_CARTESIAN (cart));
  g_assert (0 <= axis && axis < LAST_AXIS);

  BElementViewCartesianPrivate *priv =
    b_element_view_cartesian_get_instance_private (cart);

  priv->vi_force_preferred[axis] = force;

  if (force)
    b_element_view_cartesian_set_preferred_view (cart, axis);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

/*
 *
 * BAxisMarkers gadgetry
 *
 */

static void
am_changed (BAxisMarkers * gam, ViewAxisPair * pair)
{
  BElementView *view = B_ELEMENT_VIEW (pair->cart);

  b_element_view_changed (view);
}

static void
set_axis_markers (BElementViewCartesian * cart,
		  BAxisType ax, BAxisMarkers * mark)
{
  BElementViewCartesianPrivate *p;

  g_assert (0 <= ax && ax < LAST_AXIS);

  p = b_element_view_cartesian_get_instance_private (cart);

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
 * b_element_view_cartesian_add_axis_markers :
 * @cart: #BElementViewCartesian
 * @axis: the axis type
 *
 * Adds axis markers to an axis, if they do not already exist.
 **/
void
b_element_view_cartesian_add_axis_markers (BElementViewCartesian * cart,
					   BAxisType axis)
{
  g_debug ("add_axis_markers");

  g_return_if_fail (B_IS_ELEMENT_VIEW_CARTESIAN (cart));
  g_assert (0 <= axis && axis < LAST_AXIS);

  BElementViewCartesianPrivate *priv =
    b_element_view_cartesian_get_instance_private (cart);

  if (priv->axis_markers[axis] == NULL)
    {
      BAxisMarkers *am = b_axis_markers_new ();
      set_axis_markers (cart, axis, am);
      g_object_unref (G_OBJECT (am));
    }
}

/**
 * b_element_view_cartesian_get_axis_marker_type :
 * @cart: #BElementViewCartesian
 * @axis: the axis
 *
 * Get the type of axis markers for axis @ax.
 *
 * Returns: the axis marker type
 **/
gint
b_element_view_cartesian_get_axis_marker_type (BElementViewCartesian * cart,
					       BAxisType axis)
{
  g_return_val_if_fail (B_IS_ELEMENT_VIEW_CARTESIAN (cart), -1);
  g_assert (0 <= axis && axis < LAST_AXIS);

  BElementViewCartesianPrivate *priv =
    b_element_view_cartesian_get_instance_private (cart);

  return priv->axis_marker_type[axis];
}

/**
 * b_element_view_cartesian_set_axis_marker_type :
 * @cart: #BElementViewCartesian
 * @axis: the axis
 * @code: the type
 *
 * Set the type of axis markers for axis @ax, and adds axis markers if they
 * do not exist for the axis type.
 **/
void
b_element_view_cartesian_set_axis_marker_type (BElementViewCartesian * cart,
					       BAxisType axis, gint code)
{
  g_return_if_fail (B_IS_ELEMENT_VIEW_CARTESIAN (cart));
  g_assert (0 <= axis && axis < LAST_AXIS);

  BElementViewCartesianPrivate *priv =
    b_element_view_cartesian_get_instance_private (cart);

  priv->axis_marker_type[axis] = code;
  b_element_view_cartesian_add_axis_markers (cart, axis);
  compute_markers (cart, axis);
}

/**
 * b_element_view_cartesian_get_axis_markers :
 * @cart: #BElementViewCartesian
 * @ax: the axis
 *
 * Get the #BAxisMarkers object associated with axis @ax.
 *
 * Returns: (transfer none): the #BAxisMarkers object
 **/
BAxisMarkers *
b_element_view_cartesian_get_axis_markers (BElementViewCartesian * cart,
					   BAxisType ax)
{
  BAxisMarkers *gam;

  g_return_val_if_fail (B_IS_ELEMENT_VIEW_CARTESIAN (cart), NULL);
  g_assert (0 <= ax && ax < LAST_AXIS);

  BElementViewCartesianPrivate *priv =
    b_element_view_cartesian_get_instance_private (cart);

  gam = priv->axis_markers[ax];
  if (gam)
    b_axis_markers_sort (gam);

  return gam;
}

/**
 * b_element_view_cartesian_connect_axis_markers :
 * @cart1: a #BElementViewCartesian
 * @axis1: an axis
 * @cart2: a #BElementViewCartesian
 * @axis2: an axis
 *
 * Bind axis markers and view intervals for two #BElementViewCartesian's. The
 * axis markers and view interval for axis @axis2 of @cart2 are set as those
 * for axis @axis1 of @cart1. This is used, for example, to connect the X axis
 * of a scatter view to an axis view.
 **/
void
b_element_view_cartesian_connect_axis_markers (BElementViewCartesian * cart1,
					       BAxisType axis1,
					       BElementViewCartesian * cart2,
					       BAxisType axis2)
{
  g_return_if_fail (B_IS_ELEMENT_VIEW_CARTESIAN (cart1));
  g_return_if_fail (B_IS_ELEMENT_VIEW_CARTESIAN (cart2));
  g_assert (0 <= axis1 && axis1 < LAST_AXIS);
  g_assert (0 <= axis2 && axis2 < LAST_AXIS);

  b_element_view_freeze ((BElementView *) cart2);

  set_axis_markers (cart2, axis2,
		    b_element_view_cartesian_get_axis_markers (cart1, axis1));

  b_element_view_cartesian_connect_view_intervals (cart1, axis1, cart2, axis2);

  b_element_view_changed ((BElementView *) cart2);
  b_element_view_thaw ((BElementView *) cart2);
}

static void
autoscale_toggled (GtkCheckMenuItem * checkmenuitem, gpointer user_data)
{
  BViewInterval *vi = B_VIEW_INTERVAL (user_data);
  b_view_interval_set_ignore_preferred_range (vi,
					      !gtk_check_menu_item_get_active
					      (checkmenuitem));
}

void b_rescale_around_val(BViewInterval *vi, double x, GdkEventButton *event)
{
  if (event->state & GDK_MOD1_MASK)
  {
    b_view_interval_rescale_around_point (vi, x, 1.0 / 0.8);
  }
  else
  {
    b_view_interval_rescale_around_point (vi, x, 0.8);
  }
}

void
_format_double_scinot (gchar *buffer, double x)
{
  if(isnan(x))
    {
      sprintf(buffer,"NaN");
      return;
    }
  if(x==0.0 || (fabs(x)<1000.0 && fabs(x)>0.001))
    {
      sprintf(buffer,"%1.3f",x);
    }
  else
    {
      double ex = floor(log10(fabs(x)));
      double mx = (x/pow(10.0,ex));
      sprintf(buffer,"%1.3f⨉10<sup>%d</sup>",mx,(int) ex);
    }
}

GtkWidget *
_y_create_autoscale_menu_check_item (BElementViewCartesian * view, BAxisType ax, const gchar * label)
{
  GtkWidget *autoscale_x = gtk_check_menu_item_new_with_label (label);
  BViewInterval *vix = b_element_view_cartesian_get_view_interval (view,
								   ax);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (autoscale_x),
				  !b_view_interval_get_ignore_preferred_range(vix));
  g_signal_connect (autoscale_x, "toggled", G_CALLBACK (autoscale_toggled),
		    vix);
  return autoscale_x;
}

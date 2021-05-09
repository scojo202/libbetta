/*
 * b-view-interval.c
 *
 * Copyright (C) 2000 EMC Capital Management, Inc.
 * Copyright (C) 2001 The Free Software Foundation
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

#include "plot/b-view-interval.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>

/**
 * SECTION: b-view-interval
 * @short_description: Object for controlling an interval.
 *
 * This is used to control the range shown in a #BElementViewCartesian.
 *
 *
 */

static GObjectClass *parent_class = NULL;

enum
{
  CHANGED,
  PREFERRED_RANGE_REQUEST,
  LAST_SIGNAL
};

static guint gvi_signals[LAST_SIGNAL] = { 0 };

static void
b2sort (double *a, double *b)
{
  double t;

  if (*a > *b)
    {
      t = *a;
      *a = *b;
      *b = t;
    }
}

struct _BViewInterval {
  GObject parent;

  gint type;
  double type_arg;

  double t0, t1;

  double min, max;
  double min_width;
  guint include_min : 1;
  guint include_max : 1;

  guint block_changed_signals : 1;

  guint ignore_preferred : 1;
};

static void
b_view_interval_finalize (GObject * obj)
{
  if (parent_class->finalize)
    parent_class->finalize (obj);
}

static void
b_view_interval_class_init (BViewIntervalClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = b_view_interval_finalize;

  gvi_signals[CHANGED] =
    g_signal_new ("changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  0,
		  NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  gvi_signals[PREFERRED_RANGE_REQUEST] =
    g_signal_new ("preferred_range_request",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  0, NULL, NULL,
		  g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
b_view_interval_init (BViewInterval * obj)
{
  obj->type = VIEW_NORMAL;
  obj->t0 = 0;
  obj->t1 = 1;
  obj->min = -HUGE_VAL;
  obj->max = HUGE_VAL;
  obj->min_width = 0;

  obj->include_min = TRUE;
  obj->include_max = TRUE;

  obj->ignore_preferred = FALSE;
}

G_DEFINE_TYPE (BViewInterval, b_view_interval, G_TYPE_OBJECT);

BViewInterval *
b_view_interval_new (void)
{
  BViewInterval *v =
    B_VIEW_INTERVAL (g_object_new (b_view_interval_get_type (), NULL));
  return v;
}

static void
changed (BViewInterval * v)
{
  if (!v->block_changed_signals)
    g_signal_emit (v, gvi_signals[CHANGED], 0);
}

/**
 * b_view_interval_get_vi_type :
 * @v: #BViewInterval
 *
 * Gets the type of the #BViewInterval, which could be for example VIEW_NORMAL
 * if the view is linear, or VIEW_LOG if it is logarithmic.
 *
 * Returns: The type
 **/
int
b_view_interval_get_vi_type(BViewInterval *v)
{
	return v->type;
}

/**
 * b_view_interval_set :
 * @v: #BViewInterval
 * @a: lower edge
 * @b: upper edge
 *
 * Set a #BViewInterval to have edges at @a and @b. If these are different from
 * its previous edges, the "changed" signal will be emitted.
 **/
void
b_view_interval_set (BViewInterval * v, double a, double b)
{
  g_return_if_fail (B_IS_VIEW_INTERVAL (v));

  b2sort (&a, &b);
  if (a < v->min)
    a = v->min;
  if (b > v->max)
    b = v->max;

  if (b - a < v->min_width)
    return;

  if (b_view_interval_is_logarithmic (v))
    {
      if (b <= 0)
        b = 1;
      if (a <= 0)
        a = b / 1e+10;
    }

  if (v->t0 != a || v->t1 != b)
    {
      v->t0 = a;
      v->t1 = b;
      changed (v);
    }
}

/**
 * b_view_interval_grow_to :
 * @v: #BViewInterval
 * @a: lower edge
 * @b: upper edge
 *
 * Increases the size of a #BViewInterval to include values @a and @b.
 **/
void
b_view_interval_grow_to (BViewInterval * v, double a, double b)
{
  g_return_if_fail (B_IS_VIEW_INTERVAL (v));

  if (a > b)
    {
      double t = a;
      a = b;
      b = t;
    }

  if (v->t0 <= v->t1)
    {
      b_view_interval_set (v, MIN (a, v->t0), MAX (b, v->t1));
    }
  else
    {
      b_view_interval_set (v, a, b);
    }
}

/**
 * b_view_interval_range :
 * @v: #BViewInterval
 * @a: (out)(nullable): lower edge
 * @b: (out)(nullable): upper edge
 *
 * Get the edges @a and @b of a #BViewInterval.
 **/
void
b_view_interval_range (BViewInterval * v, double *a, double *b)
{
  g_return_if_fail (B_IS_VIEW_INTERVAL (v));

  if (a)
    *a = v->t0;
  if (b)
    *b = v->t1;
}

/**
 * b_view_interval_set_bounds :
 * @v: #BViewInterval
 * @a: lower bound
 * @b: upper bound
 *
 * Set a #BViewInterval to have bounds at @a and @b. These are the maximum and
 * minimum values that its edges can take.
 **/
void
b_view_interval_set_bounds (BViewInterval * v, double a, double b)
{
  g_return_if_fail (B_IS_VIEW_INTERVAL (v));

  b2sort (&a, &b);

  v->min = a;
  v->max = b;
}

/**
 * b_view_interval_get_width :
 * @v: #BViewInterval
 *
 * Get the width of the view interval.
 **/
double b_view_interval_get_width (BViewInterval *v)
{
  g_return_val_if_fail (B_IS_VIEW_INTERVAL (v), NAN);
  return v->t1-v->t0;
}

/**
 * b_view_interval_clear_bounds :
 * @v: #BViewInterval
 *
 * Reset a #BViewInterval's bounds to the default values, -HUGE_VAL and HUGE_VAL.
 **/
void
b_view_interval_clear_bounds (BViewInterval * v)
{
  g_return_if_fail (B_IS_VIEW_INTERVAL (v));

  v->min = -HUGE_VAL;
  v->max = HUGE_VAL;
}

/**
 * b_view_interval_set_min_width :
 * @v: #BViewInterval
 * @mw: minimum width
 *
 * Set a #BViewInterval's minimum width.
 **/
void
b_view_interval_set_min_width (BViewInterval * v, double mw)
{
  g_return_if_fail (B_IS_VIEW_INTERVAL (v));

  v->min_width = mw;
}

/**
 * b_view_interval_valid :
 * @v: #BViewInterval
 * @x: a value to test
 *
 * Check whether a number could possibly exist on a view interval. For example,
 * only positive values are valid on a logarithmic view interval.
 *
 * Returns: %TRUE if @x is valid for @v.
 **/
gboolean
b_view_interval_valid (BViewInterval * v, double x)
{
  g_return_val_if_fail (B_IS_VIEW_INTERVAL (v), FALSE);

  switch (v->type) {

     case VIEW_LOG:
     return x > 0;
     break;

     #if 0
     case VIEW_RECIPROCAL:
     return x != 0;
     #endif

     //default:
     //int i = 9;
     }

  return TRUE;
}

/**
 * b_view_interval_conv :
 * @v: #BViewInterval
 * @x: a value to convert
 *
 * Convert a double-precision value to the view interval's coordinates
 *
 * Returns: the converted value, which is between 0 and 1 if the value is
 * inside the view interval.
 **/
double
b_view_interval_conv (BViewInterval * v, double x)
{
  double t0, t1;

  t0 = v->t0;
  t1 = v->t1;

  switch (v->type)
    {

    case VIEW_NORMAL:
      /* do nothing */
      break;

    case VIEW_LOG:

      return log (x / t0) / log (t1 / t0);

#if 0
    case VIEW_RECIPROCAL:
      x = x ? 1 / x : 0;
      t0 = t0 ? 1 / t0 : 0;
      t1 = t1 ? 1 / t1 : 0;
      break;
#endif

    default:
      g_assert_not_reached ();

    }

  return (x - t0) / (t1 - t0);
}

/**
 * b_view_interval_unconv :
 * @v: #BViewInterval
 * @x: a value to convert
 *
 * Convert a value from the ViewInterval's coordinates (0 to 1)
 *
 * Returns: the value.
 **/
double
b_view_interval_unconv (BViewInterval * v, double x)
{
  double t0, t1;

  g_return_val_if_fail (B_IS_VIEW_INTERVAL (v), 0);

  t0 = v->t0;
  t1 = v->t1;

  switch (v->type)
    {

    case VIEW_NORMAL:
      return t0 + x * (t1 - t0);
      break;

    case VIEW_LOG:
      return t0 * pow (t1 / t0, x);

    default:
      g_assert_not_reached ();
    }

  return 0;
}

/**
 * b_view_interval_conv_bulk :
 * @v: #BViewInterval
 * @in_data: values to convert
 * @out_data: output array of values
 * @N: length of arrays
 *
 * Convert an array of double-precision values to the ViewInterval's
 * coordinates.
 **/
void
b_view_interval_conv_bulk (BViewInterval * v,
			   const double *in_data, double *out_data, gsize N)
{
  double t0, t1, tsize, x, y = 0, c = 0;
  gsize i;
  gint type;

  g_return_if_fail (B_IS_VIEW_INTERVAL (v));
  g_return_if_fail (out_data != NULL);
  g_return_if_fail (N == 0 || in_data != NULL);

  if (N == 0)
    return;

  t0 = v->t0;
  t1 = v->t1;
  tsize = t1 - t0;
  type = v->type;

  if (type == VIEW_LOG)
    c = log (t1 / t0);

  for (i = 0; i < N; ++i)
    {
      x = in_data[i];

      if (type == VIEW_NORMAL)
	{
	  y = (x - t0) / tsize;
	}
      else if (type == VIEW_LOG)
	{
	  y = log (x / t0) / c;
	}
      else
	{
	  g_assert_not_reached ();
	}

      out_data[i] = y;
    }
}

/**
 * b_view_interval_unconv_bulk :
 * @v: #BViewInterval
 * @in_data: values to convert
 * @out_data: output array of values
 * @N: length of arrays
 *
 * Convert an array from the ViewInterval's coordinates to data coordinates.
 **/
void
b_view_interval_unconv_bulk (BViewInterval * v,
			     const double *in_data, double *out_data, gsize N)
{
  double t0, t1, x, y = 0, c = 0;
  gsize i;
  gint type;

  g_return_if_fail (B_IS_VIEW_INTERVAL (v));
  g_return_if_fail (out_data != NULL);
  g_return_if_fail (N == 0 || in_data != NULL);

  if (N == 0)
    return;

  t0 = v->t0;
  t1 = v->t1;
  type = v->type;
  if (type == VIEW_LOG)
    c = t1 / t0;

  for (i = 0; i < N; ++i)
    {
      x = in_data[i];

      if (type == VIEW_NORMAL)
	y = t0 + x * (t1 - t0);
      else if (type == VIEW_LOG)
	y = t0 * pow (c, x);
      else
	g_assert_not_reached ();

      out_data[i] = y;
    }
}

/**
 * b_view_interval_rescale_around_point :
 * @v: #BViewInterval
 * @x: point
 * @s: scaling factor
 *
 * Scale @v by factor @s and center it around @x.
 **/
void
b_view_interval_rescale_around_point (BViewInterval * v, double x, double s)
{
  double a, b;

  g_return_if_fail (B_IS_VIEW_INTERVAL (v));

  if (s < 0)
    s = -s;

  if (s != 1)
    {

      x = b_view_interval_conv (v, x);

      /* I do this to be explicit: we are transforming the conv-coordinate
         edge-points of the interval. */
      a = s * (0 - x) + x;
      b = s * (1 - x) + x;

      a = b_view_interval_unconv (v, a);
      b = b_view_interval_unconv (v, b);

      b_view_interval_set (v, a, b);
    }
}

/**
 * b_view_interval_recenter_around_point :
 * @v: #BViewInterval
 * @x: point
 *
 * Center @v around a new point @x without changing its width.
 **/
void
b_view_interval_recenter_around_point (BViewInterval * v, double x)
{
  double a, b, c;
  g_return_if_fail (B_IS_VIEW_INTERVAL (v));

  b_view_interval_range (v, &a, &b);
  c = (a + b) / 2;
  if (c != x)
    b_view_interval_translate (v, x - c);
}

/**
 * b_view_interval_translate :
 * @v: #BViewInterval
 * @dx: point
 *
 * Move interval by @dx without changing its width.
 **/
void
b_view_interval_translate (BViewInterval * v, double dx)
{
  double a, b;

  g_return_if_fail (B_IS_VIEW_INTERVAL (v));

  b_view_interval_range (v, &a, &b);

  if (dx != 0 && v->min <= a + dx && b + dx <= v->max)
    {
      b_view_interval_set (v, a + dx, b + dx);
    }
}

/* move interval by dx without changing its width */
void
b_view_interval_conv_translate (BViewInterval * v, double x)
{
  double a, b;

  g_return_if_fail (B_IS_VIEW_INTERVAL (v));

  if (x == 0.0)
    return;

  a = x;
  b = 1 + x;

  if (!(b_view_interval_is_logarithmic (v) && v->t0 <= 0))
    {

      a = b_view_interval_unconv (v, a);

    }
  else
    {

      a = v->t0;

    }

  b = b_view_interval_unconv (v, b);

  b2sort (&a, &b);

  if (v->min <= a && b <= v->max)
    b_view_interval_set (v, a, b);
}

/**
 * b_view_interval_request_preferred_range :
 * @v: #BViewInterval
 *
 * Causes the #BViewInterval to emit a signal that will cause connected
 * views to set the view interval to their preferred range.
 **/
void
b_view_interval_request_preferred_range (BViewInterval * v)
{
  double p0, p1;

  g_return_if_fail (B_IS_VIEW_INTERVAL (v));

  if (v->ignore_preferred)
    return;

  p0 = v->t0;
  p1 = v->t1;

  v->block_changed_signals = TRUE;

  v->t0 = 0;
  v->t1 = -1;

  g_signal_emit (v, gvi_signals[PREFERRED_RANGE_REQUEST], 0);

  if (v->t0 > v->t1)
    b_view_interval_set (v, -0.05, 1.05);

  v->block_changed_signals = FALSE;

  if (v->t0 != p0 || v->t1 != p1)
    changed (v);
}

/**
 * b_view_interval_set_ignore_preferred_range :
 * @v: #BViewInterval
 * @ignore: a boolean
 *
 * Whether the #BViewInterval should ignore connected views' preferred ranges.
 **/
void
b_view_interval_set_ignore_preferred_range (BViewInterval * v,
					    gboolean ignore)
{
  v->ignore_preferred = ignore;
  if (!ignore)
    {
      b_view_interval_request_preferred_range (v);
    }
}

/**
 * b_view_interval_get_ignore_preferred_range :
 * @v: #BViewInterval
 *
 * Return whether the #BViewInterval is ignoring connected views' preferred ranges.
 **/
gboolean b_view_interval_get_ignore_preferred_range (BViewInterval *v)
{
	return v->ignore_preferred;
}

/**************************************************************************/

/**
 * b_view_interval_scale_linearly :
 * @v: #BViewInterval
 *
 * Use a linear scale for @v.
 **/
void
b_view_interval_scale_linearly (BViewInterval * v)
{
  g_return_if_fail (B_IS_VIEW_INTERVAL (v));

  if (v->type != VIEW_NORMAL)
    {
      v->type = VIEW_NORMAL;
      changed (v);
    }
}

/**
 * b_view_interval_scale_logarithmically :
 * @v: #BViewInterval
 * @base: the base, currently not used
 *
 * Use a logarithmic scale for @v, with base @base. The base isn't currently used.
 **/
void
b_view_interval_scale_logarithmically (BViewInterval * v, double base)
{
  g_return_if_fail (B_IS_VIEW_INTERVAL (v));

  if (v->type != VIEW_LOG)
    {
      v->type = VIEW_LOG;
      v->type_arg = base;
      changed (v);
    }
}

/**
 * b_view_interval_rescale_event :
 * @vi: #BViewInterval
 * @x: point
 * @event: #GdkEventButton
 *
 * Zoom in or out around point @x, depending on whether the GDK_MOD1_MASK flag is %TRUE.
 **/
void b_view_interval_rescale_event(BViewInterval *vi, double x, GdkEvent *event)
{
  if (gdk_event_get_modifier_state(event) & GDK_ALT_MASK)
  {
    b_view_interval_rescale_around_point (vi, x, 1.0 / 0.8);
  }
  else
  {
    b_view_interval_rescale_around_point (vi, x, 0.8);
  }
}

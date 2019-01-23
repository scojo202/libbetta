/*
 * y-view-interval.c
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

#include "plot/y-view-interval.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>

/**
 * SECTION: y-view-interval
 * @short_description: Object for controlling an interval.
 *
 * This is used to control the range shown in a #YElementViewCartesian.
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
g2sort (double *a, double *b)
{
  double t;

  if (*a > *b)
    {
      t = *a;
      *a = *b;
      *b = t;
    }
}

struct _YViewInterval {
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
y_view_interval_finalize (GObject * obj)
{
  if (parent_class->finalize)
    parent_class->finalize (obj);
}

static void
y_view_interval_class_init (YViewIntervalClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = y_view_interval_finalize;

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
y_view_interval_init (YViewInterval * obj)
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

G_DEFINE_TYPE (YViewInterval, y_view_interval, G_TYPE_OBJECT);

YViewInterval *
y_view_interval_new (void)
{
  YViewInterval *v =
    Y_VIEW_INTERVAL (g_object_new (y_view_interval_get_type (), NULL));
  return v;
}

static void
changed (YViewInterval * v)
{
  if (!v->block_changed_signals)
    g_signal_emit (v, gvi_signals[CHANGED], 0);
}

/**
 * y_view_interval_get_vi_type :
 * @v: #YViewInterval
 *
 * Gets the type of the #YViewInterval, which could be for example VIEW_NORMAL
 * if the view is linear, or VIEW_LOG if it is logarithmic.
 *
 * Returns: The type
 **/
int
y_view_interval_get_vi_type(YViewInterval *v)
{
	return v->type;
}

/**
 * y_view_interval_set :
 * @v: #YViewInterval
 * @a: lower edge
 * @b: upper edge
 *
 * Set a #YViewInterval to have edges at @a and @b. If these are different from
 * its previous edges, the "changed" signal will be emitted.
 **/
void
y_view_interval_set (YViewInterval * v, double a, double b)
{
  g_return_if_fail (Y_IS_VIEW_INTERVAL (v));

  g2sort (&a, &b);
  if (a < v->min)
    a = v->min;
  if (b > v->max)
    b = v->max;

  if (b - a < v->min_width)
    return;

  if (y_view_interval_is_logarithmic (v))
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
 * y_view_interval_grow_to :
 * @v: #YViewInterval
 * @a: lower edge
 * @b: upper edge
 *
 * Increases the size of a #YViewInterval to include values @a and @b.
 **/
void
y_view_interval_grow_to (YViewInterval * v, double a, double b)
{
  g_return_if_fail (Y_IS_VIEW_INTERVAL (v));

  if (a > b)
    {
      double t = a;
      a = b;
      b = t;
    }

  if (v->t0 <= v->t1)
    {
      y_view_interval_set (v, MIN (a, v->t0), MAX (b, v->t1));
    }
  else
    {
      y_view_interval_set (v, a, b);
    }
}

/**
 * y_view_interval_range :
 * @v: #YViewInterval
 * @a: (out)(nullable): lower edge
 * @b: (out)(nullable): upper edge
 *
 * Get the edges @a and @b of a #YViewInterval.
 **/
void
y_view_interval_range (YViewInterval * v, double *a, double *b)
{
  g_return_if_fail (Y_IS_VIEW_INTERVAL (v));

  if (a)
    *a = v->t0;
  if (b)
    *b = v->t1;
}

/**
 * y_view_interval_set_bounds :
 * @v: #YViewInterval
 * @a: lower bound
 * @b: upper bound
 *
 * Set a #YViewInterval to have bounds at @a and @b. These are the maximum and
 * minimum values that its edges can take.
 **/
void
y_view_interval_set_bounds (YViewInterval * v, double a, double b)
{
  g_return_if_fail (Y_IS_VIEW_INTERVAL (v));

  g2sort (&a, &b);

  v->min = a;
  v->max = b;
}

/**
 * y_view_interval_clear_bounds :
 * @v: #YViewInterval
 *
 * Reset a #YViewInterval's bounds to the default values, -HUGE_VAL and HUGE_VAL.
 **/
void
y_view_interval_clear_bounds (YViewInterval * v)
{
  g_return_if_fail (Y_IS_VIEW_INTERVAL (v));

  v->min = -HUGE_VAL;
  v->max = HUGE_VAL;
}

/**
 * y_view_interval_set_min_width :
 * @v: #YViewInterval
 * @mw: minimum width
 *
 * Set a #YViewInterval's minimum width.
 **/
void
y_view_interval_set_min_width (YViewInterval * v, double mw)
{
  g_return_if_fail (Y_IS_VIEW_INTERVAL (v));

  v->min_width = mw;
}

/**
 * y_view_interval_valid :
 * @v: #YViewInterval
 * @x: a value to test
 *
 * Check whether a number could possibly exist on a view interval. For example,
 * only positive values are valid on a logarithmic view interval.
 *
 * Returns: %TRUE if @x is valid for @v.
 **/
gboolean
y_view_interval_valid (YViewInterval * v, double x)
{
  g_return_val_if_fail (Y_IS_VIEW_INTERVAL (v), FALSE);

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
 * y_view_interval_conv :
 * @v: #YViewInterval
 * @x: a value to convert
 *
 * Convert a double-precision value to the view interval's coordinates
 *
 * Returns: the converted value, which is between 0 and 1 if the value is
 * inside the view interval.
 **/
double
y_view_interval_conv (YViewInterval * v, double x)
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
 * y_view_interval_unconv :
 * @v: #YViewInterval
 * @x: a value to convert
 *
 * Convert a value from the ViewInterval's coordinates (0 to 1)
 *
 * Returns: the value.
 **/
double
y_view_interval_unconv (YViewInterval * v, double x)
{
  double t0, t1;

  g_return_val_if_fail (Y_IS_VIEW_INTERVAL (v), 0);

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
 * y_view_interval_conv_bulk :
 * @v: #YViewInterval
 * @in_data: values to convert
 * @out_data: output array of values
 * @N: length of arrays
 *
 * Convert an array of double-precision values to the ViewInterval's
 * coordinates.
 **/
void
y_view_interval_conv_bulk (YViewInterval * v,
			   const double *in_data, double *out_data, gsize N)
{
  double t0, t1, tsize, x, y = 0, c = 0;
  gsize i;
  gint type;

  g_return_if_fail (Y_IS_VIEW_INTERVAL (v));
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
 * y_view_interval_unconv_bulk :
 * @v: #YViewInterval
 * @in_data: values to convert
 * @out_data: output array of values
 * @N: length of arrays
 *
 * Convert an array from the ViewInterval's coordinates to data coordinates.
 **/
void
y_view_interval_unconv_bulk (YViewInterval * v,
			     const double *in_data, double *out_data, gsize N)
{
  double t0, t1, x, y = 0, c = 0;
  gsize i;
  gint type;

  g_return_if_fail (Y_IS_VIEW_INTERVAL (v));
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
 * y_view_interval_rescale_around_point :
 * @v: #YViewInterval
 * @x: point
 * @s: scaling factor
 *
 * Scale @v by factor @s and center it around @x.
 **/
void
y_view_interval_rescale_around_point (YViewInterval * v, double x, double s)
{
  double a, b;

  g_return_if_fail (Y_IS_VIEW_INTERVAL (v));

  if (s < 0)
    s = -s;

  if (s != 1)
    {

      x = y_view_interval_conv (v, x);

      /* I do this to be explicit: we are transforming the conv-coordinate
         edge-points of the interval. */
      a = s * (0 - x) + x;
      b = s * (1 - x) + x;

      a = y_view_interval_unconv (v, a);
      b = y_view_interval_unconv (v, b);

      y_view_interval_set (v, a, b);
    }
}

/**
 * y_view_interval_recenter_around_point :
 * @v: #YViewInterval
 * @x: point
 *
 * Center @v around a new point @x without changing its width.
 **/
void
y_view_interval_recenter_around_point (YViewInterval * v, double x)
{
  double a, b, c;
  g_return_if_fail (Y_IS_VIEW_INTERVAL (v));

  y_view_interval_range (v, &a, &b);
  c = (a + b) / 2;
  if (c != x)
    y_view_interval_translate (v, x - c);
}

/**
 * y_view_interval_translate :
 * @v: #YViewInterval
 * @dx: point
 *
 * Move interval by @dx without changing its width.
 **/
void
y_view_interval_translate (YViewInterval * v, double dx)
{
  double a, b;

  g_return_if_fail (Y_IS_VIEW_INTERVAL (v));

  y_view_interval_range (v, &a, &b);

  if (dx != 0 && v->min <= a + dx && b + dx <= v->max)
    {
      y_view_interval_set (v, a + dx, b + dx);
    }
}

/* move interval by dx without changing its width */
void
y_view_interval_conv_translate (YViewInterval * v, double x)
{
  double a, b;

  g_return_if_fail (Y_IS_VIEW_INTERVAL (v));

  if (x == 0.0)
    return;

  a = x;
  b = 1 + x;

  if (!(y_view_interval_is_logarithmic (v) && v->t0 <= 0))
    {

      a = y_view_interval_unconv (v, a);

    }
  else
    {

      a = v->t0;

    }

  b = y_view_interval_unconv (v, b);

  g2sort (&a, &b);

  if (v->min <= a && b <= v->max)
    y_view_interval_set (v, a, b);
}

/**
 * y_view_interval_request_preferred_range :
 * @v: #YViewInterval
 *
 * Causes the #YViewInterval to emit a signal that will cause connected
 * views to set the view interval to their preferred range.
 **/
void
y_view_interval_request_preferred_range (YViewInterval * v)
{
  double p0, p1;

  g_return_if_fail (Y_IS_VIEW_INTERVAL (v));

  if (v->ignore_preferred)
    return;

  p0 = v->t0;
  p1 = v->t1;

  v->block_changed_signals = TRUE;

  v->t0 = 0;
  v->t1 = -1;

  g_signal_emit (v, gvi_signals[PREFERRED_RANGE_REQUEST], 0);

  if (v->t0 > v->t1)
    y_view_interval_set (v, -0.05, 1.05);

  v->block_changed_signals = FALSE;

  if (v->t0 != p0 || v->t1 != p1)
    changed (v);
}

/**
 * y_view_interval_set_ignore_preferred_range :
 * @v: #YViewInterval
 * @ignore: a boolean
 *
 * Whether the #YViewInterval should ignore connected views' preferred ranges.
 **/
void
y_view_interval_set_ignore_preferred_range (YViewInterval * v,
					    gboolean ignore)
{
  v->ignore_preferred = ignore;
  if (!ignore)
    {
      y_view_interval_request_preferred_range (v);
    }
}

/**
 * y_view_interval_get_ignore_preferred_range :
 * @v: #YViewInterval
 *
 * Return whether the #YViewInterval is ignoring connected views' preferred ranges.
 **/
gboolean y_view_interval_get_ignore_preferred_range (YViewInterval *v)
{
	return v->ignore_preferred;
}

/**************************************************************************/

/**
 * y_view_interval_scale_linearly :
 * @v: #YViewInterval
 *
 * Use a linear scale for @v.
 **/
void
y_view_interval_scale_linearly (YViewInterval * v)
{
  g_return_if_fail (Y_IS_VIEW_INTERVAL (v));

  if (v->type != VIEW_NORMAL)
    {
      v->type = VIEW_NORMAL;
      changed (v);
    }
}

/**
 * y_view_interval_scale_logarithmically :
 * @v: #YViewInterval
 * @base: the base, currently not used
 *
 * Use a logarithmic scale for @v, with base @base. The base isn't currently used.
 **/
void
y_view_interval_scale_logarithmically (YViewInterval * v, double base)
{
  g_return_if_fail (Y_IS_VIEW_INTERVAL (v));

  if (v->type != VIEW_LOG)
    {
      v->type = VIEW_LOG;
      v->type_arg = base;
      changed (v);
    }
}

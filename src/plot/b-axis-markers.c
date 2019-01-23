/*
 * y-axis-markers.c
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

#include "plot/y-axis-markers.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

/**
 * SECTION: y-axis-markers
 * @short_description: Object for managing axis markers.
 *
 * This is used to control axis markers.
 */

static GObjectClass *parent_class = NULL;

enum
{
  CHANGED,
  LAST_SIGNAL
};

static guint gam_signals[LAST_SIGNAL] = { 0 };

struct _YAxisMarkers {
  GObject parent;

  gint N, pool;
  YTick *ticks;

  gboolean sorted;

  gint freeze_count;
  gboolean pending;

  /* A hack to prevent multiple recalculation. */
  double pos_min, pos_max;
  gint goal, radix;
};

static void clear (YAxisMarkers *);

static void
y_axis_markers_finalize (GObject * obj)
{
  YAxisMarkers *gal = Y_AXIS_MARKERS (obj);

  clear (gal);
  g_free (gal->ticks);

  if (parent_class->finalize)
    parent_class->finalize (obj);
}

static void
y_axis_markers_class_init (YAxisMarkersClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = y_axis_markers_finalize;

  gam_signals[CHANGED] =
    g_signal_new ("changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  0,
		  NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
y_axis_markers_init (YAxisMarkers * obj)
{

}

G_DEFINE_TYPE (YAxisMarkers, y_axis_markers, G_TYPE_OBJECT);

/**
 * y_axis_markers_new:
 *
 * Create a #YAxisMarkers object.
 *
 * Returns: the new object
 **/
YAxisMarkers *
y_axis_markers_new (void)
{
  return Y_AXIS_MARKERS (g_object_new (y_axis_markers_get_type (), NULL));
}

/**************************************************************************/

static void
changed (YAxisMarkers * gam)
{
  g_return_if_fail (gam != NULL);

  if (gam->freeze_count)
    gam->pending = TRUE;
  else
    g_signal_emit (gam, gam_signals[CHANGED], 0);
}

static void
clear (YAxisMarkers * gam)
{
  gint i;

  g_return_if_fail (gam != NULL);

  for (i = 0; i < gam->N; ++i)
    {
      g_free (gam->ticks[i].label);
      gam->ticks[i].label = NULL;
    }
  gam->N = 0;
}

/**
 * y_axis_markers_freeze:
 * @am: a #YAxisMarkers
 *
 * Increment the freeze count of @am, preventing it from updating.
 **/
void
y_axis_markers_freeze (YAxisMarkers * am)
{
  g_return_if_fail (am != NULL);
  ++am->freeze_count;
}

/**
 * y_axis_markers_thaw:
 * @am: a #YAxisMarkers
 *
 * Reduce the freeze count of @am. When the freeze count reaches 0, @am will
 * respond to signals again.
 **/
void
y_axis_markers_thaw (YAxisMarkers * am)
{
  g_return_if_fail (am != NULL);
  g_return_if_fail (am->freeze_count > 0);
  --am->freeze_count;
  if (am->freeze_count == 0 && am->pending)
    {
      changed (am);
      am->pending = FALSE;
    }
}

/**
 * y_axis_markers_size:
 * @am: a #YAxisMarkers
 *
 * Get the number of axis markers in @am.
 *
 * Returns: the number of axis markers
 **/
gint
y_axis_markers_size (YAxisMarkers * am)
{
  g_return_val_if_fail (am != NULL, 0);

  return am->N;
}

/**
 * y_axis_markers_get:
 * @am: a #YAxisMarkers
 * @i: an integer
 *
 * Get a tick from @am.
 *
 * Returns: the tick
 **/
const YTick *
y_axis_markers_get (YAxisMarkers * am, gint i)
{
  g_return_val_if_fail (am != NULL, NULL);
  g_return_val_if_fail (i >= 0, NULL);
  g_return_val_if_fail (i < am->N, NULL);

  return &am->ticks[i];
}

/**
 * y_axis_markers_clear:
 * @am: a #YAxisMarkers
 *
 * Clear all ticks from @am.
 **/
void
y_axis_markers_clear (YAxisMarkers * am)
{
  g_return_if_fail (am != NULL);
  clear (am);
  changed (am);
}

/**
 * y_axis_markers_add:
 * @am: a #YAxisMarkers
 * @pos: the position, in plot coordinates
 * @type: the type of tick to add
 * @label: the label to show next to the tick
 *
 * Add a tick to @am.
 **/
void
y_axis_markers_add (YAxisMarkers * am,
		    double pos, gint type, const gchar * label)
{
  g_return_if_fail (am != NULL);

  if (am->N == am->pool)
    {
      gint new_size = MAX (2 * am->pool, 32);
      YTick *tmp = g_new0 (YTick, new_size);
      if (am->ticks)
        memcpy (tmp, am->ticks, sizeof (YTick) * am->N);
      g_free (am->ticks);
      am->ticks = tmp;
      am->pool = new_size;
    }

  g_assert (am->ticks != NULL);

  am->ticks[am->N].position = pos;
  am->ticks[am->N].type = type;
  if (label != NULL)
    am->ticks[am->N].label = g_strdup (label);
  else
    am->ticks[am->N].label = NULL;

  ++am->N;

  am->sorted = FALSE;

  changed (am);
}

/* Stupid copy & modify */
void
y_axis_markers_add_critical (YAxisMarkers * am,
			     double pos, gint type, const gchar * label)
{
  g_return_if_fail (am != NULL);

  if (am->N == am->pool)
    {
      gint new_size = MAX (2 * am->pool, 32);
      YTick *tmp = g_new0 (YTick, new_size);
      if (am->ticks)
        memcpy (tmp, am->ticks, sizeof (YTick) * am->N);
      g_free (am->ticks);
      am->ticks = tmp;
      am->pool = new_size;
    }

  am->ticks[am->N].position = pos;
  am->ticks[am->N].type = type;
  am->ticks[am->N].label = g_strdup (label);
  am->ticks[am->N].critical_label = TRUE;

  ++am->N;

  am->sorted = FALSE;

  changed (am);
}

static gint
y_tick_compare (const void *a, const void *b)
{
  const YTick *ta = a;
  const YTick *tb = b;
  return (ta->position > tb->position) - (ta->position < tb->position);
}

/**
 * y_axis_markers_sort:
 * @am: a #YAxisMarkers
 *
 * Sort the ticks in @am by their position.
 **/
void
y_axis_markers_sort (YAxisMarkers * am)
{
  g_return_if_fail (Y_IS_AXIS_MARKERS (am));

  if (am->sorted)
    return;

  qsort (am->ticks, am->N, sizeof (YTick), y_tick_compare);

  am->sorted = TRUE;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static const double base4_divisors[] = { 4, 2, 1, -1 };
static const double base8_divisors[] = { 8, 4, 2, 1, -1 };
static const double base10_divisors[] = { 10, 5, 4, 2, 1, -1 };
static const double base16_divisors[] = { 16, 8, 4, 2, 1, -1 };
static const double base32_divisors[] = { 32, 16, 8, 4, 2, 1, -1 };
static const double base64_divisors[] = { 64, 32, 16, 8, 4, 2, 1, -1 };

/**
 * y_axis_markers_populate_scalar:
 * @am: a #YAxisMarkers
 * @pos_min: the minimum position, in plot coordinates
 * @pos_max: the maximum position, in plot coordinates
 * @goal: the number of ticks desired
 * @radix: ??
 * @percentage: whether to include a percent sign in the label
 *
 * Automatically populate @am with ticks, assuming that the view interval
 * scales linearly.
 **/
void
y_axis_markers_populate_scalar (YAxisMarkers * am,
				double pos_min, double pos_max,
				gint goal, gint radix, gboolean percentage)
{
  double width, mag, div, step, start, count, t;
  double delta_best = 1e+8, step_best = 0, start_best = 0;
  gint i, count_best = 0;
  const double *divisors = NULL;
  gchar labelbuf[64];

  g_return_if_fail (am != NULL);
  g_return_if_fail (goal > 1);

  /* Avoid redundant recalculations. */
  if (am->N &&
      pos_min == am->pos_min &&
      pos_max == am->pos_max && goal == am->goal && radix == am->radix)
    return;

  am->pos_min = pos_min;
  am->pos_max = pos_max;
  am->goal = goal;
  am->radix = radix;

  y_axis_markers_freeze (am);

  y_axis_markers_clear (am);

  if (fabs (pos_min - pos_max) < 1e-10)
    {
      y_axis_markers_thaw (am);
      return;
    }

  if (pos_min > pos_max)
    {
      t = pos_min;
      pos_min = pos_max;
      pos_max = t;
    }

  width = fabs (pos_max - pos_min);
  mag = ceil (log (width / goal) / log (radix));

  switch (radix)
    {
    case 4:
      divisors = base4_divisors;
      break;
    case 8:
      divisors = base8_divisors;
      break;
    case 10:
      divisors = base10_divisors;
      break;
    case 16:
      divisors = base16_divisors;
      break;
    case 32:
      divisors = base32_divisors;
      break;
    case 64:
      divisors = base64_divisors;
      break;
    default:
      g_assert_not_reached ();
    }

  g_assert (divisors != NULL);

  for (i = 0; divisors[i] > 0; ++i)
    {
      div = divisors[i];
      step = pow (radix, mag) / div;
      start = ceil (pos_min / step) * step;
      count = floor (width / step);
      if (pos_min <= start && start <= pos_max)
        ++count;

      if (fabs (count - goal) < delta_best)
        {
          delta_best = fabs (count - goal);
          step_best = step;
          start_best = start;
          count_best = (gint) count;
        }
    }

  if (step_best <= 0)
    {
      return;
      //g_error ("Search for nice axis points failed.  This shouldn't happen.");
    }

  for (i = -1; i <= count_best; ++i)
    {
      double x;
      t = start_best + i * step_best;
      if (fabs (t / step_best) < 1e-12)
        t = 0;

      if (percentage)
      {
        g_snprintf (labelbuf, 64, "%g%%", t * 100);
      }
      else
      {
        g_snprintf (labelbuf, 64, "%g", t);
      }

      if (pos_min <= t && t <= pos_max)
      {
        y_axis_markers_add (am, t, Y_TICK_MAJOR, labelbuf);
        y_axis_markers_add (am, t,
			      t ==
			      0 ? Y_TICK_MAJOR_RULE :
			      Y_TICK_MINOR_RULE, NULL);
      }
#if 0
      /* Add some minor/micro ticks & rules just for fun... */
      x = t + step_best / 4;
      if (pos_min <= x && x <= pos_max)
        y_axis_markers_add (gam, x, Y_TICK_MICRO, NULL);
#endif
      x = t + step_best / 2;
      if (pos_min <= x && x <= pos_max)
      {
        y_axis_markers_add (am, x, Y_TICK_MINOR, NULL);
#if 0
        y_axis_markers_add (gam, x, Y_TICK_MICRO_RULE, NULL);
#endif
      }
#if 0
      x = t + 3 * step_best / 4;
      if (pos_min <= x && x <= pos_max)
        y_axis_markers_add (gam, x, Y_TICK_MICRO, NULL);
#endif
    }

  y_axis_markers_thaw (am);
}

/**
 * y_axis_markers_populate_scalar_log:
 * @am: a #YAxisMarkers
 * @min: the minimum position, in plot coordinates
 * @max: the maximum position, in plot coordinates
 * @goal: the number of ticks desired
 * @base: the base to use for the logarithm
 *
 * Automatically populate @am with ticks, assuming that the view interval
 * scales logarithmically.
 **/
void
y_axis_markers_populate_scalar_log (YAxisMarkers * gam,
				    double min, double max,
				    gint goal, double base)
{
  double minexp, maxexp;
  gint g, i, botexp, topexp, expstep, count = 0;
  gchar labelbuf[64];

  g_return_if_fail (gam != NULL);
  g_return_if_fail (Y_IS_AXIS_MARKERS (gam));
  g_return_if_fail (min < max);
  g_return_if_fail (goal > 0);
  g_return_if_fail (base > 0);

  if (max / min < base)
    {
      y_axis_markers_populate_scalar (gam, min, max, goal, base, FALSE);
      return;
    }

  y_axis_markers_freeze (gam);
  y_axis_markers_clear (gam);

  minexp = log (min) / log (base);
  maxexp = log (max) / log (base);

  botexp = floor (minexp);
  topexp = ceil (maxexp);

  expstep = 0;
  g = goal;
  while (g > 0 && expstep == 0)
    {
      expstep = (gint) rint ((maxexp - minexp) / g);
      --g;
    }

  if (expstep == 0)
    expstep = 1;

  if (expstep)
    {
      for (i = topexp; i >= botexp - 2; i -= expstep)
	{
	  double t = pow (base, i);
	  double ta = pow (base, i + expstep);
	  double x;

	  if (min <= t && t <= max)
	    {
	      g_snprintf (labelbuf, 64, "%g", t);
	      y_axis_markers_add (gam, t, Y_TICK_MAJOR, labelbuf);
	      y_axis_markers_add (gam, t, Y_TICK_MINOR_RULE, NULL);
	      ++count;
	    }

	  x = (ta + t) / 2;
	  if (min <= x && x <= max)
	    {
	      y_axis_markers_add (gam, x, Y_TICK_MINOR, NULL);
	      y_axis_markers_add (gam, x, Y_TICK_MICRO_RULE, NULL);
	    }

	  x = ta / 4 + 3 * t / 4;
	  if (min <= x && x <= max)
	    y_axis_markers_add (gam, x, Y_TICK_MICRO, NULL);

	  x = 3 * ta / 4 + t / 4;
	  if (min <= x && x <= max)
	    y_axis_markers_add (gam, x, Y_TICK_MICRO, NULL);
	}
    }

  if (count < 2)
    {
      y_axis_markers_populate_scalar (gam, min, max,
				      goal > 4 ? goal - 2 : 3, 10, FALSE);
    }

  y_axis_markers_thaw (gam);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

#if 0
static void
populate_dates_daily (YAxisMarkers * gam, GDate * min, GDate * max)
{
  gchar buf[32];
  GDate dt = *min;

  while (g_date_lteq (&dt, max))
    {

      g_date_strftime (buf, 32, "%d %b %y", &dt);

      y_axis_markers_add (gam, g_date_get_julian (&dt), Y_TICK_MAJOR, buf);

      g_date_add_days (&dt, 1);
    }
}

static void
populate_dates_weekly (YAxisMarkers * gam, GDate * min, GDate * max)
{
  gchar buf[32];
  GDate dt = *min;

  while (g_date_get_weekday (&dt) != G_DATE_MONDAY)
    g_date_add_days (&dt, 1);

  while (g_date_lteq (&dt, max))
    {

      if (g_date_get_weekday (&dt) == G_DATE_MONDAY)
	{
	  g_date_strftime (buf, 32, "%d %b %y", &dt);
	  y_axis_markers_add (gam, g_date_get_julian (&dt), Y_TICK_MAJOR,
			      buf);
	}
      else
	{
	  y_axis_markers_add (gam, g_date_get_julian (&dt), Y_TICK_MICRO, "");
	}

      g_date_add_days (&dt, 1);
    }

}

static void
populate_dates_monthly (YAxisMarkers * gam, GDate * min, GDate * max)
{
  gchar buf[32];
  GDate dt, dt2;
  gint j, j2;

  g_date_set_dmy (&dt, 1, g_date_get_month (min), g_date_get_year (min));

  while (g_date_lteq (&dt, max))
    {
      dt2 = dt;
      g_date_add_months (&dt2, 1);
      j = g_date_get_julian (&dt);
      j2 = g_date_get_julian (&dt2);

      g_date_strftime (buf, 32, "%b-%y", &dt);

      y_axis_markers_add (gam, j, Y_TICK_MAJOR, "");
      y_axis_markers_add (gam, (j + j2) / 2.0, Y_TICK_NONE, buf);

      dt = dt2;
    }
}

static void
populate_dates_quarterly (YAxisMarkers * gam, GDate * min, GDate * max)
{
  gchar monthname[32], buf[32];
  GDate dt, dt2;
  gint j, j2;

  g_date_set_dmy (&dt, 1, g_date_get_month (min), g_date_get_year (min));

  while (g_date_lteq (&dt, max))
    {

      dt2 = dt;
      g_date_add_months (&dt2, 1);
      j = g_date_get_julian (&dt);
      j2 = g_date_get_julian (&dt2);

      g_date_strftime (monthname, 32, "%b", &dt);
      g_snprintf (buf, 32, "%c-%02d", *monthname,
		  g_date_get_year (&dt) % 100);

      if (g_date_get_month (&dt) % 3 == 1)
	y_axis_markers_add (gam, j, Y_TICK_MAJOR, "");
      else
	y_axis_markers_add (gam, j, Y_TICK_MINOR, "");

      y_axis_markers_add (gam, (j + j2) / 2.0, Y_TICK_NONE, buf);

      dt = dt2;
    }

}

static void
populate_dates_yearly (YAxisMarkers * gam, GDate * min, GDate * max)
{
  gchar buf[32];
  GDate dt, dt2;
  gint j, j2, y;
  gint count = 0, step = 1;
  gboolean two_digit_years;

  g_date_set_dmy (&dt, 1, 1, g_date_get_year (min));
  while (g_date_lteq (&dt, max))
    {
      g_date_add_years (&dt, 1);
      ++count;
    }

  two_digit_years = count > 5;
  if (count > 10)
    step = 2;
  if (count > 20)
    step = 5;

  g_date_set_dmy (&dt, 1, 1, g_date_get_year (min));
  while (g_date_lteq (&dt, max))
    {
      dt2 = dt;
      g_date_add_years (&dt2, 1);
      j = g_date_get_julian (&dt);
      j2 = g_date_get_julian (&dt2);
      y = g_date_get_year (&dt);

      if (two_digit_years)
	{
	  if (step == 1 || y % step == 0)
	    g_snprintf (buf, 32, "%02d", y % 100);
	  else
	    *buf = '\0';
	}
      else
	{
	  g_snprintf (buf, 32, "%d", y);
	}

      y_axis_markers_add (gam, j, Y_TICK_MAJOR, "");
      if (*buf)
	y_axis_markers_add (gam, (j + j2) / 2.0, Y_TICK_NONE, buf);

      if (step == 1)
	{
	  y_axis_markers_add (gam, j + 0.25 * (j2 - j), Y_TICK_MICRO, "");
	  y_axis_markers_add (gam, (j + j2) / 2.0, Y_TICK_MICRO, "");
	  y_axis_markers_add (gam, j + 0.75 * (j2 - j), Y_TICK_MICRO, "");
	}

      dt = dt2;
    }
}

void
y_axis_markers_populate_dates (YAxisMarkers * gam, GDate * min, GDate * max)
{
  gint jspan;

  g_return_if_fail (gam && Y_IS_AXIS_MARKERS (gam));
  g_return_if_fail (min && g_date_valid (min));
  g_return_if_fail (max && g_date_valid (max));

  jspan = g_date_get_julian (max) - g_date_get_julian (min);

  y_axis_markers_freeze (gam);
  y_axis_markers_clear (gam);

  if (jspan < 2 * 7)
    populate_dates_daily (gam, min, max);
  else if (jspan < 8 * 7)
    populate_dates_weekly (gam, min, max);
  else if (jspan < 8 * 30)
    populate_dates_monthly (gam, min, max);
  else if (jspan < 6 * 90)
    populate_dates_quarterly (gam, min, max);
  else
    populate_dates_yearly (gam, min, max);

  y_axis_markers_thaw (gam);
}
#endif

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static inline void
y_2sort (double *a, double *b)
{
  double t;

  if (a == NULL || b == NULL)
    return;

  if (*a > *b)
    {
      t = *a;
      *a = *b;
      *b = t;
    }
}

/**
 * y_axis_markers_populate_generic:
 * @am: a #YAxisMarkers
 * @type: the axis type
 * @min: the low edge of the view interval
 * @max: the high edge of the view interval
 *
 * Automatically populate @am with ticks, using aesthetically pleasing defaults
 * for the number of ticks.
 **/
void
y_axis_markers_populate_generic (YAxisMarkers * am,
				 gint type, double min, double max)
{
  g_return_if_fail (am && Y_IS_AXIS_MARKERS (am));

  y_2sort (&min, &max);

  switch (type)
    {

    case Y_AXIS_SCALAR:
      y_axis_markers_populate_scalar (am, min, max, 6, 10, FALSE);
      break;

    case Y_AXIS_SCALAR_LOG2:
      y_axis_markers_populate_scalar_log (am, min, max, 6, 2.0);
      break;

    case Y_AXIS_SCALAR_LOG10:
      y_axis_markers_populate_scalar_log (am, min, max, 6, 10);
      break;

    case Y_AXIS_PERCENTAGE:
      y_axis_markers_populate_scalar (am, min, max, 6, 10, TRUE);
      break;

#if 0
    case Y_AXIS_DATE:
      {
	gint ja, jb;
	GDate dt_a, dt_b;

	ja = (gint) floor (a);
	jb = (gint) ceil (b);

	if (ja <= 0 || jb <= 0)
	  return;

	if (!(g_date_valid_julian (ja) && g_date_valid_julian (jb)))
	  return;

	g_date_set_julian (&dt_a, ja);
	g_date_set_julian (&dt_b, jb);

	y_axis_markers_populate_dates (gam, &dt_a, &dt_b);
      }
      break;
#endif
    default:
      g_assert_not_reached ();

    }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

#if 0
void
y_axis_markers_max_label_size (YAxisMarkers * gam, GnomeFont * f,
			       gboolean consider_major,
			       gboolean consider_minor,
			       gboolean consider_micro,
			       double *max_w, double *max_h)
{
  gint i;
  const YTick *tick;

  g_return_if_fail (gam != NULL);
  g_return_if_fail (f != NULL);

  if (max_w == NULL && max_h == NULL)
    return;

  /* Font height is independent of the string */
  if (max_h)
    *max_h = gnome_font_get_ascender (f) + gnome_font_get_descender (f);


  if (max_w)
    {
      *max_w = 0;

      for (i = 0; i < y_axis_markers_size (gam); ++i)
	{

	  tick = y_axis_markers_get (gam, i);

	  if (y_tick_is_labelled (tick) &&
	      ((consider_major && y_tick_is_major (tick)) ||
	       (consider_minor && y_tick_is_minor (tick)) ||
	       (consider_micro && y_tick_is_micro (tick))))
	    {

	      if (max_w)
		{
		  double w =
		    gnome_font_get_width_utf8 (f, y_tick_label (tick));
		  *max_w = MAX (*max_w, w);
		}
	    }
	}
    }
}
#endif

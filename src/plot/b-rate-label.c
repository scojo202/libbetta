/*
 * b-rate-label.c
 *
 * Copyright (C) 2017 Scott O. Johnson (scojo202@gmail.com)
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

#include "plot/b-rate-label.h"

/**
 * SECTION: b-rate-label
 * @short_description: Widget for displaying a rate, e.g. frames per second.
 *
 * This is a label used to display a rate.
 */

struct _BRateLabel
{
  GtkLabel parent_instance;
  GTimer *timer;
  gdouble last_stop;
  gdouble rate;
  char i;
  gchar *text;
  gchar *suffix;
  BData *source;
  gulong handler;
};

G_DEFINE_TYPE (BRateLabel, b_rate_label, GTK_TYPE_LABEL)

static void b_rate_label_finalize (GObject * obj)
{
  BRateLabel *self = B_RATE_LABEL (obj);

  if (self->source != NULL)
    {
      g_signal_handler_disconnect (self->source, self->handler);
      g_object_unref (self->source);
    }

  g_timer_destroy (self->timer);
  if (self->text)
    g_free (self->text);
  if (self->suffix)
    g_free (self->suffix);

  if (G_OBJECT_CLASS (b_rate_label_parent_class)->finalize)
    G_OBJECT_CLASS (b_rate_label_parent_class)->finalize (obj);
}

static void
on_source_changed (BData * data, gpointer user_data)
{
  BRateLabel *f = (BRateLabel *) user_data;
  b_rate_label_update (f);
}

static void
b_rate_label_class_init (BRateLabelClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->finalize = b_rate_label_finalize;
}

static void
b_rate_label_init (BRateLabel * self)
{
  self->timer = g_timer_new ();
  g_timer_start (self->timer);
  self->last_stop = g_timer_elapsed (self->timer, NULL);
  self->i = 0;
}

/**
 * b_rate_label_new:
 * @text: label string
 * @suffix: suffix string
 *
 * Create a new #BRateLabel.
 *
 * Returns: the widget.
 **/
BRateLabel *
b_rate_label_new (const gchar * text, const gchar * suffix)
{
  BRateLabel *w =
    g_object_new (B_TYPE_RATE_LABEL, "wrap", TRUE, "width-request", 64,
		  "margin", 2, NULL);

  if (text)
    w->text = g_strdup (text);
  if (suffix)
    w->suffix = g_strdup (suffix);
  return w;
}

/**
 * b_rate_label_set_source:
 * @f: a #BRateLabel
 * @source: a #BData object
 *
 * Set a source object for #BRateLabel. The frame rate will reflect the rate
 * that "changed" signals are generated.
 **/
void
b_rate_label_set_source (BRateLabel * f, BData * source)
{
  g_assert (B_IS_RATE_LABEL (f));
  g_assert (source == NULL || B_IS_DATA (source));
  if (source == f->source)
    return;
  if (f->source != NULL)
    {
      g_signal_handler_disconnect (f->source, f->handler);
      g_object_unref (f->source);
    }
  if (source != NULL)
    {
      f->source = g_object_ref_sink (source);
      f->handler =
        g_signal_connect (f->source, "changed",
            G_CALLBACK (on_source_changed), f);
    }
  else
    {
      f->source = NULL;
      f->handler = 0;
    }
}

/**
 * b_rate_label_update:
 * @f: a #BRateLabel
 *
 * Force an update.
 **/
void
b_rate_label_update (BRateLabel * f)
{
  g_assert (B_IS_RATE_LABEL (f));
  f->i++;
  if (f->i == 4)
    {
      gdouble stop = g_timer_elapsed (f->timer, NULL);
      gchar buff[200];
      f->rate = 4.0 / (stop - f->last_stop);

      sprintf (buff, "%s: %1.2f %s", f->text, f->rate, f->suffix);
      gtk_label_set_text (GTK_LABEL (f), buff);
      f->last_stop = stop;
      f->i = 0;
    }
}

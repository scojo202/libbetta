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

#include <string.h>
#include "plot/b-rate-label.h"

/**
 * SECTION: b-rate-label
 * @short_description: Widget for displaying a rate, e.g. frames per second.
 *
 * This is a label used to display a rate.
 */

static GObjectClass *parent_class = NULL;

struct _BRateLabel
{
  GtkWidget parent_instance;
  GtkLabel *label;
  GTimer *timer;
  double last_stop;
  double rate;
  char i;
  char *text;
  char *suffix;
  char *buffer;
  BData *source;
  gulong handler;
  guint timeout;
  double interval;
};

G_DEFINE_TYPE (BRateLabel, b_rate_label, GTK_TYPE_WIDGET)

static void b_rate_label_finalize (GObject * obj)
{
  BRateLabel *self = B_RATE_LABEL (obj);

  if (self->source != NULL)
    {
      g_signal_handler_disconnect (self->source, self->handler);
      g_object_unref (self->source);
    }

  if(self->timeout)
    g_source_remove(self->timeout);

  g_timer_destroy (self->timer);
  if (self->text)
    g_free (self->text);
  if (self->suffix)
    g_free (self->suffix);
  g_free (self->buffer);

  if (G_OBJECT_CLASS (b_rate_label_parent_class)->finalize)
    G_OBJECT_CLASS (b_rate_label_parent_class)->finalize (obj);
}

static void
b_rate_label_dispose (GObject *obj)
{
  BRateLabel *rl = (BRateLabel *) obj;

  gtk_widget_unparent(GTK_WIDGET(rl->label));

  if (parent_class->dispose)
    parent_class->dispose (obj);
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
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = b_rate_label_finalize;
  object_class->dispose = b_rate_label_dispose;
}

static void
b_rate_label_init (BRateLabel * self)
{
  GtkLayoutManager *man = gtk_bin_layout_new();
  gtk_widget_set_layout_manager(GTK_WIDGET(self),man);

  self->timer = g_timer_new ();
  g_timer_start (self->timer);
  self->label = GTK_LABEL(gtk_label_new(""));
  gtk_widget_insert_before(GTK_WIDGET(self->label),GTK_WIDGET(self),NULL);

  self->last_stop = g_timer_elapsed (self->timer, NULL);
  self->i = 0;
  self->interval = 5.0;
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
    g_object_new (B_TYPE_RATE_LABEL, NULL);

  if (text)
    w->text = g_strdup (text);
  if (suffix)
    w->suffix = g_strdup (suffix);

  int wc = 9+strlen(text)+strlen(suffix);

  if(wc>150) {
    g_warning("BRateLabel: text and suffix too long, truncating.");
    wc=150;
  }

  g_object_set(w->label, "wrap", TRUE, "width-chars", wc, NULL);

  w->buffer = malloc(180);

  g_snprintf (w->buffer, 180, "%s: not running", w->text);
  gtk_label_set_text (w->label, w->buffer);

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
  g_return_if_fail (B_IS_RATE_LABEL (f));
  g_return_if_fail (source == NULL || B_IS_DATA (source));
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

static gboolean
check_timed_out (gpointer user_data)
{
  BRateLabel *l = (BRateLabel *) user_data;
  double stop = g_timer_elapsed (l->timer, NULL);
  if(stop-l->last_stop > l->interval) {
    g_snprintf (l->buffer, 180, "%s: timed out", l->text);
    gtk_label_set_text (GTK_LABEL (l->label), l->buffer);
  }
  return G_SOURCE_CONTINUE;
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
  g_return_if_fail (B_IS_RATE_LABEL (f));
  f->i++;
  if (f->i == 4)
    {
      double stop = g_timer_elapsed (f->timer, NULL);
      f->rate = 4.0 / (stop - f->last_stop);

      g_snprintf (f->buffer, 180, "%s: %1.2f %s", f->text, f->rate, f->suffix);
      gtk_label_set_text (GTK_LABEL (f->label), f->buffer);
      f->last_stop = stop;
      f->i = 0;
    }
  if(f->interval > 0.0 && f->timeout==0)
    f->timeout = g_timeout_add(1000, check_timed_out, f);
}

/**
 * b_rate_label_set_timeout:
 * @f: a #BRateLabel
 * @interval: a timeout interval, in seconds
 *
 * Set the timeout for the rate label. The label will display "timed out" if
 * there are no updates within this time period. A value of @interval less than
 * or equal to zero will disable the timeout.
 **/
void
b_rate_label_set_timeout (BRateLabel * f, double interval)
{
  g_return_if_fail (B_IS_RATE_LABEL (f));
  if(interval<=0.0) {
    f->interval = -1.0;
    g_source_remove(f->timeout);
    f->timeout = 0;
  }
  else {
    f->interval = interval;
  }
}


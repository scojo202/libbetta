/*
 * y-rate-label.c
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

#include "y-rate-label.h"

struct _YRateLabel
{
  GtkLabel parent_instance;
  GTimer *timer;
  gdouble last_stop;
  gdouble rate;
  char i;
  gchar * text;
  gchar * suffix;
  YData * source;
  gulong handler;
};

G_DEFINE_TYPE (YRateLabel, y_rate_label, GTK_TYPE_LABEL)

static
void y_rate_label_finalize(GObject *obj)
{
  YRateLabel *self = Y_RATE_LABEL(obj);

  if(self->source!=NULL) {
    g_signal_handler_disconnect(self->source,self->handler);
    g_object_unref(self->source);
  }
  
  g_timer_destroy(self->timer);
  g_free(self->text);
  g_free(self->suffix);
  
  if (G_OBJECT_CLASS(y_rate_label_parent_class)->finalize)
    G_OBJECT_CLASS(y_rate_label_parent_class)->finalize (obj);
}

static
void on_source_changed(YData *data, gpointer user_data)
{
  YRateLabel *f = (YRateLabel*) user_data;
  y_rate_label_update(f);
}

static void
y_rate_label_class_init (YRateLabelClass *klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;
  
  object_class->finalize = y_rate_label_finalize;
}

static void
y_rate_label_init (YRateLabel *self)
{
  self->timer = g_timer_new();
  g_timer_start(self->timer);
  self->last_stop = g_timer_elapsed(self->timer,NULL);
  self->i = 0;
}

YRateLabel * y_rate_label_new(const gchar * text, const gchar *suffix)
{
  YRateLabel *w = g_object_new(Y_TYPE_RATE_LABEL,"wrap",TRUE,"width-request",64,"margin",2,NULL);
  
  w->text = g_strdup(text);
  w->suffix = g_strdup(suffix);
  return w;
}

void y_rate_label_set_source(YRateLabel *f, YData *source)
{
  g_assert(Y_IS_RATE_LABEL(f));
  g_assert(source==NULL || Y_IS_DATA(source));
  if(source==f->source) return;
  if(f->source!=NULL) {
    g_signal_handler_disconnect(f->source,f->handler);
    g_object_unref(f->source);
  }
  if(source!=NULL) {
    f->source = g_object_ref_sink(source);
    f->handler = g_signal_connect(f->source,"changed",G_CALLBACK(on_source_changed),f);
  }
  else {
    f->source = NULL;
    f->handler = 0;
  }
}

void y_rate_label_update(YRateLabel *f)
{
  g_assert(Y_IS_RATE_LABEL(f));
  f->i++;
  if(f->i==4) {
    gdouble stop = g_timer_elapsed(f->timer,NULL);
    gchar buff[200];
    f->rate = 4.0/(stop-f->last_stop);
  
    sprintf(buff,"%s: %1.2f %s",f->text,f->rate,f->suffix);
    gtk_label_set_text(GTK_LABEL(f),buff);
    f->last_stop=stop;
    f->i=0;
  }
}


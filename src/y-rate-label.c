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
};

G_DEFINE_TYPE (YRateLabel, y_rate_label, GTK_TYPE_LABEL)

static
void y_rate_label_finalize(GObject *obj)
{
  YRateLabel *self = Y_RATE_LABEL(obj);
  
  g_timer_destroy(self->timer);
  g_free(self->text);
  
  if (G_OBJECT_CLASS(y_rate_label_parent_class)->finalize)
    G_OBJECT_CLASS(y_rate_label_parent_class)->finalize (obj);
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

void y_rate_label_update(YRateLabel *f)
{
  f->i++;
  if(f->i==4) {
    gdouble stop = g_timer_elapsed(f->timer,NULL);
    gchar buff[400];
    f->rate = 4.0/(stop-f->last_stop);
  
    sprintf(buff,"%s: %1.2f %s",f->text,f->rate,f->suffix);
    gtk_label_set_text(GTK_LABEL(f),buff);
    f->last_stop=stop;
    f->i=0;
  }
}


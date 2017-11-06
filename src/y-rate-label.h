#include <gtk/gtk.h>

#ifndef __Y_RATE_LABEL_H__
#define __Y_RATE_LABEL_H__

G_DECLARE_FINAL_TYPE (YRateLabel, y_rate_label, Y, RATE_LABEL, GtkLabel)

#define Y_TYPE_RATE_LABEL                  (y_rate_label_get_type ())

YRateLabel * y_rate_label_new(const gchar * text, const gchar *suffix);
void y_rate_label_update(YRateLabel *f);

#endif

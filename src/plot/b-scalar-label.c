/*
 * b-scalar-label.c
 *
 * Copyright (C) 2019 Scott O. Johnson (scojo202@gmail.com)
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

#include "plot/b-scalar-label.h"

/**
 * SECTION: b-scalar-label
 * @short_description: Widget for displaying a scalar.
 *
 * This is a label used to display a scalar value.
 */

struct _BScalarLabel
{
  GtkLabel parent_instance;
  BScalar *source;
	gulong handler;
	GString *str;
  gchar *format;
  gchar *prefix;
};

G_DEFINE_TYPE (BScalarLabel, b_scalar_label, GTK_TYPE_LABEL)

static void b_scalar_label_finalize (GObject * obj)
{
  BScalarLabel *self = B_SCALAR_LABEL (obj);

  if (self->source != NULL)
    {
      g_signal_handler_disconnect (self->source, self->handler);
      g_object_unref (self->source);
    }

  if (self->format)
    g_free (self->format);
  if (self->prefix)
    g_free (self->prefix);

  if (G_OBJECT_CLASS (b_scalar_label_parent_class)->finalize)
    G_OBJECT_CLASS (b_scalar_label_parent_class)->finalize (obj);
}

static void
on_source_changed (BData * data, gpointer user_data)
{
  BScalarLabel *f = (BScalarLabel *) user_data;
  g_string_printf(f->str,f->format,b_scalar_get_value(B_SCALAR(data)));
  gtk_label_set_text(GTK_LABEL(f),f->str->str);
}

static void
b_scalar_label_class_init (BScalarLabelClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->finalize = b_scalar_label_finalize;
}

static void
b_scalar_label_init (BScalarLabel * self)
{
  self->format = g_strdup("1.3f");
}

/**
 * b_scalar_label_new:
 * @format: format string
 * @prefix: prefix string
 *
 * Create a new #BScalarLabel.
 *
 * Returns: the widget.
 **/
BScalarLabel *
b_scalar_label_new (const gchar * format, const gchar * prefix)
{
  BScalarLabel *w =
    g_object_new (B_TYPE_SCALAR_LABEL, "wrap", TRUE, "width-request", 64,
		  "margin", 2, NULL);

  if (format)
    w->format = g_strndup (format, 20);
  if (prefix)
    w->prefix = g_strndup (prefix, 10);
  return w;
}

/**
 * b_scalar_label_set_source:
 * @f: a #BScalarLabel
 * @source: a #BData object
 *
 * Set a source object for #BScalarLabel.
 **/
void
b_scalar_label_set_source (BScalarLabel * f, BScalar * source)
{
  g_return_if_fail (B_IS_SCALAR_LABEL (f));
  g_return_if_fail (source == NULL || B_IS_SCALAR (source));
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

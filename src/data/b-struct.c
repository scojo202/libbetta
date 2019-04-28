/*
 * b-struct.c :
 *
 * Copyright (C) 2016,2018 Scott O. Johnson (scojo202@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include "b-struct.h"

/**
 * SECTION: b-struct
 * @short_description: A dictionary containing data objects.
 *
 * A data object that can contain other data objects.
 */

typedef struct
{
  GHashTable *hash;
} BStructPrivate;

enum
{
  CHANGED_SUBDATA,
  LAST_STRUCT_SIGNAL
};
static guint struct_signals[LAST_STRUCT_SIGNAL] = { 0 };

/**
 * BStruct:
 *
 * Object representing a dictionary full of BData objects.
 **/

G_DEFINE_TYPE_WITH_PRIVATE (BStruct, b_struct, B_TYPE_DATA);

static void
b_struct_finalize (GObject * obj)
{
  BStruct *s = (BStruct *) obj;
  BStructPrivate *priv = b_struct_get_instance_private (s);
  g_hash_table_unref (priv->hash);

  GObjectClass *obj_class = G_OBJECT_CLASS (b_struct_parent_class);

  (*obj_class->finalize) (obj);
}

static void
disconnect (gpointer key, gpointer value, gpointer user_data)
{
  BStruct *s = (BStruct *) user_data;
  if (value != NULL)
    {
      g_signal_handlers_disconnect_by_data (value, s);
    }
}

static void
b_struct_dispose (GObject * obj)
{
  BStruct *s = (BStruct *) obj;
  BStructPrivate *priv = b_struct_get_instance_private (s);
  g_hash_table_foreach (priv->hash, disconnect, s);

  GObjectClass *obj_class = G_OBJECT_CLASS (b_struct_parent_class);

  (*obj_class->dispose) (obj);
}

static char
_struct_get_sizes (BData * data, unsigned int *sizes)
{
  return -1;
}

static void
b_struct_class_init (BStructClass * s_klass)
{
  BDataClass *ydata_klass = (BDataClass *) s_klass;
  GObjectClass *gobject_klass = (GObjectClass *) s_klass;

  struct_signals[CHANGED_SUBDATA] =
    g_signal_new ("subdata-changed",
		  G_TYPE_FROM_CLASS (s_klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BStructClass, subdata_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1, G_TYPE_POINTER);

  gobject_klass->finalize = b_struct_finalize;
  gobject_klass->dispose = b_struct_dispose;
  ydata_klass->get_sizes = _struct_get_sizes;
}

static void
unref_if_not_null (gpointer object)
{
  if (object != NULL)
    {
      g_object_unref (object);
    }
}

static void
b_struct_init (BStruct * s)
{
  BStructPrivate *priv = b_struct_get_instance_private (s);
  priv->hash =
    g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
			   unref_if_not_null);
}

/**
 * b_struct_get_data :
 * @s: #BStruct
 * @name: string id
 *
 * Get a data object from the struct.
 *
 * Returns: (transfer none): the data.
 **/
BData *
b_struct_get_data (BStruct * s, const gchar * name)
{
  BStructPrivate *priv = b_struct_get_instance_private (s);
  return g_hash_table_lookup (priv->hash, name);
}

static void
on_subdata_changed (BData * d, gpointer user_data)
{
  BStruct *s = B_STRUCT (user_data);
  g_signal_emit (G_OBJECT (s), struct_signals[CHANGED_SUBDATA], 0, d);
}

/**
 * b_struct_set_data :
 * @s: #BStruct
 * @name: string id
 * @d: (transfer full): #BData
 *
 * Set a data object.
 **/
void
b_struct_set_data (BStruct * s, const gchar * name, BData * d)
{
  BStructPrivate *priv = b_struct_get_instance_private (s);
  BData *od = B_DATA (g_hash_table_lookup (priv->hash, name));
  if (od != NULL)
    {
      g_signal_handlers_disconnect_by_data (od, s);
    }
  if (d == NULL)
    {
      g_hash_table_insert (priv->hash, g_strdup (name), NULL);
    }
  else
    {
      g_return_if_fail (B_IS_DATA (d));
      g_hash_table_insert (priv->hash, g_strdup (name),
			   g_object_ref_sink (d));
      g_signal_connect (d, "changed", G_CALLBACK (on_subdata_changed), s);
    }
  b_data_emit_changed (B_DATA (s));
}

/**
 * b_struct_foreach :
 * @s: #BStruct
 * @f: (scope call): #GHFunc
 * @user_data: user data
 *
 * Set a data object.
 **/
void
b_struct_foreach (BStruct * s, GHFunc f, gpointer user_data)
{
  BStructPrivate *priv = b_struct_get_instance_private (s);
  g_hash_table_foreach (priv->hash, f, user_data);
}

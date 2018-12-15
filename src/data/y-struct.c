/*
 * y-struct.c :
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

#include "y-struct.h"

/**
 * SECTION: y-struct
 * @short_description: A dictionary containing data objects.
 *
 * A data object that can contain other data objects.
 */

typedef struct {
	GHashTable *hash;
} YStructPrivate;

enum {
  CHANGED_SUBDATA,
  LAST_STRUCT_SIGNAL
};
static guint struct_signals[LAST_STRUCT_SIGNAL] = { 0 };

/**
 * YStruct:
 *
 * Object representing a dictionary full of YData objects.
 **/

G_DEFINE_TYPE_WITH_PRIVATE(YStruct, y_struct, Y_TYPE_DATA);

static void y_struct_finalize(GObject *obj)
{
	YStruct *s = (YStruct *) obj;
	YStructPrivate *priv = y_struct_get_instance_private(s);
	g_hash_table_unref(priv->hash);

	GObjectClass *obj_class = G_OBJECT_CLASS(y_struct_parent_class);

	(*obj_class->finalize) (obj);
}

static void disconnect(gpointer key, gpointer value, gpointer user_data)
{
	YStruct *s = (YStruct *)user_data;
	if(value!=NULL) {
		g_signal_handlers_disconnect_by_data(value,s);
	}
}

static void y_struct_dispose(GObject * obj)
{
	YStruct *s = (YStruct *)obj;
	YStructPrivate *priv = y_struct_get_instance_private(s);
	g_hash_table_foreach(priv->hash,disconnect,s);

	GObjectClass *obj_class = G_OBJECT_CLASS(y_struct_parent_class);

	(*obj_class->dispose) (obj);
}

static char _struct_get_sizes(YData * data, unsigned int *sizes)
{
	return -1;
}

static void y_struct_class_init(YStructClass * s_klass)
{
	YDataClass *ydata_klass = (YDataClass *) s_klass;
	GObjectClass *gobject_klass = (GObjectClass *) s_klass;

	struct_signals[CHANGED_SUBDATA] =
    g_signal_new ("subdata-changed",
		  G_TYPE_FROM_CLASS (s_klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (YStructClass, subdata_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__POINTER,
		  G_TYPE_NONE, 1, G_TYPE_POINTER);

	gobject_klass->finalize = y_struct_finalize;
	gobject_klass->dispose  = y_struct_dispose;
	ydata_klass->get_sizes  = _struct_get_sizes;
}

static void
unref_if_not_null (gpointer object)
{
	if(object!=NULL) {
		g_object_unref(object);
	}
}

static void y_struct_init(YStruct * s)
{
	YStructPrivate *priv = y_struct_get_instance_private(s);
	priv->hash =
	    g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
				unref_if_not_null);
}

/**
 * y_struct_get_data :
 * @s: #YStruct
 * @name: string id
 *
 * Get a data object from the struct.
 *
 * Returns: (transfer none): the data.
 **/
YData *y_struct_get_data(YStruct * s, const gchar * name)
{
	YStructPrivate *priv = y_struct_get_instance_private(s);
	return g_hash_table_lookup(priv->hash, name);
}

static
void on_subdata_changed(YData *d, gpointer user_data)
{
	YStruct *s = Y_STRUCT(user_data);
	g_signal_emit(G_OBJECT(s), struct_signals[CHANGED_SUBDATA], 0, d);
}

/**
 * y_struct_set_data :
 * @s: #YStruct
 * @name: string id
 * @d: (transfer full): #YData
 *
 * Set a data object.
 **/
void y_struct_set_data(YStruct * s, const gchar * name, YData * d)
{
	YStructPrivate *priv = y_struct_get_instance_private(s);
	YData *od = Y_DATA(g_hash_table_lookup(priv->hash, name));
	if(od!=NULL) {
		g_signal_handlers_disconnect_by_data(od,s);
	}
	if(d==NULL) {
		g_hash_table_insert(priv->hash, g_strdup(name), NULL);
	}
	else {
		g_assert(Y_IS_DATA(d));
		g_hash_table_insert(priv->hash, g_strdup(name), g_object_ref_sink(d));
		g_signal_connect(d, "changed",
				 G_CALLBACK(on_subdata_changed), s);
	}
	y_data_emit_changed(Y_DATA(s));
}

/**
 * y_struct_foreach :
 * @s: #YStruct
 * @f: (scope call): #GHFunc
 * @user_data: user data
 *
 * Set a data object.
 **/
void y_struct_foreach(YStruct * s, GHFunc f, gpointer user_data)
{
	YStructPrivate *priv = y_struct_get_instance_private(s);
	g_hash_table_foreach(priv->hash, f, user_data);
}


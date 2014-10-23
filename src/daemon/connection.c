/**
 * This file is part of Loom.
 *
 * Copyright (C) 2014 Tobias Schäfer <tschaefer@blackox.org>.
 *
 * Loom is free software: you can redistribute it and*or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Loom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Loom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gsystem-local-alloc.h"

#include <glib/gi18n.h>

#include "daemon.h"
#include "interface.h"
#include "setting.h"
#include "tools.h"
#include "connection.h"

typedef struct _ConnectionClass ConnectionClass;

struct _Connection
{
  LoomConnectionSkeleton parent_instance;
  Daemon *daemon;
  Interface *interface;
  Setting *setting;
  gchar *id;
};

struct _ConnectionClass
{
  LoomConnectionSkeletonClass parent_class;
};

enum
{
  PROP_0,
  PROP_DAEMON,
  PROP_INTERFACE_OBJECT,
  PROP_SETTING_OBJECT,
  PROP_OBJECT_PATH,
  PROP_ID,
};

static void connection_iface_init (LoomConnectionIface *iface);

G_DEFINE_TYPE_WITH_CODE (Connection, connection,
                         LOOM_TYPE_CONNECTION_SKELETON,
                         G_IMPLEMENT_INTERFACE (LOOM_TYPE_CONNECTION,
                                                connection_iface_init));

static void
connection_init (Connection *connection)
{
}

static void
connection_finalize (GObject *object)
{
  Connection *connection = CONNECTION (object);

  g_free (connection->id);

  G_OBJECT_CLASS (connection_parent_class)->finalize (object);
}

static void
connection_set_property (GObject *object,
                         guint prop_id,
                         const GValue *value,
                         GParamSpec *pspec)
{
  Connection *connection = CONNECTION (object);

  switch (prop_id)
    {
    case PROP_DAEMON:
      g_assert (connection->daemon == NULL);
      connection->daemon = g_value_get_object (value);
      break;

    case PROP_INTERFACE_OBJECT:
      g_assert (connection->interface == NULL);
      connection->interface = g_value_get_object (value);
      break;

    case PROP_SETTING_OBJECT:
      g_assert (connection->setting == NULL);
      connection->setting = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
connection_get_property (GObject *object,
                         guint prop_id,
                         GValue *value,
                         GParamSpec *pspec)
{
  Connection *connection = CONNECTION (object);

  switch (prop_id)
    {
    case PROP_OBJECT_PATH:
      g_value_set_string (value, connection_get_object_path (connection));
      break;

    case PROP_ID:
      g_value_set_string (value, connection_get_id (connection));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

void
connection_unexport (Connection *connection)
{
  g_return_if_fail (IS_CONNECTION (connection));

  GDBusObjectManagerServer *object_manager;
  GDBusObject *object;

  object_manager = daemon_get_object_manager (connection->daemon);

  object = g_dbus_interface_get_object (G_DBUS_INTERFACE (connection));
  if (object != NULL)
    g_dbus_object_manager_server_unexport (object_manager,
                                        g_dbus_object_get_object_path (object));
}

void
connection_export (Connection *connection)
{
  g_return_if_fail (IS_CONNECTION (connection));

  GDBusObjectManagerServer *object_manager;
  object_manager = daemon_get_object_manager (connection->daemon);

  if (g_dbus_interface_get_object (G_DBUS_INTERFACE (connection)) == NULL)
    {
      LoomObjectSkeleton *object = NULL;

      object = loom_object_skeleton_new ("/org/blackox/Loom/Connection");
      loom_object_skeleton_set_connection (object,
                                           LOOM_CONNECTION (connection));
      g_dbus_object_manager_server_export_uniquely (object_manager,
                                               G_DBUS_OBJECT_SKELETON (object));
      g_object_unref (object);
    }
}

static void
connection_constructed (GObject *object)
{
  Connection *connection = CONNECTION (object);

  connection->id = g_strjoin ("%",
                              setting_get_uuid (connection->setting),
                              interface_get_name (connection->interface),
                              NULL);

  loom_connection_set_interface (LOOM_CONNECTION (connection),
                             interface_get_object_path (connection->interface));
  loom_connection_set_setting (LOOM_CONNECTION (connection),
                               setting_get_object_path (connection->setting));

  if (G_OBJECT_CLASS (connection_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (connection_parent_class)->constructed (object);
}

static void
connection_class_init (ConnectionClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = connection_finalize;
  gobject_class->constructed = connection_constructed;
  gobject_class->set_property = connection_set_property;
  gobject_class->get_property = connection_get_property;

  g_object_class_install_property (gobject_class,
                                   PROP_DAEMON,
                                   g_param_spec_object ("daemon",
                                                        NULL,
                                                        NULL,
                                                        TYPE_DAEMON,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_INTERFACE_OBJECT,
                                   g_param_spec_object ("interface-object",
                                                        NULL,
                                                        NULL,
                                                        TYPE_INTERFACE,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_SETTING_OBJECT,
                                   g_param_spec_object ("setting-object",
                                                        NULL,
                                                        NULL,
                                                        TYPE_SETTING,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_OBJECT_PATH,
                                   g_param_spec_string ("object-path",
                                                        NULL,
                                                        NULL,
                                                        "",
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_OBJECT_PATH,
                                   g_param_spec_string ("id",
                                                        NULL,
                                                        NULL,
                                                        "",
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS));
}

LoomConnection *
connection_new (Daemon *daemon,
                Interface *interface,
                Setting *setting)
{
  g_return_val_if_fail (IS_DAEMON (daemon), NULL);
  g_return_val_if_fail (IS_INTERFACE (interface), NULL);
  g_return_val_if_fail (IS_SETTING (setting), NULL);

  return LOOM_CONNECTION (g_object_new (TYPE_CONNECTION,
                                        "daemon", daemon,
                                        "interface-object", interface,
                                        "setting-object", setting,
                                        NULL));
}

const gchar *
connection_get_object_path (Connection *connection)
{
  g_return_val_if_fail (IS_CONNECTION (connection), NULL);
  GDBusObject *object = NULL;
  object = g_dbus_interface_get_object (G_DBUS_INTERFACE (connection));
  return g_dbus_object_get_object_path (object);
}

const gchar *
connection_get_id (Connection *connection)
{
  g_return_val_if_fail (IS_CONNECTION (connection), NULL);
  return connection->id;
}

void
connection_add (Connection *connection)
{
  g_return_if_fail (IS_CONNECTION (connection));

  GVariantDict *dict;
  GVariant *value;
  GVariant *configuration;
  const gchar *address;

  configuration = setting_get_configuration (connection->setting);
  dict = g_variant_dict_new (configuration);

  value = g_variant_dict_lookup_value (dict, "address", G_VARIANT_TYPE_STRING);
  address = g_variant_get_string (value, NULL);

  interface_set_up (connection->interface);
  interface_add_address (connection->interface, address);

  if (g_variant_dict_contains (dict, "router"))
    {
      value = g_variant_dict_lookup_value (dict, "router",
                                           G_VARIANT_TYPE_STRING);
      tools_add_router_address (g_variant_get_string (value, NULL));
    }

  if (g_variant_dict_contains (dict, "nameservers"))
    {
      gs_strfreev gchar **nameservers = NULL;
      const gchar *domain = NULL;
      gs_strfreev gchar **searches = NULL;

      value = g_variant_dict_lookup_value (dict, "nameservers",
                                           G_VARIANT_TYPE_STRING_ARRAY);
      nameservers = g_variant_dup_strv (value, NULL);

      if (g_variant_dict_contains (dict, "domain"))
        {
          value = g_variant_dict_lookup_value (dict, "domain",
                                               G_VARIANT_TYPE_STRING);
          domain = g_variant_get_string (value, NULL);
        }

      if (g_variant_dict_contains (dict, "searches"))
        {
          value = g_variant_dict_lookup_value (dict, "searches",
                                               G_VARIANT_TYPE_STRING_ARRAY);
          searches = g_variant_dup_strv (value, NULL);
        }

      tools_write_resolver_configuration ((const gchar * const *)nameservers,
                                          domain,
                                          (const gchar * const *)searches);
    }

  g_variant_dict_unref (dict);
}

void
connection_delete (Connection *connection)
{
  g_return_if_fail (IS_CONNECTION (connection));

  GVariantDict *dict;
  GVariant *value;
  GVariant *configuration;
  const gchar *address;

  configuration = setting_get_configuration (connection->setting);
  dict = g_variant_dict_new (configuration);

  value = g_variant_dict_lookup_value (dict, "address", G_VARIANT_TYPE_STRING);
  address = g_variant_get_string (value, NULL);

  interface_set_down (connection->interface);
  interface_delete_address (connection->interface, address);

  if (g_variant_dict_contains (dict, "router"))
    {
      value = g_variant_dict_lookup_value (dict, "router",
                                           G_VARIANT_TYPE_STRING);
      tools_delete_router_address (g_variant_get_string (value, NULL));
    }

  if (g_variant_dict_contains (dict, "nameservers"))
    tools_erase_resolver_configuration (NULL, NULL, NULL);

  g_variant_dict_unref (dict);
}

Interface *
connection_get_interface(Connection *connection)
{
  g_return_val_if_fail (IS_CONNECTION (connection), NULL);

  return connection->interface;
}

Setting *
connection_get_setting(Connection *connection)
{
  g_return_val_if_fail (IS_CONNECTION (connection), NULL);

  return connection->setting;
}

static void
connection_iface_init (LoomConnectionIface *iface)
{
}

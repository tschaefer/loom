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
#include "interfaces.h"
#include "interface.h"
#include "settings.h"
#include "setting.h"
#include "connections.h"
#include "connection.h"

/**
 * SECTION: Connections
 * @title: Connections
 * @short_description: Implementation of #LoomConnections for interface
 * management.
 *
 * This type provides an implementation of the #LoomConnections interface.
 */

typedef struct _ConnectionsClass ConnectionsClass;

/**
 * Connections:
 *
 * The #Connections structure contains only private data and should only be
 * accessed using the provided API.
 */
struct _Connections
{
  LoomConnectionsSkeleton parent_instance;
  Daemon *daemon;
  Interfaces *interfaces;
  Settings *settings;
  GHashTable *connections;
};

struct _ConnectionsClass
{
  LoomConnectionsSkeletonClass parent_class;
};

enum
{
  PROP_0,
  PROP_DAEMON,
  PROP_INTERFACES,
  PROP_SETTINGS,
};

static void connections_iface_init (LoomConnectionsIface *iface);

G_DEFINE_TYPE_WITH_CODE (Connections, connections,
                         LOOM_TYPE_CONNECTIONS_SKELETON,
                         G_IMPLEMENT_INTERFACE (LOOM_TYPE_CONNECTIONS,
                                                connections_iface_init));

static void
connections_init (Connections *connections)
{
  connections->connections = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                    NULL, g_object_unref);
}

static void
connections_finalize (GObject *object)
{
  Connections *connections = CONNECTIONS (object);

  g_hash_table_unref (connections->connections);

  G_OBJECT_CLASS (connections_parent_class)->finalize (object);
}

static void
connections_set_property (GObject *object,
                      guint prop_id,
                      const GValue *value,
                      GParamSpec *pspec)
{
  Connections *connections = CONNECTIONS (object);

  switch (prop_id)
    {
    case PROP_DAEMON:
      g_assert (connections->daemon == NULL);
      connections->daemon = g_value_get_object (value);
      break;

    case PROP_INTERFACES:
      g_assert (connections->interfaces == NULL);
      connections->interfaces = g_value_get_object (value);
      break;

    case PROP_SETTINGS:
      g_assert (connections->settings == NULL);
      connections->settings = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static Connections *connections_instance;

static void
connections_constructed (GObject *object)
{
  Connections *connections = CONNECTIONS (object);

  g_assert (connections_instance == NULL);
  connections_instance = connections;

  if (G_OBJECT_CLASS (connections_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (connections_parent_class)->constructed (object);
}

static void
connections_class_init (ConnectionsClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = connections_finalize;
  gobject_class->constructed = connections_constructed;
  gobject_class->set_property = connections_set_property;

  /**
   * Connections:daemon:
   *
   * The #Daemon for the object.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_DAEMON,
                                   g_param_spec_object ("daemon",
                                                        NULL,
                                                        NULL,
                                                        TYPE_DAEMON,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * Connections:interfaces:
   *
   * The #Interfaces manager.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_INTERFACES,
                                   g_param_spec_object ("interfaces",
                                                        NULL,
                                                        NULL,
                                                        TYPE_INTERFACES,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * Connections:settings:
   *
   * The #Settings manager.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SETTINGS,
                                   g_param_spec_object ("settings",
                                                        NULL,
                                                        NULL,
                                                        TYPE_SETTINGS,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

}

/**
 * connections_new:
 * @daemon: A #Daemon.
 * @interfaces: A #Interfaces.
 * @settings: A #Settings.
 *
 * Creates a new #Connections instance.
 *
 * Returns: A new #Connections. Free with g_object_unref().
 */
LoomConnections *
connections_new (Daemon *daemon,
                 Interfaces *interfaces,
                 Settings *settings)
{
  g_return_val_if_fail (IS_DAEMON (daemon), NULL);
  g_return_val_if_fail (IS_INTERFACES (interfaces), NULL);
  g_return_val_if_fail (IS_SETTINGS (settings), NULL);

  return LOOM_CONNECTIONS (g_object_new (TYPE_CONNECTIONS,
                                     "daemon", daemon,
                                     "interfaces", interfaces,
                                     "settings", settings,
                                     NULL));
}

/**
 * connections_get_connection_by_object_path:
 * @connections: A #Connections.
 * @object_path: A D-Bus object-path.
 *
 * Gets a #Connection by its D-Bus object-path.
 *
 * Returns: A #Connection object. Do not free, the object is owned by
 * @settings.
 */
Connection *
connections_get_by_object_path (Connections *connections,
                                const gchar* object_path)
{
  g_return_val_if_fail (IS_CONNECTIONS (connections), NULL);
  g_return_val_if_fail (g_variant_is_object_path (object_path), NULL);

  return g_hash_table_lookup (connections->connections, object_path);
}

static gboolean
handle_create (LoomConnections *object,
               GDBusMethodInvocation *invocation,
               const gchar *arg_interface,
               const gchar *arg_setting)
{
  Connections *connections = CONNECTIONS (object);

  Interface *interface;
  Setting *setting;
  Connection *connection;
  gs_free gchar **object_paths = NULL;
  gs_free gchar *connection_id = NULL;
  GError *error = NULL;
  GHashTableIter iter;
  gpointer key, value;
  gboolean exists = FALSE;

  interface = interfaces_get_by_object_path (connections->interfaces,
                                             arg_interface);
  setting = settings_get_by_object_path (connections->settings, arg_setting);

  if (interface == NULL)
    {
      error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                           _("no such 'interface' object found"));
      g_dbus_method_invocation_take_error (invocation, error);

      return TRUE;
    }
  if (setting == NULL)
    {
      error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                           _("no such 'setting' object found"));
      g_dbus_method_invocation_take_error (invocation, error);

      return TRUE;
    }

  connection_id = g_strjoin ("%", setting_get_uuid (setting),
                             interface_get_name (interface), NULL);
  g_hash_table_iter_init (&iter, connections->connections);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      if (g_str_equal (connection_id, connection_get_id ((Connection *)value)))
        {
          exists = TRUE;
          break;
        }
    }
  if (exists)
    {
      error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                           _("'connection' object already exists"));
      g_dbus_method_invocation_take_error (invocation, error);

      return TRUE;
    }

  connection = CONNECTION (connection_new (connections->daemon,
                                           interface, setting));
  connection_export (connection);
  g_hash_table_insert (connections->connections,
                       (gchar *)connection_get_object_path (connection),
                       connection);

  object_paths =
    (gchar **)g_hash_table_get_keys_as_array (connections->connections,
                                              NULL);
  loom_connections_set_connections (object, (const gchar* const*)object_paths);

  loom_connections_complete_create (object, invocation,
                                    connection_get_object_path (connection));

  return TRUE;
}

static gboolean
handle_destroy (LoomConnections *object,
               GDBusMethodInvocation *invocation,
               const gchar *arg_connection)
{
  Connections *connections = CONNECTIONS (object);

  GError *error = NULL;
  const gchar * const *active_connections;
  gs_free gchar **object_paths = NULL;

  if (!g_hash_table_contains (connections->connections, arg_connection))
    {
      error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                           _("no such 'connection' object found"));
      g_dbus_method_invocation_take_error (invocation, error);

      return TRUE;
    }

  active_connections = loom_connections_get_active_connections (object);
  if (active_connections != NULL)
    {
      for (guint i = 0; active_connections[i] != NULL; i++)
        {
          if (g_str_equal (arg_connection, active_connections[i]))
            {
              error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                   _("'connection' object is active"));
              g_dbus_method_invocation_take_error (invocation, error);

              return TRUE;
            }
        }
    }

  g_hash_table_remove (connections->connections, arg_connection);

  object_paths =
    (gchar **)g_hash_table_get_keys_as_array (connections->connections,
                                              NULL);
  loom_connections_set_connections (object, (const gchar * const *)object_paths);

  loom_connections_complete_destroy (object, invocation);

  return TRUE;
}

static gboolean
handle_add (LoomConnections *object,
                GDBusMethodInvocation *invocation,
                const gchar *arg_connection)
{
  Connections *connections = CONNECTIONS (object);

  GError *error = NULL;
  const gchar * const *active_connections;
  gs_unref_ptrarray GPtrArray *_active_connections = NULL;
  Connection *connection;
  const gchar *connection_id;
  gs_strfreev gchar **connection_id_tokens = NULL;
  Interface *interface;
  Setting *setting;

  connection = connections_get_by_object_path (connections, arg_connection);
  if (connection == NULL)
    {
      error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                           _("no such 'connection' object found"));
      g_dbus_method_invocation_take_error (invocation, error);

      return TRUE;
    }

  connection_id = connection_get_id (connection);
  connection_id_tokens = g_strsplit (connection_id, "%", 2);

  _active_connections = g_ptr_array_new ();

  active_connections = loom_connections_get_active_connections (object);
  if (active_connections != NULL)
    {
      active_connections = loom_connections_get_active_connections (object);
      for (guint i = 0; active_connections[i] != NULL; i++)
        {
          if (g_str_equal (arg_connection, active_connections[i]))
            {
              error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                   _("'connection' object already in use"));
              g_dbus_method_invocation_take_error (invocation, error);

              return TRUE;
            }
          if (g_str_has_prefix (active_connections[i], connection_id_tokens[1]))
            {
              error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                   _("'interface' object already in use"));
              g_dbus_method_invocation_take_error (invocation, error);

              return TRUE;
            }
          g_ptr_array_add (_active_connections, (gpointer)active_connections[i]);
        }
    }

  connection_add (connection);

  g_ptr_array_add (_active_connections, (gpointer)arg_connection);
  g_ptr_array_add (_active_connections, NULL);
  loom_connections_set_active_connections (object,
                             (const gchar * const *)_active_connections->pdata);

  interface = connection_get_interface (connection);
  interfaces_add_to_actives (connections->interfaces, interface);

  setting = connection_get_setting (connection);
  settings_add_to_actives (connections->settings, setting);

  loom_connections_complete_add (object, invocation);

  return TRUE;
}

static gboolean
handle_delete (LoomConnections *object,
                GDBusMethodInvocation *invocation,
                const gchar *arg_connection)
{
  Connections *connections = CONNECTIONS (object);

  GError *error = NULL;
  Connection *connection;
  const gchar * const *active_connections;
  gs_unref_ptrarray GPtrArray *_active_connections;
  gboolean active = FALSE;
  Interface *interface;
  Setting *setting;

  connection = connections_get_by_object_path (connections, arg_connection);

  if (connection == NULL)
    {
      error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                           _("no such 'connection' object found"));
      g_dbus_method_invocation_take_error (invocation, error);

      return TRUE;
    }

  _active_connections = g_ptr_array_new ();

  active_connections =
  (const gchar * const *)loom_connections_get_active_connections (object);
  if (active_connections != NULL)
    {
      for (guint i = 0; active_connections[i] != NULL; i++)
        {
          if (!g_str_equal (active_connections[i], arg_connection))
            {
              g_ptr_array_add (_active_connections,
                               (gpointer)active_connections[i]);
            }
          else
            {
              active = TRUE;
            }
        }
    }
  else
    {
      error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                           _("no 'connection' objects active"));
      g_dbus_method_invocation_take_error (invocation, error);

      return TRUE;
    }

  if (!active)
    {
      error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                           _("'connection' object is not active"));
      g_dbus_method_invocation_take_error (invocation, error);

      return TRUE;
    }

  connection_delete (connection);

  loom_connections_set_active_connections (object,
                             (const gchar * const *)_active_connections->pdata);

  interface = connection_get_interface (connection);
  interfaces_remove_from_actives (connections->interfaces, interface);

  setting = connection_get_setting (connection);
  settings_remove_from_actives (connections->settings, setting);

  loom_connections_complete_delete (object, invocation);

  return TRUE;
}

static void
connections_iface_init (LoomConnectionsIface *iface)
{
  iface->handle_create = handle_create;
  iface->handle_destroy = handle_destroy;
  iface->handle_add = handle_add;
  iface->handle_delete = handle_delete;
}

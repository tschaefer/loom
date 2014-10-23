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

#include "daemon.h"
#include "interfaces.h"
#include "settings.h"
#include "connections.h"

/**
 * SECTION: Daemon
 * @title: Daemon
 * @short_description: Main daemon object
 *
 * Object handling all global state.
 */

typedef struct _DaemonClass DaemonClass;

/**
 * Daemon:
 *
 * The #Daemon structure contains only private data and should only be
 * accessed using the provided API.
 */
struct _Daemon
{
  GObject parent_instance;
  GDBusConnection *connection;
  GDBusObjectManagerServer *object_manager;

  Interfaces *interfaces;
  Settings *settings;
  Connections *connections;
  guint tick_timeout_id;
  gint64 last_tick;
};

struct _DaemonClass
{
  GObjectClass parent_class;

  void (*tick) (Daemon *daemon,
                guint64 delta_usec);
};

enum
{
  PROP_0,
  PROP_CONNECTION,
  PROP_OBJECT_MANAGER,
};

enum
{
  TICK_SIGNAL,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(Daemon, daemon, G_TYPE_OBJECT);

static void
daemon_finalize (GObject *object)
{
  Daemon *daemon = DAEMON (object);

  g_object_unref (daemon->connection);
  g_object_unref (daemon->object_manager);
  g_object_unref (daemon->interfaces);
  g_object_unref (daemon->settings);
  g_object_unref (daemon->connections);

  if (daemon->tick_timeout_id > 0)
    g_source_remove (daemon->tick_timeout_id);

  if (G_OBJECT_CLASS (daemon_parent_class)->finalize != NULL)
    G_OBJECT_CLASS (daemon_parent_class)->finalize (object);
}

static void
daemon_get_property (GObject *object,
                     guint prop_id,
                     GValue *value,
                     GParamSpec *pspec)
{
  Daemon *daemon = DAEMON (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_object (value, daemon_get_connection (daemon));
      break;

    case PROP_OBJECT_MANAGER:
      g_value_set_object (value, daemon_get_object_manager (daemon));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
daemon_set_property (GObject *object,
                     guint prop_id,
                     const GValue *value,
                     GParamSpec *pspec)
{
  Daemon *daemon = DAEMON (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_assert (daemon->connection == NULL);
      daemon->connection = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
daemon_init (Daemon *daemon)
{
}

static gboolean
on_timeout (gpointer user_data)
{
  Daemon *daemon = DAEMON (user_data);
  guint64 delta_usec = 0;
  gint64 now;

  now = g_get_monotonic_time ();
  if (daemon->last_tick != 0)
    delta_usec = now - daemon->last_tick;
  daemon->last_tick = now;

  g_signal_emit (daemon, signals[TICK_SIGNAL], 0, delta_usec);

  return TRUE;
}

static Daemon *daemon_instance;

static void
daemon_constructed (GObject *_object)
{
  Daemon *daemon = DAEMON (_object);
  LoomInterfaces *interfaces;
  LoomSettings *settings;
  LoomConnections *connections;
  LoomObjectSkeleton *object = NULL;

  g_assert (daemon_instance == NULL);
  daemon_instance = daemon;

  daemon->object_manager = g_dbus_object_manager_server_new ("/org/blackox/Loom");

  /* /org/blackox/Loom/Interfaces */
  interfaces = interfaces_new (daemon);
  daemon->interfaces = INTERFACES (interfaces);
  object = loom_object_skeleton_new ("/org/blackox/Loom/Interfaces");
  loom_object_skeleton_set_interfaces (object, interfaces);
  g_dbus_object_manager_server_export (daemon->object_manager,
                                       G_DBUS_OBJECT_SKELETON (object));
  g_object_unref (object);

  /* /org/blackox/Loom/Settings */
  settings = settings_new (daemon);
  daemon->settings = SETTINGS (settings);
  object = loom_object_skeleton_new ("/org/blackox/Loom/Settings");
  loom_object_skeleton_set_settings (object, settings);
  g_dbus_object_manager_server_export (daemon->object_manager,
                                       G_DBUS_OBJECT_SKELETON (object));
  g_object_unref (object);

  /* /org/blackox/Loom/Connections */
  connections = connections_new (daemon, daemon->interfaces, daemon->settings);
  daemon->connections = CONNECTIONS (connections);
  object = loom_object_skeleton_new ("/org/blackox/Loom/Connections");
  loom_object_skeleton_set_connections (object, connections);
  g_dbus_object_manager_server_export (daemon->object_manager,
                                       G_DBUS_OBJECT_SKELETON (object));
  g_object_unref (object);

  g_dbus_object_manager_server_set_connection (daemon->object_manager,
                                               daemon->connection);

  daemon->tick_timeout_id = g_timeout_add_seconds (1, on_timeout, daemon);

  if (G_OBJECT_CLASS (daemon_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (daemon_parent_class)->constructed (_object);
}

static void
daemon_class_init (DaemonClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = daemon_finalize;
  gobject_class->constructed = daemon_constructed;
  gobject_class->set_property = daemon_set_property;
  gobject_class->get_property = daemon_get_property;

  /**
   * Daemon:connection:
   *
   * The #GDBusConnection the daemon is for.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_CONNECTION,
                                   g_param_spec_object ("connection",
                                                        "Connection",
                                                        "The D-Bus connection the daemon is for.",
                                                        G_TYPE_DBUS_CONNECTION,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * Daemon:object-manager:
   *
   * The #GDBusObjectManager used by the daemon.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_OBJECT_MANAGER,
                                   g_param_spec_object ("object-manager",
                                                        "Object manager",
                                                        "The D-Bus Object Manager server used by the daemon.",
                                                        G_TYPE_DBUS_OBJECT_MANAGER_SERVER,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * Daemon:tick:
   * @daemon: A #Daemon.
   * @delta_usec: The number of micro-seconds since this was last emitted or 0
   * if the first time it's emitted.
   *
   * Emitted every second - subsystems use this signal instead of setting up
   * their own timeout.
   *
   * This signal is emitted in the
   * <link linkend="g-main-context-push-thread-default">thread-default main
   * Loop</link> that @daemon was created in.
   */
  signals[TICK_SIGNAL] = g_signal_new ("tick",
                                       G_OBJECT_CLASS_TYPE (klass),
                                       G_SIGNAL_RUN_LAST,
                                       G_STRUCT_OFFSET (DaemonClass, tick),
                                       NULL,
                                       NULL,
                                       g_cclosure_marshal_generic,
                                       G_TYPE_NONE,
                                       1,
                                       G_TYPE_UINT64);
}

/**
 * daemon_new:
 * @connection: A #GDBusConnection
 *
 * Create a new daemon object for exporting objects on @connection.
 *
 * Returns: A #Daemon object. Free with g_object_unref().
 */
Daemon *
daemon_new (GDBusConnection *connection)
{
  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);
  return DAEMON (g_object_new (TYPE_DAEMON,
                               "connection",
                               connection,
                               NULL));
}

/**
 * daemon_get:
 *
 * Returns: (transfer none): The singelton #Daemon instance.
 */
Daemon *
daemon_get (void)
{
  g_assert (daemon_instance);
  return daemon_instance;
}

/**
 * daemon_get_connection:
 * @daemon: A #Daemon.
 *
 * Gets the D-Bus connection used by @daemon.
 *
 * Returns: A #GDBusConnection. Do not free, the object is owned by @daemon.
 */
GDBusConnection *
daemon_get_connection (Daemon *daemon)
{
  g_return_val_if_fail (IS_DAEMON (daemon), NULL);
  return daemon->connection;
}

/**
 * daemon_get_object_manager:
 * @daemon: A #Daemon,
 *
 * Gets the D-Bus object manager used by @daemon.
 *
 * Returns: A #GDBusObjectManagerServer. Do not free, the object is owned by
 * @daemon.
 */
GDBusObjectManagerServer *
daemon_get_object_manager (Daemon *daemon)
{
  g_return_val_if_fail (IS_DAEMON (daemon), NULL);
  return daemon->object_manager;
}

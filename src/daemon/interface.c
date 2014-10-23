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

#include <string.h>

#include <glib/gi18n.h>

#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/cache.h>
#include <netlink/route/link.h>
#include <netlink/route/addr.h>

#include "gsystem-local-alloc.h"

#include "daemon.h"
#include "interface.h"

/**
 * SECTION: Interface
 * @title: Interface
 * @short_description: Implementation of #LoomInterface for network interface
 * description.
 *
 * This type provides an implementation of the #LoomInterfaces interface.
 */

typedef struct _InterfaceClass InterfaceClass;

/**
 * Interface:
 *
 * The #Interface structure contains only private data and should only be
 * accessed using the provided API.
 */
struct _Interface
{
  LoomInterfaceSkeleton parent_instance;
  Daemon *daemon;
  gchar *name;
};

struct _InterfaceClass
{
  LoomInterfaceSkeletonClass parent_class;
};

enum
{
  PROP_0,
  PROP_DAEMON,
  PROP_NAME,
  PROP_OBJECT_PATH
};

static void interface_iface_init (LoomInterfaceIface *iface);

G_DEFINE_TYPE_WITH_CODE (Interface, interface,
                         LOOM_TYPE_INTERFACE_SKELETON,
                         G_IMPLEMENT_INTERFACE (LOOM_TYPE_INTERFACE,
                                                interface_iface_init));

static void on_tick (Daemon *daemon,
                     guint64 delta_usec,
                     gpointer user_data);

static void
interface_init (Interface *interface)
{
}

static void
interface_finalize (GObject *object)
{
  Interface *interface = INTERFACE (object);

  g_free (interface->name);

  g_signal_handlers_disconnect_by_func (interface->daemon,
                                        G_CALLBACK (on_tick),
                                        interface);

  G_OBJECT_CLASS (interface_parent_class)->finalize (object);
}

static void interface_set_property (GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec)
{
  Interface *interface = INTERFACE (object);

  switch (prop_id)
    {
    case PROP_DAEMON:
      g_assert (interface->daemon == NULL);
      interface->daemon = g_value_get_object (value);
      break;

    case PROP_NAME:
      g_assert (interface->name == NULL);
      interface->name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void interface_get_property (GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec)
{
  Interface *interface = INTERFACE (object);

  switch (prop_id)
    {
    case PROP_OBJECT_PATH:
      g_value_set_string (value, interface_get_object_path (interface));
      break;

    case PROP_NAME:
      g_value_set_string (value, interface_get_name (interface));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

void
interface_export (Interface *interface)
{
  g_return_if_fail (IS_INTERFACE (interface));

  GDBusObjectManagerServer *object_manager;

  object_manager = daemon_get_object_manager (interface->daemon);

  if (g_dbus_interface_get_object (G_DBUS_INTERFACE (interface)) == NULL)
    {
      LoomObjectSkeleton *object = NULL;
      gs_free gchar *object_path = NULL;

      object_path = g_strdup_printf ("/org/blackox/Loom/Interface/%s",
                                     interface->name);
      object = loom_object_skeleton_new (object_path);
      loom_object_skeleton_set_interface (object, LOOM_INTERFACE (interface));
      g_dbus_object_manager_server_export (object_manager,
                                           G_DBUS_OBJECT_SKELETON (object));
      g_object_unref (object);
    }
}

static void
read_link_address (Interface *interface)
{
  struct nl_sock *sock = NULL;
  struct rtnl_link *link = NULL;
  struct nl_addr *addr = NULL;

  gs_free gchar *addr_str = NULL;

  sock = nl_socket_alloc ();
  nl_connect (sock, NETLINK_ROUTE);

  rtnl_link_get_kernel (sock, 0, interface->name, &link);

  if (link == NULL)
    {
      g_warning (_("Error getting link info from kernel."));
      goto out;
    }

  addr = rtnl_link_get_addr (link);
  addr_str = g_malloc0 (17);
  nl_addr2str (addr, addr_str, 17);
  loom_interface_set_address (LOOM_INTERFACE (interface),
                              addr_str);
out:
  rtnl_link_put (link);
  nl_socket_free (sock);
}

static gboolean
read_link_properties (Interface *interface)
{
  LoomInterface *_interface = LOOM_INTERFACE (interface);

  struct nl_sock *sock = NULL;
  struct rtnl_link *link = NULL;

  gboolean changed = FALSE;

  gboolean state;
  gboolean carrier;
  guint8 flags;

  sock = nl_socket_alloc ();
  nl_connect (sock, NETLINK_ROUTE);

  rtnl_link_get_kernel (sock, 0, interface->name, &link);

  if (link == NULL)
    {
      g_warning (_("Error getting link info from kernel."));
      goto out;
    }

  flags = rtnl_link_get_flags (link);
  state = (gboolean) flags & IFF_UP;
  carrier = (gboolean) rtnl_link_get_carrier (link);

  if (loom_interface_get_state (_interface) != state)
    {
      loom_interface_set_state (_interface, state);
      changed = TRUE;
    }

  if (loom_interface_get_carrier (_interface) != carrier)
    {
      loom_interface_set_carrier (_interface, carrier);
      changed = TRUE;
    }

out:
  rtnl_link_put (link);
  nl_socket_free (sock);

  return changed;
}

static void
on_tick (Daemon *daemon,
         guint64 delta_usec,
         gpointer user_data)
{
  Interface *interface = INTERFACE (user_data);

  gboolean changed = FALSE;

  changed |= read_link_properties (interface);

  if (changed)
    loom_interface_emit_changed (LOOM_INTERFACE (interface));
}

static void
interface_constructed (GObject *object)
{
  Interface *interface = INTERFACE (object);

  read_link_address (interface);
  read_link_properties (interface);

  g_signal_connect (interface->daemon, "tick", G_CALLBACK (on_tick), interface);

  if (G_OBJECT_CLASS (interface_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (interface_parent_class)->constructed (object);
}

static void
interface_class_init (InterfaceClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = interface_finalize;
  gobject_class->constructed = interface_constructed;
  gobject_class->set_property = interface_set_property;
  gobject_class->get_property = interface_get_property;

  g_object_class_install_property (gobject_class,
                                   PROP_DAEMON,
                                   g_param_spec_object ("daemon",
                                                        NULL,
                                                        NULL,
                                                        TYPE_DAEMON,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        NULL,
                                                        NULL,
                                                        "",
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
                                   PROP_OBJECT_PATH,
                                   g_param_spec_string ("object-path",
                                                        NULL,
                                                        NULL,
                                                        "",
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS));
}

LoomInterface *
interface_new (Daemon *daemon, gchar *name)
{
  g_return_val_if_fail (IS_DAEMON (daemon), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  return LOOM_INTERFACE (g_object_new (TYPE_INTERFACE,
                                             "daemon", daemon,
                                             "name", name,
                                             NULL));
}

const gchar *
interface_get_object_path (Interface *interface)
{
  g_return_val_if_fail (IS_INTERFACE (interface), NULL);
  GDBusObject *object = NULL;

  object = g_dbus_interface_get_object (G_DBUS_INTERFACE (interface));

  return g_dbus_object_get_object_path (object);
}

const gchar *
interface_get_name (Interface *interface)
{
  g_return_val_if_fail (IS_INTERFACE (interface), NULL);

  return interface->name;
}


void
interface_set_up (Interface *interface)
{
  struct nl_sock *sock = NULL;
  struct rtnl_link *link = NULL;
  struct rtnl_link *change = NULL;

  sock = nl_socket_alloc ();
  nl_connect (sock, NETLINK_ROUTE);

  rtnl_link_get_kernel (sock, 0, interface->name, &link);
  if (link == NULL)
    {
      g_warning (_("Error getting link info from kernel."));
      goto out;
    }

  change = rtnl_link_alloc ();
  rtnl_link_set_flags (change, IFF_UP);

  rtnl_link_change (sock, link, change, 0);

out:
  rtnl_link_put (change);
  rtnl_link_put (link);
  nl_socket_free (sock);
}

void
interface_set_down (Interface *interface)
{
  struct nl_sock *sock = NULL;
  struct rtnl_link *link = NULL;
  struct rtnl_link *change = NULL;

  sock = nl_socket_alloc ();
  nl_connect (sock, NETLINK_ROUTE);

  rtnl_link_get_kernel (sock, 0, interface->name, &link);
  if (link == NULL)
    {
      g_warning (_("Error getting link info from kernel."));
      goto out;
    }

  change = rtnl_link_alloc ();
  rtnl_link_unset_flags (change, IFF_UP);

  rtnl_link_change (sock, link, change, 0);

out:
  rtnl_link_put (change);
  rtnl_link_put (link);
  nl_socket_free (sock);
}

void
interface_add_address (Interface *interface,
                       const gchar *address)
{
  struct nl_sock *sock = NULL;
  struct rtnl_link *link = NULL;
  struct rtnl_addr *addr = NULL;
  struct nl_addr *local = NULL;

  sock = nl_socket_alloc ();
  nl_connect (sock, NETLINK_ROUTE);

  rtnl_link_get_kernel (sock, 0, interface->name, &link);
  if (link == NULL)
    {
      g_warning (_("Error getting link info from kernel."));
      goto out;
    }

  nl_addr_parse (address, AF_INET, &local);

  addr = rtnl_addr_alloc ();
  rtnl_addr_set_ifindex (addr, rtnl_link_get_ifindex (link));
  rtnl_addr_set_family (addr, AF_INET);
  rtnl_addr_set_local (addr, local);

  rtnl_addr_add(sock, addr, 0);

out:
  rtnl_link_put (link);
  rtnl_addr_put (addr);
  nl_addr_put (local);
  nl_socket_free (sock);
}

void
interface_delete_address  (Interface *interface,
                           const gchar *address)
{
  struct nl_sock *sock = NULL;
  struct rtnl_link *link = NULL;
  struct rtnl_addr *addr = NULL;
  struct nl_addr *local = NULL;

  sock = nl_socket_alloc ();
  nl_connect (sock, NETLINK_ROUTE);

  rtnl_link_get_kernel (sock, 0, interface->name, &link);
  if (link == NULL)
    {
      g_warning (_("Error getting link info from kernel."));
      goto out;
    }

  nl_addr_parse (address, AF_INET, &local);

  addr = rtnl_addr_alloc ();
  rtnl_addr_set_ifindex (addr, rtnl_link_get_ifindex (link));
  rtnl_addr_set_family (addr, AF_INET);
  rtnl_addr_set_local (addr, local);

  rtnl_addr_delete(sock, addr, 0);

out:
  rtnl_link_put (link);
  rtnl_addr_put (addr);
  nl_addr_put (local);
  nl_socket_free (sock);

}

static void
interface_iface_init (LoomInterfaceIface *iface)
{
}

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

#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/cache.h>
#include <netlink/route/link.h>

#include "daemon.h"
#include "interface.h"
#include "interfaces.h"

/**
 * SECTION: Interfaces
 * @title: Interfaces
 * @short_description: Implementation of #LoomInterfaces for interface
 * management.
 *
 * This type provides an implementation of the #LoomInterfaces interface.
 */

typedef struct _InterfacesClass InterfacesClass;

/**
 * Interfaces:
 *
 * The #Interfaces structure contains only private data and should only be
 * accessed using the provided API.
 */
struct _Interfaces
{
  LoomInterfacesSkeleton parent_instance;
  Daemon *daemon;
  GHashTable *interfaces;
};

struct _InterfacesClass
{
  LoomInterfacesSkeletonClass parent_class;
};

enum
{
  PROP_0,
  PROP_DAEMON,
};

static void interfaces_iface_init (LoomInterfacesIface *iface);

G_DEFINE_TYPE_WITH_CODE (Interfaces, interfaces, LOOM_TYPE_INTERFACES_SKELETON,
                         G_IMPLEMENT_INTERFACE (LOOM_TYPE_INTERFACES,
                                                interfaces_iface_init));

static void
interfaces_init (Interfaces *interfaces)
{
  interfaces->interfaces = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                  NULL, g_object_unref);
}

static void
interfaces_finalize (GObject *object)
{
  Interfaces *interfaces = INTERFACES (object);

  g_hash_table_unref (interfaces->interfaces);

  G_OBJECT_CLASS (interfaces_parent_class)->finalize (object);
}

static void
interfaces_set_property (GObject *object,
                         guint prop_id,
                         const GValue *value,
                         GParamSpec *pspec)
{
  Interfaces *interfaces = INTERFACES (object);

  switch (prop_id)
    {
    case PROP_DAEMON:
      g_assert (interfaces->daemon == NULL);
      interfaces->daemon = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
create_interfaces (Interfaces *interfaces)
{
  struct nl_sock *sock = NULL;
  struct nl_cache *cache = NULL;

  sock = nl_socket_alloc ();
  nl_connect (sock, NETLINK_ROUTE);

  rtnl_link_alloc_cache (sock, AF_UNSPEC, &cache);
  if (cache == NULL)
    {
      g_warning (_("Error getting link cache from kernel."));
      goto out;
    }

  struct nl_object *object = nl_cache_get_first (cache);
  while (object != NULL)
    {
      struct rtnl_link *link = (struct rtnl_link *) object;
      const gchar *name = rtnl_link_get_name (link);
      const gchar *type = rtnl_link_get_type (link);
      guint8 flags = rtnl_link_get_flags (link);
      if (name != NULL && !(flags & IFF_LOOPBACK) && type == NULL)
        {
          Interface *interface = INTERFACE (interface_new (interfaces->daemon,
                                                           (gchar *)name));
          interface_export (interface);
          g_hash_table_insert (interfaces->interfaces,
                               (gchar *)interface_get_object_path (interface),
                               interface);
        }
      object = nl_cache_get_next (object);
      if (object == NULL)
        break;
    }

out:
  nl_cache_free (cache);
  nl_socket_free (sock);
}

static Interfaces *interfaces_instance;

static void
interfaces_constructed (GObject *object)
{
  Interfaces *interfaces = INTERFACES (object);
  gs_free gchar **object_paths = NULL;

  g_assert (interfaces_instance == NULL);
  interfaces_instance = interfaces;

  create_interfaces (interfaces);

  object_paths = (gchar **)g_hash_table_get_keys_as_array (interfaces->interfaces,
                                                           NULL);
  loom_interfaces_set_interfaces (LOOM_INTERFACES (object),
                                  (const gchar * const *)object_paths);

  if (G_OBJECT_CLASS (interfaces_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (interfaces_parent_class)->constructed (object);
}

static void
interfaces_class_init (InterfacesClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = interfaces_finalize;
  gobject_class->constructed = interfaces_constructed;
  gobject_class->set_property = interfaces_set_property;

  /**
   * Interfaces:daemon:
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
}

/**
 * interfaces_new:
 * @daemon: A #Daemon.
 *
 * Creates a new #Interfaces instance.
 *
 * Returns: A new #Interfaces. Free with g_object_unref().
 */
LoomInterfaces *
interfaces_new (Daemon *daemon)
{
  g_return_val_if_fail (IS_DAEMON (daemon), NULL);
  return LOOM_INTERFACES (g_object_new (TYPE_INTERFACES,
                                        "daemon", daemon,
                                        NULL));
}

/**
 * interfaces_get_interface_by_object_path:
 * @interfaces: A #Interfaces.
 * @object_path: A D-Bus object-path.
 *
 * Gets a #Interface by its D-Bus object-path.
 *
 * Returns: A #Interface object. Do not free, the object is owned by
 * @interfaces.
 */
Interface *
interfaces_get_by_object_path (Interfaces *interfaces,
                               const gchar* object_path)
{
  g_return_val_if_fail (IS_INTERFACES (interfaces), NULL);
  g_return_val_if_fail (g_variant_is_object_path (object_path), NULL);

  return g_hash_table_lookup (interfaces->interfaces, object_path);
}

/**
 * interfaces_add_interface_to_actives:
 * @interfaces: A #interfaces.
 * @interface: A #interface.
 *
 * Adds a #Interface to the active list.
 */
void
interfaces_add_to_actives (Interfaces *interfaces,
                           Interface *interface)
{
  g_return_if_fail (IS_INTERFACES (interfaces));
  g_return_if_fail (IS_INTERFACE (interface));

  const gchar *object_path;
  const gchar * const *active_interfaces;
  gs_unref_ptrarray GPtrArray *_active_interfaces;

  object_path = interface_get_object_path (interface);
  active_interfaces =
    loom_interfaces_get_active_interfaces (LOOM_INTERFACES (interfaces));
  _active_interfaces = g_ptr_array_new ();

  if (active_interfaces != NULL)
    {
      for (guint i = 0; active_interfaces[i] != NULL; i++)
        {
          g_ptr_array_add (_active_interfaces, (gpointer)active_interfaces[i]);
        }
    }

  g_ptr_array_add (_active_interfaces, (gpointer)object_path);
  g_ptr_array_add (_active_interfaces, NULL);

  loom_interfaces_set_active_interfaces (LOOM_INTERFACES (interfaces),
                              (const gchar * const *)_active_interfaces->pdata);
}

/**
 * interfaces_remove_interface_from_actives:
 * @interfaces: A #interfaces.
 * @interface: A #interface.
 *
 * Remove a #Interface from the active list.
 */
void
interfaces_remove_from_actives (Interfaces *interfaces,
                                Interface *interface)
{
  g_return_if_fail (IS_INTERFACES (interfaces));
  g_return_if_fail (IS_INTERFACE (interface));

  const gchar *object_path;
  const gchar * const *active_interfaces;
  gs_unref_ptrarray GPtrArray *_active_interfaces;

  object_path = interface_get_object_path (interface);
  active_interfaces =
    loom_interfaces_get_active_interfaces (LOOM_INTERFACES (interfaces));
  _active_interfaces = g_ptr_array_new ();

  if (active_interfaces != NULL)
    {
      for (guint i = 0; active_interfaces[i] != NULL; i++)
        {
          if (!g_str_equal (object_path, active_interfaces[i]))
            {
              g_ptr_array_add (_active_interfaces,
                               (gpointer)active_interfaces[i]);
            }
        }
    }
  g_ptr_array_add (_active_interfaces, NULL);

  loom_interfaces_set_active_interfaces (LOOM_INTERFACES (interfaces),
                              (const gchar * const *)_active_interfaces->pdata);
}

static void
interfaces_iface_init (LoomInterfacesIface *iface)
{
}

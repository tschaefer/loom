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
#include <netlink/route/route.h>

#include "tools.h"

void
tools_add_router_address (const gchar *address)
{
  struct nl_sock *sock = NULL;
  struct nl_addr *dst = NULL;
  struct nl_addr *gw = NULL;
  struct rtnl_nexthop *nhop = NULL;
  struct rtnl_route *route = NULL;

  sock = nl_socket_alloc ();
  nl_connect (sock, NETLINK_ROUTE);

  nhop = rtnl_route_nh_alloc ();
  nl_addr_parse (address, AF_INET, &gw);
  rtnl_route_nh_set_gateway (nhop, gw);
  rtnl_route_nh_set_flags (nhop, 0);

  route = rtnl_route_alloc ();
  rtnl_route_set_family (route, AF_INET);
  nl_addr_parse ("default", AF_INET, &dst);
  rtnl_route_add_nexthop (route, nhop);

  if (rtnl_route_add (sock, route, NLM_F_CREATE | NLM_F_REPLACE) != 0)
    g_warning (_("Failed to add default route."));

  nl_socket_free (sock);
  rtnl_route_put (route);
  nl_addr_put (dst);
  nl_addr_put (gw);
}

void
tools_delete_router_address (const gchar *address)
{
  struct nl_sock *sock = NULL;
  struct nl_addr *dst = NULL;
  struct nl_addr *gw = NULL;
  struct rtnl_nexthop *nhop = NULL;
  struct rtnl_route *route = NULL;

  sock = nl_socket_alloc ();
  nl_connect (sock, NETLINK_ROUTE);

  nhop = rtnl_route_nh_alloc ();
  nl_addr_parse (address, AF_INET, &gw);
  rtnl_route_nh_set_gateway (nhop, gw);
  rtnl_route_nh_set_flags (nhop, 0);

  route = rtnl_route_alloc ();
  rtnl_route_set_family (route, AF_INET);
  nl_addr_parse ("default", AF_INET, &dst);
  rtnl_route_add_nexthop (route, nhop);

  if (rtnl_route_delete (sock, route, NLM_F_CREATE | NLM_F_REPLACE) != 0)
    g_warning (_("Failed to delete default route."));

  nl_socket_free (sock);
  rtnl_route_put (route);
  nl_addr_put (dst);
  nl_addr_put (gw);

}

void tools_write_resolver_configuration (const gchar * const *nameservers,
                                         const gchar *domain,
                                         const gchar * const *searches)
{
  gchar *comment = NULL;
  GString * resolv_conf;
  GDateTime *now;
  gchar *_now = NULL;
  GError *error;

  now = g_date_time_new_now_local ();
  _now = g_date_time_format (now, "%F %T");
  g_date_time_unref (now);
  comment = g_strdup_printf (_("# Created by Loom: %s\n"), _now);
  g_free (_now);

  resolv_conf = g_string_new (comment);
  g_free (comment);

  if (domain != NULL)
      g_string_printf (resolv_conf, "domain %s\n", domain);

  if (searches != NULL)
    {
      g_string_printf (resolv_conf, "search");
      for (guint i = 0; searches[i] != NULL; i++)
          g_string_printf (resolv_conf, " %s", searches[i]);
      g_string_printf (resolv_conf, "\n");
    }

  if (nameservers != NULL)
    {
      for (guint i = 0; nameservers[i] != NULL; i++)
          g_string_printf (resolv_conf, "nameserver %s\n", nameservers[i]);
    }

  g_file_set_contents ("/etc/resolv.conf", resolv_conf->str, -1, &error);

  g_string_free (resolv_conf, TRUE);
}

void tools_erase_resolver_configuration (const gchar * const *nameservers,
                                         const gchar *domain,
                                         const gchar * const *searches)
{
  g_file_set_contents ("/etc/resolv.conf", "", 1, NULL);
}

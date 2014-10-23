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

#include <locale.h>

#include <glib/gi18n.h>
#include <glib-unix.h>

#include "daemon.h"

static GMainLoop *loop = NULL;
static Daemon *the_daemon = NULL;
static gboolean name_acquired;

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar *name,
                 gpointer user_data)
{
  name = name;
  user_data = user_data;
  the_daemon = daemon_new (connection);
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar *name,
              gpointer user_data)
{
  if (the_daemon == NULL)
    g_warning (_("Failed to connect to the message bus."));
  else if (name_acquired)
    g_message (_("Lost the name %s on the message bus."), name);
  else
    g_message (_("Failed to acquire the name %s on the message bus"), name);
  g_main_loop_quit (loop);
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar *name,
                  gpointer user_data)
{
  name_acquired = TRUE;
  g_debug (_("Acquired the name %s on the message bus."), name);
}

static gboolean
on_sigint (gpointer user_data)
{
  g_message (_("Caught SIGINT. Initiating shutdown."));
  g_main_loop_quit (loop);
  return FALSE;
}

int
main (int argc,
      char *argv[])
{
  guint name_owner_id;
  gint ret;
  guint sigint_id;

  setlocale(LC_ALL, "");

  bindtextdomain (GETTEXT_PACKAGE, LOOM_LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  ret = 1;
  loop = NULL;
  name_owner_id = 0;
  sigint_id = 0;

  signal (SIGPIPE, SIG_IGN);
  sigint_id = g_unix_signal_add_full (G_PRIORITY_DEFAULT,
                                      SIGINT,
                                      on_sigint,
                                      NULL,
                                      NULL);

  g_debug (_("loom daemon version %s starting"), PACKAGE_VERSION);

  loop = g_main_loop_new (NULL, FALSE);

  name_owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                  "org.blackox.Loom",
                                  G_BUS_NAME_OWNER_FLAGS_NONE,
                                  on_bus_acquired,
                                  on_name_acquired,
                                  on_name_lost,
                                  NULL,
                                  NULL);

  g_main_loop_run (loop);

  ret = 0;

  if (sigint_id > 0)
    g_source_remove (sigint_id);
  if (the_daemon != NULL)
    g_object_unref (the_daemon);
  if (name_owner_id != 0)
    g_bus_unown_name (name_owner_id);
  if (loop != NULL)
    g_main_loop_unref (loop);

  return ret;
}

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

#ifndef LOOM_DAEMON_H
#define LOOM_DAEMON_H

#include "types.h"

G_BEGIN_DECLS

#define TYPE_DAEMON   (daemon_get_type ())
#define DAEMON(o)     (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_DAEMON, Daemon))
#define IS_DAEMON(o)  (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_DAEMON))

GType    daemon_get_type (void) G_GNUC_CONST;
Daemon * daemon_new      (GDBusConnection *connection);

Daemon *                   daemon_get                (void);
GDBusConnection *          daemon_get_connection     (Daemon *daemon);
GDBusObjectManagerServer * daemon_get_object_manager (Daemon *daemon);

G_END_DECLS

#endif /* LOOM_DAEMON_H */

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

#ifndef LOOM_CONNECTIONS_H
#define LOOM_CONNECTIONS_H

#include "types.h"

G_BEGIN_DECLS

#define TYPE_CONNECTIONS  (connections_get_type ())
#define CONNECTIONS(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_CONNECTIONS, \
                           Connections))
#define IS_CONNECTIONS(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_CONNECTIONS))

GType             connections_get_type (void) G_GNUC_CONST;
LoomConnections * connections_new      (Daemon *daemon,
                                        Interfaces *interfaces,
                                        Settings *settings);

Connection * connections_get_by_object_path (Connections *connections,
                                             const gchar* object_path);


G_END_DECLS

#endif /* LOOM_CONNECTIONS_H */

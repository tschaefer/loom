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

#ifndef LOOM_CONNECTION_H
#define LOOM_CONNECTION_H

#include "types.h"

G_BEGIN_DECLS

#define TYPE_CONNECTION  (connection_get_type ())
#define CONNECTION(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_CONNECTION, \
                          Connection))
#define IS_CONNECTION(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_CONNECTION))

GType            connection_get_type (void) G_GNUC_CONST;
LoomConnection * connection_new      (Daemon *daemon,
                                      Interface *interface,
                                      Setting *setting);

const gchar * connection_get_object_path (Connection *connection);
const gchar * connection_get_id          (Connection *connection);
Interface *   connection_get_interface   (Connection *connection);
Setting *     connection_get_setting     (Connection *connection);

void connection_export   (Connection *connection);
void connection_unexport (Connection *connection);

void connection_add    (Connection *connection);
void connection_delete (Connection *connection);

G_END_DECLS

#endif /* LOOM_CONNECTION_H */

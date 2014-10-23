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

#ifndef LOOM_INTERFACES_H
#define LOOM_INTERFACES_H

#include "types.h"

G_BEGIN_DECLS

#define TYPE_INTERFACES  (interfaces_get_type ())
#define INTERFACES(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), \
                          TYPE_INTERFACES, Interfaces))
#define IS_INTERFACES(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), \
                          TYPE_INTERFACES))

GType            interfaces_get_type (void) G_GNUC_CONST;
LoomInterfaces * interfaces_new      (Daemon *daemon);

Interface * interfaces_get_by_object_path (Interfaces *interfaces,
                                           const gchar* object_path);

void interfaces_add_to_actives      (Interfaces *interfaces,
                                     Interface *interface);
void interfaces_remove_from_actives (Interfaces *interfaces,
                                     Interface *interface);

G_END_DECLS

#endif /* LOOM_INTERFACES_H */

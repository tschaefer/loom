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

#ifndef LOOM_INTERFACE_H
#define LOOM_INTERFACE_H

#include "types.h"

G_BEGIN_DECLS

#define TYPE_INTERFACE  (interface_get_type ())
#define INTERFACE(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), \
                         TYPE_INTERFACE, Interface))
#define IS_INTERFACE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_INTERFACE))

GType           interface_get_type (void) G_GNUC_CONST;
LoomInterface * interface_new      (Daemon *daemon, gchar *name);

const gchar * interface_get_object_path (Interface *interface);
const gchar * interface_get_name        (Interface *interface);

void interface_export   (Interface *interface);
void interface_unexport (Interface *interface);

void interface_set_up          (Interface *interface);
void interface_set_down        (Interface *interface);
void interface_add_address     (Interface *interface, const gchar *address);
void interface_delete_address  (Interface *interface, const gchar *address);

G_END_DECLS

#endif /* LOOM_INTERFACE_WIRED_H */

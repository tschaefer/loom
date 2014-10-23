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

#ifndef LOOM_TOOLS_H
#define LOOM_TOOLS_H

#include "types.h"

G_BEGIN_DECLS

void tools_add_router_address    (const gchar *address);
void tools_delete_router_address (const gchar *address);

void tools_write_resolver_configuration (const gchar * const *nameservers,
                                         const gchar *domain,
                                         const gchar * const *searches);
void tools_erase_resolver_configuration (const gchar * const *nameservers,
                                         const gchar *domain,
                                         const gchar * const *searches);

G_END_DECLS

#endif /* LOOM_TOOLS_H */

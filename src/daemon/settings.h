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

#ifndef LOOM_SETTINGS_H
#define LOOM_SETTINGS_H

#include "types.h"

G_BEGIN_DECLS

#define TYPE_SETTINGS  (settings_get_type ())
#define SETTINGS(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_SETTINGS, \
                           Settings))
#define IS_SETTINGS(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_SETTINGS))

GType          settings_get_type (void) G_GNUC_CONST;
LoomSettings * settings_new      (Daemon *daemon);

Setting * settings_get_by_object_path (Settings *settings,
                                       const gchar* object_path);

void settings_add_to_actives (Settings *settings, Setting *setting);
void settings_remove_from_actives (Settings *settings, Setting *setting);

G_END_DECLS

#endif /* LOOM_SETTINGS_H */

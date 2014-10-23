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

#ifndef LOOM_SETTING_H
#define LOOM_SETTING_H

#include "types.h"

G_BEGIN_DECLS

#define TYPE_SETTING  (setting_get_type ())
#define SETTING(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_SETTING, Setting))
#define IS_SETTING(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_SETTING))

GType         setting_get_type (void) G_GNUC_CONST;
LoomSetting * setting_new (Daemon *daemon, GVariant *configuration);

const gchar * setting_get_object_path   (Setting *setting);
const gchar * setting_get_uuid          (Setting *setting);
GVariant *    setting_get_configuration (Setting *setting);

void setting_export (Setting *setting);
void setting_unexport (Setting *setting);

G_END_DECLS

#endif /* LOOM_SETTING_H */

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

#include <uuid/uuid.h>

#include "gsystem-local-alloc.h"

#include <glib/gi18n.h>

#include "daemon.h"
#include "setting.h"

typedef struct _SettingClass SettingClass;

struct _Setting
{
  LoomSettingSkeleton parent_instance;
  Daemon *daemon;
  GVariant *configuration;
  gchar uuid[37];
};

struct _SettingClass
{
  LoomSettingSkeletonClass parent_class;
};

enum
{
  PROP_0,
  PROP_DAEMON,
  PROP_CONFIGURATION,
  PROP_OBJECT_PATH,
  PROP_UUID,
};

static void setting_iface_init (LoomSettingIface *iface);

G_DEFINE_TYPE_WITH_CODE (Setting, setting,
                         LOOM_TYPE_SETTING_SKELETON,
                         G_IMPLEMENT_INTERFACE (LOOM_TYPE_SETTING,
                                                setting_iface_init));

static void
setting_init (Setting *setting)
{
  uuid_t uuid;
  uuid_generate (uuid);
  uuid_unparse (uuid, setting->uuid);
  uuid_clear (uuid);
}

static void
setting_finalize (GObject *object)
{
  Setting *setting = SETTING (object);

  g_variant_unref (setting->configuration);

  G_OBJECT_CLASS (setting_parent_class)->finalize (object);
}

static void
setting_set_property (GObject *object,
                      guint prop_id,
                      const GValue *value,
                      GParamSpec *pspec)
{
  Setting *setting = SETTING (object);

  switch (prop_id)
    {
    case PROP_DAEMON:
      g_assert (setting->daemon == NULL);
      setting->daemon = g_value_get_object (value);
      break;

    case PROP_CONFIGURATION:
      g_assert (setting->configuration == NULL);
      setting->configuration = g_value_dup_variant (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
setting_get_property (GObject *object,
                      guint prop_id,
                      GValue *value,
                      GParamSpec *pspec)
{
  Setting *setting = SETTING (object);

  switch (prop_id)
    {
    case PROP_OBJECT_PATH:
      g_value_set_string (value, setting_get_object_path (setting));
      break;

    case PROP_UUID:
      g_value_set_string (value, setting_get_uuid (setting));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

void
setting_unexport (Setting *setting)
{
  g_return_if_fail (IS_SETTING (setting));

  GDBusObjectManagerServer *object_manager;
  GDBusObject *object;

  object_manager = daemon_get_object_manager (setting->daemon);

  object = g_dbus_interface_get_object (G_DBUS_INTERFACE (setting));
  if (object != NULL)
    g_dbus_object_manager_server_unexport (object_manager,
                                        g_dbus_object_get_object_path (object));
}

void
setting_export (Setting *setting)
{
  g_return_if_fail (IS_SETTING (setting));

  GDBusObjectManagerServer *object_manager;
  object_manager = daemon_get_object_manager (setting->daemon);

  if (g_dbus_interface_get_object (G_DBUS_INTERFACE (setting)) == NULL)
    {
      LoomObjectSkeleton *object = NULL;

      object = loom_object_skeleton_new ("/org/blackox/Loom/Setting");
      loom_object_skeleton_set_setting (object, LOOM_SETTING (setting));
      g_dbus_object_manager_server_export_uniquely (object_manager,
                                               G_DBUS_OBJECT_SKELETON (object));
      g_object_unref (object);
    }
}

static void
parse_configuration (Setting *setting)
{
  GVariantDict *dict = NULL;
  GVariant *value = NULL;
  LoomSetting *loom_setting = LOOM_SETTING (setting);

  dict = g_variant_dict_new (setting->configuration);
  value = g_variant_dict_lookup_value (dict, "address", G_VARIANT_TYPE_STRING);
  loom_setting_set_address (loom_setting, g_variant_get_string (value, NULL));

  if (g_variant_dict_contains (dict, "router"))
    {
      value = g_variant_dict_lookup_value (dict, "router",
                                           G_VARIANT_TYPE_STRING);
      loom_setting_set_router (loom_setting, g_variant_get_string (value, NULL));
    }

  if (g_variant_dict_contains (dict, "nameservers"))
    {
      value = g_variant_dict_lookup_value (dict, "nameservers",
                                           G_VARIANT_TYPE_STRING_ARRAY);
      gchar **strv = g_variant_dup_strv (value, NULL);
      loom_setting_set_name_servers (loom_setting, (const gchar **)strv);
      g_strfreev (strv);
    }

  if (g_variant_dict_contains (dict, "domain"))
    {
      value = g_variant_dict_lookup_value (dict, "domains",
                                           G_VARIANT_TYPE_STRING);
      loom_setting_set_domain (loom_setting, g_variant_get_string (value, NULL));
    }

  if (g_variant_dict_contains (dict, "searches"))
    {
      value = g_variant_dict_lookup_value (dict, "searches",
                                           G_VARIANT_TYPE_STRING_ARRAY);
      gchar **strv = g_variant_dup_strv (value, NULL);
      loom_setting_set_searches (loom_setting, (const gchar **)strv);
      g_strfreev (strv);
    }

  g_variant_dict_unref (dict);
}

static void
setting_constructed (GObject *object)
{
  Setting *setting = SETTING (object);

  parse_configuration (setting);

  if (G_OBJECT_CLASS (setting_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (setting_parent_class)->constructed (object);
}

static void
setting_class_init (SettingClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = setting_finalize;
  gobject_class->constructed = setting_constructed;
  gobject_class->set_property = setting_set_property;
  gobject_class->get_property = setting_get_property;

  g_object_class_install_property (gobject_class,
                                   PROP_DAEMON,
                                   g_param_spec_object ("daemon",
                                                        NULL,
                                                        NULL,
                                                        TYPE_DAEMON,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_CONFIGURATION,
                                   g_param_spec_variant ("configuration",
                                                         NULL,
                                                         NULL,
                                                         G_VARIANT_TYPE_VARDICT,
                                                         NULL,
                                                         G_PARAM_WRITABLE |
                                                         G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_OBJECT_PATH,
                                   g_param_spec_string ("object-path",
                                                        NULL,
                                                        NULL,
                                                        "",
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_UUID,
                                   g_param_spec_string ("uuid",
                                                        NULL,
                                                        NULL,
                                                        "",
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_STRINGS));
}

LoomSetting *
setting_new (Daemon *daemon,
             GVariant *configuration)
{
  g_return_val_if_fail (IS_DAEMON (daemon), NULL);
  g_return_val_if_fail (g_variant_is_of_type (configuration,
                                              G_VARIANT_TYPE_VARDICT), NULL);

  return LOOM_SETTING (g_object_new (TYPE_SETTING,
                                     "daemon", daemon,
                                     "configuration", configuration,
                                     NULL));
}

const gchar *
setting_get_object_path (Setting *setting)
{
  g_return_val_if_fail (IS_SETTING (setting), NULL);
  GDBusObject *object = NULL;
  object = g_dbus_interface_get_object (G_DBUS_INTERFACE (setting));
  return g_dbus_object_get_object_path (object);
}

const gchar *
setting_get_uuid (Setting *setting)
{
  g_return_val_if_fail (IS_SETTING (setting), NULL);
  return setting->uuid;
}

GVariant *
setting_get_configuration (Setting *setting)
{
  g_return_val_if_fail (IS_SETTING (setting), NULL);
  return setting->configuration;
}

static void
setting_iface_init (LoomSettingIface *iface)
{
}

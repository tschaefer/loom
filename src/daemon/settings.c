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

#include "gsystem-local-alloc.h"

#include <glib/gi18n.h>

#include "daemon.h"
#include "settings.h"
#include "setting.h"

/**
 * SECTION: Settings
 * @title: Settings
 * @short_description: Implementation of #LoomSettings for interface
 * management.
 *
 * This type provides an implementation of the #LoomSettings interface.
 */

typedef struct _SettingsClass SettingsClass;

/**
 * Settings:
 *
 * The #Settings structure contains only private data and should only be
 * accessed using the provided API.
 */
struct _Settings
{
  LoomSettingsSkeleton parent_instance;
  Daemon *daemon;
  GHashTable *settings;
};

struct _SettingsClass
{
  LoomSettingsSkeletonClass parent_class;
};

enum
{
  PROP_0,
  PROP_DAEMON,
};

static void settings_iface_init (LoomSettingsIface *iface);

G_DEFINE_TYPE_WITH_CODE (Settings, settings,
                         LOOM_TYPE_SETTINGS_SKELETON,
                         G_IMPLEMENT_INTERFACE (LOOM_TYPE_SETTINGS,
                                                settings_iface_init));

static void
settings_init (Settings *settings)
{
  settings->settings = g_hash_table_new_full (g_str_hash, g_str_equal,
                                              NULL, g_object_unref);
}

static void
settings_finalize (GObject *object)
{
  Settings *settings = SETTINGS (object);
  settings = settings;

  g_hash_table_unref (settings->settings);

  G_OBJECT_CLASS (settings_parent_class)->finalize (object);
}

static void
settings_set_property (GObject *object,
                       guint prop_id,
                       const GValue *value,
                       GParamSpec *pspec)
{
  Settings *settings = SETTINGS (object);

  switch (prop_id)
    {
    case PROP_DAEMON:
      g_assert (settings->daemon == NULL);
      settings->daemon = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static Settings *settings_instance;

static void
settings_constructed (GObject *object)
{
  Settings *settings = SETTINGS (object);

  g_assert (settings_instance == NULL);
  settings_instance = settings;

  if (G_OBJECT_CLASS (settings_parent_class)->constructed != NULL)
    G_OBJECT_CLASS (settings_parent_class)->constructed (object);
}

static void
settings_class_init (SettingsClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = settings_finalize;
  gobject_class->constructed = settings_constructed;
  gobject_class->set_property = settings_set_property;

  /**
   * Settings:daemon:
   *
   * The #Daemon for the object.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_DAEMON,
                                   g_param_spec_object ("daemon",
                                                        NULL,
                                                        NULL,
                                                        TYPE_DAEMON,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
}

/**
 * settings_new:
 * @daemon: A #Daemon.
 *
 * Creates a new #Settings instance.
 *
 * Returns: A new #Settings. Free with g_object_unref().
 */
LoomSettings *
settings_new (Daemon *daemon)
{
  g_return_val_if_fail (IS_DAEMON (daemon), NULL);
  return LOOM_SETTINGS (g_object_new (TYPE_SETTINGS,
                                      "daemon", daemon,
                                      NULL));
}

static gboolean
validate_address (const gchar *key,
                  const gchar *value,
                  gboolean suffix,
                  GError **error)
{
  if (suffix && g_strrstr (value, "/"))
    {
      gs_strfreev gchar **tokens = NULL;
      gs_unref_object GInetAddress *address = NULL;

      tokens = g_strsplit (value, "/", 2);
      address = g_inet_address_new_from_string (tokens[0]);

      if (address == NULL)
        {
          *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                           _("'%s' entry must contain a valid IPv4 address"),
                                key);
          return FALSE;
        }

      GRegex *regex = g_regex_new ("^[0-9]{1,2}$", 0, 0, NULL);
      if (!g_regex_match_full (regex, tokens[1], -1, 0, 0, NULL, NULL))
        {
          g_regex_unref (regex);
          *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                            _("'%s' entry must contain a valid IPv4 suffix"),
                                key);
          return FALSE;
        }
      g_regex_unref (regex);

      if (g_ascii_strtoull (tokens[1], NULL, 0) > 32)
        {
          *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                            _("'%s' entry must contain a valid IPv4 suffix"),
                                key);
          return FALSE;
        }
    }
  else
    {
      gs_unref_object GInetAddress *address = NULL;

      address = g_inet_address_new_from_string (value);

      if (address == NULL)
        {
          *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                _("'%s' entry must be a valid IPv4 address"),
                                key);
          return FALSE;
        }
    }

  return TRUE;
}

static gboolean
validate_domainname (const gchar *key,
                     const gchar *value,
                     GError **error)
{
  GRegex *regex;
  regex = g_regex_new ("^((?!-)[A-Za-z0-9-]{1,63}(?<!-)\\.)+[A-Za-z]{2,13}$",
                       0, 0, NULL);
  if (!g_regex_match_full (regex, value, -1, 0, 0, NULL, NULL))
    {
      g_regex_unref (regex);
      *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                            _("'%s' entry must contain a valid domain name"),
                            key);
      return FALSE;
    }
  g_regex_unref (regex);

  return TRUE;
}

static gboolean
validate_configuration (GVariant *configuration,
                        GError **error)
{
  GVariantDict *dict = NULL;
  GVariant *value = NULL;

  dict = g_variant_dict_new (configuration);

  if (g_variant_dict_contains (dict, "address"))
    {
      value = g_variant_dict_lookup_value (dict, "address",
                                           G_VARIANT_TYPE_STRING);
      if (value == NULL)
        {
          g_variant_dict_unref (dict);
          *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                _("'address' entry must be a string"));
          return FALSE;
        }
      if (!validate_address ("address", g_variant_get_string (value, NULL),
                             TRUE, error))
        {
          g_variant_dict_unref (dict);
          return FALSE;
        }
    }
  else
    {
      g_variant_dict_unref (dict);
      *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                            _("'address' entry is required"));
      return FALSE;
    }

  if (g_variant_dict_contains (dict, "router"))
    {
      value = g_variant_dict_lookup_value (dict, "router",
                                           G_VARIANT_TYPE_STRING);
      if (value == NULL)
        {
          g_variant_dict_unref (dict);
          *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                _("'router' entry must be a string"));
          return FALSE;
        }
      if (!validate_address ("router", g_variant_get_string (value, NULL),
                             FALSE, error))
        {
          g_variant_dict_unref (dict);
          return FALSE;
        }
    }

  if (g_variant_dict_contains (dict, "nameservers"))
    {
      value = g_variant_dict_lookup_value (dict, "nameservers",
                                           G_VARIANT_TYPE_STRING_ARRAY);
      if (value == NULL)
        {
          g_variant_dict_unref (dict);
          *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                            _("'nameservers' entry must be a string array"));
          return FALSE;
        }
      gsize len;
      gs_strfreev gchar **nameservers = g_variant_dup_strv (value, &len);
      for (gsize i = 0; i < len; i++)
        {
          if (!validate_address ("nameservers", nameservers[i],
                                 FALSE, error))
            {
              g_variant_dict_unref (dict);
              return FALSE;
            }
        }
    }

  if (g_variant_dict_contains (dict, "domain"))
    {
      value = g_variant_dict_lookup_value (dict, "domain",
                                           G_VARIANT_TYPE_STRING);
      if (value == NULL)
        {
          g_variant_dict_unref (dict);
          *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                _("'domain' entry must be a string"));
          return FALSE;
        }
      if (!validate_domainname ("domain", g_variant_get_string (value, NULL),
                                error))
        {
          g_variant_dict_unref (dict);
          return FALSE;
        }
    }

  if (g_variant_dict_contains (dict, "searches"))
    {
      value = g_variant_dict_lookup_value (dict, "searches",
                                           G_VARIANT_TYPE_STRING_ARRAY);
      if (value == NULL)
        {
          g_variant_dict_unref (dict);
          *error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                               _("'searches' entry must be a string array"));

          return FALSE;
        }
      gsize len;
      gs_strfreev gchar **searches = g_variant_dup_strv (value, &len);
      for (gsize i = 0; i < len; i++)
        {
          if (!validate_domainname ("searches", searches[i], error))
            {
              g_variant_dict_unref (dict);
              return FALSE;
            }
        }
    }

  g_variant_dict_unref (dict);

  return TRUE;
}

/**
 * settings_get_setting_by_object_path:
 * @settings: A #Settings.
 * @object_path: A D-Bus object-path.
 *
 * Gets a #Setting by its D-Bus object-path.
 *
 * Returns: A #Setting object. Do not free, the object is owned by
 * @settings.
 */
Setting *
settings_get_by_object_path (Settings *settings,
                             const gchar* object_path)
{
  g_return_val_if_fail (IS_SETTINGS (settings), NULL);
  g_return_val_if_fail (g_variant_is_object_path (object_path), NULL);

  return g_hash_table_lookup (settings->settings, object_path);
}


static gboolean
handle_create (LoomSettings *object,
               GDBusMethodInvocation *invocation,
               GVariant *arg_configuration)
{
  Settings *settings = SETTINGS (object);
  Setting *setting;
  gs_free gchar **object_paths = NULL;
  GError *error = NULL;

  if (!validate_configuration (arg_configuration, &error))
    {
      g_dbus_method_invocation_take_error (invocation, error);
      return TRUE;
    }

  setting = SETTING (setting_new (settings->daemon, arg_configuration));
  setting_export (setting);
  g_hash_table_insert (settings->settings,
                       (gchar *)setting_get_object_path (setting),
                       setting);

  object_paths = (gchar **)g_hash_table_get_keys_as_array (settings->settings,
                                                           NULL);
  loom_settings_set_settings (object, (const gchar* const*)object_paths);

  loom_settings_complete_create (object, invocation,
                                 setting_get_object_path (setting));

  return TRUE;
}

static gboolean
handle_destroy (LoomSettings *object,
                GDBusMethodInvocation *invocation,
                const gchar *arg_setting)
{
  Settings *settings = SETTINGS (object);
  GError *error;
  gs_free gchar **object_paths = NULL;
  const gchar * const *active_settings;

  if (!g_hash_table_remove (settings->settings, arg_setting))
    {
      error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                           _("no such 'setting' object found"));
      g_dbus_method_invocation_take_error (invocation, error);

      return TRUE;
    }

  active_settings = loom_settings_get_active_settings (object);
  if (active_settings != NULL)
    {
      for (guint i = 0; active_settings[i] != NULL; i++)
        {
          if (g_str_equal (arg_setting, active_settings[i]))
            {
              error = g_error_new (G_DBUS_ERROR, G_DBUS_ERROR_INVALID_ARGS,
                                   _("'setting' object is in use"));
              g_dbus_method_invocation_take_error (invocation, error);

              return TRUE;
            }
        }
    }

  object_paths = (gchar **)g_hash_table_get_keys_as_array (settings->settings,
                                                           NULL);
  loom_settings_set_settings (object, (const gchar* const*)object_paths);

  loom_settings_complete_destroy (object, invocation);

  return TRUE;
}

/**
 * settings_add_setting_to_actives:
 * @interfaces: A #settings.
 * @interface: A #setting.
 *
 * Adds a #Setting to the active list.
 */
void
settings_add_to_actives (Settings *settings,
                         Setting *setting)
{
  g_return_if_fail (IS_SETTINGS (settings));
  g_return_if_fail (IS_SETTING (setting));

  const gchar *object_path;
  const gchar * const *active_settings;
  gs_unref_ptrarray GPtrArray *_active_settings;

  object_path = setting_get_object_path (setting);
  active_settings =
    loom_settings_get_active_settings (LOOM_SETTINGS (settings));
  _active_settings = g_ptr_array_new ();

  if (active_settings != NULL)
    {
      for (guint i = 0; active_settings[i] != NULL; i++)
        {
          g_ptr_array_add (_active_settings, (gpointer)active_settings[i]);
        }
    }

  g_ptr_array_add (_active_settings, (gpointer)object_path);
  g_ptr_array_add (_active_settings, NULL);

  loom_settings_set_active_settings (LOOM_SETTINGS (settings),
                                (const gchar * const *)_active_settings->pdata);
}

/**
 * settings_remove_setting_from_actives:
 * @interfaces: A #settings.
 * @interface: A #setting.
 *
 * Removes a #Setting from the active list.
 */
void
settings_remove_from_actives (Settings *settings,
                              Setting *setting)
{
  g_return_if_fail (IS_SETTINGS (settings));
  g_return_if_fail (IS_SETTING (setting));

  const gchar *object_path;
  const gchar * const *active_settings;
  gs_unref_ptrarray GPtrArray *_active_settings;

  object_path = setting_get_object_path (setting);
  active_settings =
    loom_settings_get_active_settings (LOOM_SETTINGS (settings));
  _active_settings = g_ptr_array_new ();

  if (active_settings != NULL)
    {
      for (guint i = 0; active_settings[i] != NULL; i++)
        {
          if (!g_str_equal (object_path, active_settings[i]))
            {
              g_ptr_array_add (_active_settings, (gpointer)active_settings[i]);
            }
        }
    }
  g_ptr_array_add (_active_settings, NULL);

  loom_settings_set_active_settings (LOOM_SETTINGS (settings),
                                (const gchar * const *)_active_settings->pdata);
}

static void
settings_iface_init (LoomSettingsIface *iface)
{
  iface->handle_create = handle_create;
  iface->handle_destroy = handle_destroy;
}

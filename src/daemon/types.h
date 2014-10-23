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

#ifndef LOOM_TYPES_H
#define LOOM_TYPES_H

#include <glib.h>
#include <gio/gio.h>
#include <glib-object.h>

#include "loom-generated.h"

struct _Daemon;
typedef struct _Daemon Daemon;

struct _Interfaces;
typedef struct _Interfaces Interfaces;

struct _Interface;
typedef struct _Interface Interface;

struct _Settings;
typedef struct _Settings Settings;

struct _Setting;
typedef struct _Setting Setting;

struct _Connections;
typedef struct _Connections Connections;

struct _Connection;
typedef struct _Connection Connection;

#endif /* LOOM_TYPES_H */

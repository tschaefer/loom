#
# This file is part of Loom.
#
# Copyright (C) 2014 Tobias Schäfer <tschaefer@blackox.org>.
#
# Loom is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Loom is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Loom.  If not, see <http://www.gnu.org/licenses/>.

AC_PREREQ([2.69])

AC_INIT([Loom],
        [2014.09],
        [https://github.com/tschaefer/loom/issues/],
        [loom],
        [https://github.com/tschaefer/loom/])

AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR(m4)

AM_INIT_AUTOMAKE([1.11 foreign])

AC_USE_SYSTEM_EXTENSIONS

AC_PROG_CC
AC_GNU_SOURCE
AC_PROG_CC_C99
AC_PROG_RANLIB
AC_PROG_SED

AC_ISC_POSIX
AC_HEADER_STDC

AM_SILENT_RULES([yes])

GLIB_VERSION="2.42.0"
LIBNL_VERSION="3.2.24"
LIBUUID_VERSION="2.20.0"

LIBGIO_REQUIREMENT="gio-unix-2.0 >= $GLIB_VERSION"
LIBNLROUTE_REQUIREMENT="libnl-route-3.0 >= $LIBNL_VERSION"
LIBUUID_REQUIREMENT="uuid >= LIBUUID_VERSION"

PKG_CHECK_MODULES(LIBGIO, [$LIBGIO_REQUIREMENT])
PKG_CHECK_MODULES(LIBNLROUTE, [$LIBNLROUTE_REQUIREMENT])
PKG_CHECK_MODULES(LIBUUID, [$LIBUUID_REQUIREMENT])

LOOM_CFLAGS="$LIBGIO_CFLAGS $LIBNLROUTE_CFLAGS $LIBUUID_CFLAGS"
LOOM_LIBS="$LIBGIO_LIBS $LIBNLROUTE_LIBS $LIBUUID_LIBS"
AC_SUBST(LOOM_CFLAGS)
AC_SUBST(LOOM_LIBS)

AM_GNU_GETTEXT_VERSION([0.18.1])
AM_GNU_GETTEXT([external])

GETTEXT_PACKAGE="loom"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],["$GETTEXT_PACKAGE"],[gettext domain])

CFLAGS=""

if test "$GCC" = "yes"; then
   CFLAGS="-Wall \
          -Werror=strict-prototypes -Werror=missing-prototypes \
          -Werror=implicit-function-declaration \
          -Werror=pointer-arith -Werror=init-self \
          -Werror=format-security \
          -Werror=missing-include-dirs -Werror=aggregate-return $CFLAGS"
fi

AC_ARG_ENABLE([strict],
              [AC_HELP_STRING([--enable-strict=no/yes],
                              [Turn on or off strict code compilation])])
AC_MSG_CHECKING([wether to build with strict code compilation])
if test "$enable_strict" = "yes"; then
  CFLAGS="$CFLAGS -Werror"
  strict_msg="yes"
else
  strict_msg="no"
fi
AC_MSG_RESULT([$strict_msg])


AC_ARG_ENABLE([debug],
              [AC_HELP_STRING([--enable-debug=no/default/yes],
                              [Turn on or off debug])])
AC_MSG_CHECKING([wether to build with debug information])
if test "$enable_debug" != "no"; then
  CFLAGS="$CFLAGS -g"
fi
if test "$enable_debug" = "yes"; then
  CFLAGS="$CFLAGS -O0"
  AC_DEFINE([LOOM_DEBUG], [1], [Define if debug is enabled])
  debug_msg="yes"
elif test "$enable_debug" = "no"; then
  CFLAGS="$CFLAGS -O2"
  debug_msg="no"
else
  debug_msg="default"
fi
AC_MSG_RESULT([$debug_msg])

AC_MSG_CHECKING([for dbus system service environment])
dbusservicedir=$($PKG_CONFIG dbus-1 --variable=system_bus_services_dir)
if ! test -d "$dbusservicedir"; then
  AC_MSG_ERROR(["Couldn't find dbus services directory. Try installing dbus-devel."])
fi
AC_SUBST([dbusservicedir], [$dbusservicedir])
dbussystemconfdir=$($PKG_CONFIG dbus-1 --variable=sysconfdir)"/dbus-1/system.d"
if ! test -d "$dbussystemconfdir"; then
  AC_MSG_ERROR(["Couldn't find dbus system configuration directory. Try installing dbus-devel."])
fi
AC_SUBST([dbussystemconfdir], [$dbussystemconfdir])
AC_MSG_RESULT([$dbusservicedir, $dbussystemconfdir])

GTK_DOC_CHECK([1.18],[--flavour no-tmpll])

AC_CONFIG_FILES([
  Makefile
  po/Makefile.in
  doc/reference/loom/Makefile
])

AC_OUTPUT

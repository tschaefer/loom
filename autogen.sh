#!/bin/sh
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

olddir=`pwd`

srcdir=`dirname "$0"`
cd "$srcdir"

if test -z `which autoreconf`; then
	echo "Error: Package 'autoconf' not found. Please install at least v2.62"
	echo -n "This should be available by ftp from ftp.gnu.org or"
	echo " any of the fine GNU mirrors."
	exit 1
fi

if test -z `which automake`; then
	echo "Error: Package 'automake' not found. Please install at least v1.11"
	echo -n "This should be available by ftp from ftp.gnu.org or"
	echo " any of the fine GNU mirrors."
	exit 1
fi

if test -z `which libtoolize`; then
	echo "Error: Package 'libtool' not found. Please install at least v2.4.2"
	echo -n "This should be available by ftp from ftp.gnu.org or"
	echo " any of the fine GNU mirrors."
	exit 1
fi

if test -z `which pkg-config`; then
	echo "Error: Package 'pkg-config' not found. Please install at least v0.26"
	echo -n "This should be available by ftp from"
	echo " http://pkgconfig.freedesktop.org/releases/"
	exit 1
fi

if test -z `which gtkdocize`; then
	echo "Error: Package 'gtk-doc' not found. Please install at least v0.26"
	echo -n "This should be available by ftp from"
	echo " http://download.gnome.org/sources/gtk-doc/"
	exit 1
fi

rm -rf autom4te.cache
mkdir -p m4

gtkdocize || exit $?
autoreconf --force --install --verbose || exit $?

cd "$olddir"
test -n "$NOCONFIGURE" || "$srcdir/configure" "$@"

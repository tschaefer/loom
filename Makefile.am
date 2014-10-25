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

NULL =
man_MANS =
BUILT_SOURCES =
EXTRA_DIST = README
CLEANFILES =
check_SCRIPTS =
check_PROGRAMS =
noinst_SCRIPTS =
libexec_PROGRAMS =
noinst_PROGRAMS =
sbin_PROGRAMS =
noinst_LIBRARIES =
noinst_DATA =
TESTS =

VALGRIND_ARGS = --trace-children=no --quiet --error-exitcode=33 \
	--suppressions=valgrind-suppressions --gen-suppressions=all \
	--num-callers=16 --leak-check=yes \
	$(NULL)
VALGRIND_SUPPRESSIONS = \
	tools/glib.supp \
	tools/pthread.supp \
	tools/unknown.supp \
	$(NULL)

valgrind-suppressions: $(VALGRIND_SUPPRESSIONS)
	$(AM_V_GEN) cat $^ > $@
CLEANFILES += valgrind-suppressions

check-memory: valgrind-suppressions
	@echo "execute: valgrind $(VALGRIND_ARGS) [PROGRAM] [PROGRAM OPTIONS]"

SED_SUBST = sed \
	-e 's,[@]datadir[@],$(datadir),g' \
	-e 's,[@]libexecdir[@],$(libexecdir),g' \
	-e 's,[@]libdir[@],$(libdir),g' \
	-e 's,[@]includedir[@],$(includedir),g' \
	-e 's,[@]bindir[@],$(bindir),g' \
	-e 's,[@]sbindir[@],$(sbindir),g' \
	-e 's,[@]prefix[@],$(prefix),g' \
	-e 's,[@]exec_prefix[@],$(exec_prefix),g' \
	-e 's,[@]prefix[@],$(prefix),g' \
	$(NULL)

SUBDIRS = . po doc/reference/loom

include src/daemon/Makefile-daemon.am

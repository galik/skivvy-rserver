# /*-----------------------------------------------------------------.
# | Copyright (C) 2011 SooKee oasookee@googlemail.com                |
# '------------------------------------------------------------------'
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
# 
# http://www.gnu.org/licenses/gpl-2.0.html
# 
# '-----------------------------------------------------------------*/

plugin_includedir = $(includedir)/skivvy
plugin_librarydir = $(libdir)/skivvy
plugin_manualsdir = $(mandir)/skivvy

DEF_FLAGS = \
	-D STAMP=\"$(shell date +%Y%m%d-%H%M%S)\" \
	-D DEV=\"$(shell git diff --quiet || echo -dev)\" \
	-D COMMITS=\"$(shell printf %04d \"$(git log --after={yesterday} --pretty=oneline|wc -l)\")\" \
	-D REVISION=\"$(shell git log -n 1 --pretty=format:%h|tr [:lower:] [:upper:])\" \
	-D DEFAULT_PLUGIN_DIR=\"$(plugindir)\"

GEN_FLAGS = -Wl,-E -Wfatal-errors -Wall -Wextra -Winit-self -ansi -pedantic -pipe -pthread \
	-I$(top_srcdir)/src/include

# Need to have set
# $(GSL_INCS)   Guideline Support Library
# $(RNG_INCS)   ranges/v3
# $(DATE_INCS)  Hinnon's date
# $(HOL_INCS)

ASIO_LIBS = \
	$(BOOST_SYSTEM_LIB) \
	$(BOOST_ASIO_LIB)

STD_INCS = $(GSL_INCS) $(RNG_INCS) $(DATE_INCS) $(HOL_INCS) $(BOOST_CPPFLAGS)
STD_LIBS = $(STDFS_LIBS) $(ASIO_LIBS)

AM_CPPFLAGS = $(SKIVVY_CFLAGS) $(STD_INCS) $(OPENSSL_CFLAGS)
AM_CXXFLAGS = $(GEN_FLAGS) $(DEF_FLAGS)
AM_LDFLAGS = $(BOOST_LDFLAGS)

# -Wl,-E is required for programs that load plugin's
PLUGIN_LDFLAGS = -module -avoid-version -Wl,-E

plugin_include_HEADERS = \
	$(srcdir)/include/skivvy/plugin-rserver.h
	
plugin_library_LTLIBRARIES =  \
	skivvy-plugin-rserver.la

# IrcBot plugins
skivvy_plugin_rserver_la_SOURCES = plugin-rserver.cpp
skivvy_plugin_rserver_la_LDFLAGS = $(PLUGIN_LDFLAGS) $(AM_LDFLAGS)
skivvy_plugin_rserver_la_LIBADD  = $(SKIVVY_LIBS) $(STD_LIBS) -L.libs

plugin_manuals_DATA = $(top_srcdir)/docs/rserver-manual.text

extra_DIST = $(top_srcdir)/docs/rserver-manual.text

install-data-hook: $(plugin_library_LTLIBRARIES)
	for m in $(plugin_library_LTLIBRARIES); do \
		base=`echo $$m | sed 's/\..*//'`; \
		rm -f $(plugin_librarydir)/$$base.la; \
	done

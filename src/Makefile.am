## Copyright (C) 2012 Kolibre
#
# This file is part of kolibre-player.
#
# Kolibre-player is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 2.1 of the License, or
# (at your option) any later version.
#
# Kolibre-player is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with kolibre-player. If not, see <http://www.gnu.org/licenses/>.

AUTOMAKE_OPTIONS = foreign

lib_LTLIBRARIES = libkolibre-player.la

# Install the headers in versioned directory -e.g. examplelib1-0:
library_includedir=$(includedir)/libkolibre/player-$(PACKAGE_VERSION)
library_include_HEADERS = Player.h PlayerState.h

libkolibre_player_la_SOURCES = Player.cpp PlayerImpl.cpp PlayerPosition.cpp
libkolibre_player_la_LIBADD = @LOG4CXX_LIBS@ @GLIB_LIBS@ @GST_LIBS@ @PTHREAD_LIBS@
libkolibre_player_la_LDFLAGS = -version-info $(VERSION_INFO)
libkolibre_player_la_CPPFLAGS= @LOG4CXX_LIBS@ @GLIB_CFLAGS@ @GST_CFLAGS@ @PTHREAD_CFLAGS@

EXTRA_DIST = PlayerImpl.h SmilTime.h PlayerPosition.h

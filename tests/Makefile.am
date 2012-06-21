## Copyright (C) 2012  The Kolibre Foundation
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

check_PROGRAMS = codectest \
				 tempopitchtest \
				 seektest \
				 seek_on_continue

TESTS = codectest.sh \
		tempopitchtest.sh \
		seektest.sh \
		seek_on_continue

codectest_SOURCES = codectest.cpp
codectest_LDADD = -L$(top_builddir)/src -lkolibre-player @GST_LIBS@ @GSTCONTROLLER_LIBS@ @PTHREAD_LIBS@ @LOG4CXX_LIBS@
codectest_CPPFLAGS = -I$(top_srcdir)/src @PTHREAD_CFLAGS@ @LOG4CXX_CFLAGS@

tempopitchtest_SOURCES = tempopitchtest.cpp
tempopitchtest_LDADD = -L$(top_builddir)/src -lkolibre-player @GST_LIBS@ @GSTCONTROLLER_LIBS@ @PTHREAD_LIBS@ @LOG4CXX_LIBS@
tempopitchtest_CPPFLAGS = -I$(top_srcdir)/src @PTHREAD_CFLAGS@ @LOG4CXX_CFLAGS@

seektest_SOURCES = seektest.cpp
seektest_LDADD = -L$(top_builddir)/src -lkolibre-player @GST_LIBS@ @GSTCONTROLLER_LIBS@ @PTHREAD_LIBS@ @LOG4CXX_LIBS@
seektest_CPPFLAGS = -I$(top_srcdir)/src @PTHREAD_CFLAGS@ @LOG4CXX_CFLAGS@

seek_on_continue_SOURCES = seek_on_continue_data.cpp seek_on_continue.cpp
seek_on_continue_LDADD = -L$(top_builddir)/src -lkolibre-player @GST_LIBS@ @GSTCONTROLLER_LIBS@ @PTHREAD_LIBS@ @LOG4CXX_LIBS@
seek_on_continue_CPPFLAGS = -I$(top_srcdir)/src @PTHREAD_CFLAGS@ @LOG4CXX_CFLAGS@

AM_CPPFLAGES = -I$(srcdir)/..$(all_includes)

EXTRA_DIST = setup_logging.h \
			 data.h \
			 codectest.sh \
			 tempopitchtest.sh \
			 seektest.sh \
			 testdata

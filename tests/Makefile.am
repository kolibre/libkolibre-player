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

check_PROGRAMS = codectest \
				 tempopitchtest \
				 seektest \
				 playersignaltest \
				 seek_on_continue

TESTS = codectest_wav.sh \
		codectest_ogg.sh \
		codectest_mp3.sh \
		tempopitchtest_wav.sh \
		tempopitchtest_ogg.sh \
		tempopitchtest_mp3.sh \
		seektest_wav.sh \
		seektest_ogg.sh \
		seektest_mp3.sh \
		seek_on_continue \
		playersignaltest

playersignaltest_SOURCES = player_signal_test.cpp 
playersignaltest_CPPFLAGS = -I$(top_srcdir)/src -g @LOG4CXX_LIBS@ @GLIB_CFLAGS@ @GST_CFLAGS@ @PTHREAD_CFLAGS@
playersignaltest_LDFLAGS = -I$(top_srcdir)/src $(top_builddir)/src/libkolibre_player_la-PlayerImpl.lo @GST_LIBS@ @GSTCONTROLLER_LIBS@ @PTHREAD_LIBS@ @LOG4CXX_LIBS@

codectest_SOURCES = codectest.cpp
tempopitchtest_SOURCES = tempopitchtest.cpp
seektest_SOURCES = seektest.cpp
seek_on_continue_SOURCES = seek_on_continue_data.cpp seek_on_continue.cpp

LDADD = -lkolibre-player
AM_LDFLAGS = -L$(top_builddir)/src @GST_LIBS@ @GSTCONTROLLER_LIBS@ @PTHREAD_LIBS@ @LOG4CXX_LIBS@
AM_CPPFLAGS = @PTHREAD_CFLAGS@ @LOG4CXX_CFLAGS@ -I$(top_srcdir)/src

EXTRA_DIST = setup_logging.h \
			 data.h \
			 codectest_wav.sh \
			 codectest_ogg.sh \
			 codectest_mp3.sh \
			 tempopitchtest_wav.sh \
			 tempopitchtest_ogg.sh \
			 tempopitchtest_mp3.sh \
			 seektest_wav.sh \
			 seektest_ogg.sh \
			 seektest_mp3.sh \
			 testdata

clean-local: clean-local-check
.PHONY: clean-local-check

clean-local-check:
	rm -f *.log

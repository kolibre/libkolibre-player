What is Kolibre?
---------------------------------
Kolibre is a Finnish non-profit association whose purpose is to promote
information systems that aid people with reading disabilities. The software
which Kolibre develops is published under open source and made available to all
stakeholders at github.com/kolibre.

Kolibre is committed to broad cooperation between organizations, businesses and
individuals around the innovative development of custom information systems for
people with different needs. More information about Kolibres activities, association 
membership and contact information can be found at http://www.kolibre.org/


What is libkolibre-player?
---------------------------------
Libkolibre-player is a library for using gstreamer for playback of audio content
such as mp3, ogg or wav. It provides necessary interface methods to open a
selected segment of content from a file source or http source and to adjust the
tempo, pitch and volume of the playback.


Documentation
---------------------------------
Kolibre client developer documentation is available at 
https://github.com/kolibre/libkolibre-builder/wiki

This library is documented using doxygen.

Write ./configure && make doxygen-doc to generate documentation.


Platforms
---------------------------------
Libkolibre-player has been tested with Linux Debian Squeeze and can be built using
dev packages from apt repositories.


Dependencies
---------------------------------
Major dependencies of libkolibre-player:

* glib-2.0
* gsteramer-0.10
* gstreamer0.10-plugins-base
* gstreamer0.10-plugins-good
* gstreamer0.10-plugins-bad
* gstreamer0.10-plugins-ugly
* gstreamer-controller
* libxml2
* libpthread
* zlib
* liblog4cxx
* libboost-signals

Gstreamer elements required for full functionality:
---------------------------------
alsasink
audioamplify
audioconvert
cdparanoiasrc
decodebin
directsoundsink(windows only)
faad
fakesink(core)
filesrc(core)
flump3dec
level
oggdemux
pipeline
pitch
queue(core)
souphttpsrc
vorbisdec
wavparse

Plugins and elements can be installed by installing the correct gst-plugin-package. 
A list of plugins and their respective gst-plugin-package can be found at:
http://gstreamer.freedesktop.org/documentation/plugins.html

Building from source 
---------------------------------
If building from GIT sources, first do a:
$ autoreconf -fi

If building from a release tarball you can skip de above step. 
$ ./configure
$ make
$ make install

see INSTALL for detailed instructions.


Licensing
---------------------------------
Copyright (C) 2012 Kolibre

This file is part of libkolibre-player.

Libkolibre-player is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

Libkolibre-player is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with libkolibre-player. If not, see <http://www.gnu.org/licenses/>.

[![githalytics.com alpha](https://cruel-carlota.pagodabox.com/e4126976c853700e8960ebc9713ed68c "githalytics.com")](http://githalytics.com/kolibre/libkolibre-player)

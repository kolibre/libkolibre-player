/*
Copyright (C) 2012  The Kolibre Foundation

This file is part of kolibre-player.

Kolibre-player is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

Kolibre-player is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with kolibre-player. If not, see <http://www.gnu.org/licenses/>.
*/

#include <vector>
#include <string>

struct playitem {

    std::string url;
    long long startms;
    long long stopms;

    playitem(std::string s, unsigned s1, unsigned s2) {
        url = s; startms = s1; stopms = s2;
    }
};

typedef std::vector <playitem> Urls;

Urls initvector();

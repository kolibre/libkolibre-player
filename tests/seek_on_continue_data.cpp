/*
Copyright (C) 2012 Kolibre

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

#include "data.h"

Urls initvector()
{
    Urls urls;
    urls.push_back(playitem("testdata/ogg/dtb_10s.ogg", 0, 999));
    urls.push_back(playitem("testdata/mp3/dtb_10s.mp3", 999, 1999));
    urls.push_back(playitem("testdata/wav/dtb_10s.wav", 1999, 2999));
    urls.push_back(playitem("testdata/ogg/dtb_10s.ogg", 2999, 3999));
    urls.push_back(playitem("testdata/mp3/dtb_10s.mp3", 3999, 4999));
    urls.push_back(playitem("testdata/wav/dtb_10s.wav", 4999, 5999));
    urls.push_back(playitem("testdata/ogg/dtb_10s.ogg", 5999, 6999));
    urls.push_back(playitem("testdata/mp3/dtb_10s.mp3", 6999, 7999));
    urls.push_back(playitem("testdata/wav/dtb_10s.wav", 7999, 8999));
    urls.push_back(playitem("testdata/ogg/dtb_10s.ogg", 8999, 9999));
    urls.push_back(playitem("testdata/mp3/dtb_20s.mp3", 0, 999));
    urls.push_back(playitem("testdata/wav/dtb_20s.wav", 999, 1999));
    urls.push_back(playitem("testdata/ogg/dtb_20s.ogg", 1999, 2999));
    urls.push_back(playitem("testdata/mp3/dtb_20s.mp3", 2999, 3999));
    urls.push_back(playitem("testdata/wav/dtb_20s.wav", 3999, 4999));
    urls.push_back(playitem("testdata/ogg/dtb_20s.ogg", 4999, 5999));
    urls.push_back(playitem("testdata/mp3/dtb_20s.mp3", 5999, 6999));
    urls.push_back(playitem("testdata/wav/dtb_20s.wav", 6999, 7999));
    urls.push_back(playitem("testdata/ogg/dtb_20s.ogg", 7999, 8999));
    urls.push_back(playitem("testdata/mp3/dtb_20s.mp3", 8999, 9999));
    urls.push_back(playitem("testdata/wav/dtb_30s.wav", 0, 999));
    urls.push_back(playitem("testdata/ogg/dtb_30s.ogg", 999, 1999));
    urls.push_back(playitem("testdata/mp3/dtb_30s.mp3", 1999, 2999));
    urls.push_back(playitem("testdata/wav/dtb_30s.wav", 2999, 3999));
    urls.push_back(playitem("testdata/ogg/dtb_30s.ogg", 3999, 4999));
    urls.push_back(playitem("testdata/mp3/dtb_30s.mp3", 4999, 5999));
    urls.push_back(playitem("testdata/wav/dtb_30s.wav", 5999, 6999));
    urls.push_back(playitem("testdata/ogg/dtb_30s.ogg", 6999, 7999));
    urls.push_back(playitem("testdata/mp3/dtb_30s.mp3", 7999, 8999));
    urls.push_back(playitem("testdata/wav/dtb_30s.wav", 8999, 9999));
    urls.push_back(playitem("testdata/ogg/dtb_40s.ogg", 0, 999));
    urls.push_back(playitem("testdata/mp3/dtb_40s.mp3", 999, 1999));
    urls.push_back(playitem("testdata/wav/dtb_40s.wav", 1999, 2999));
    urls.push_back(playitem("testdata/ogg/dtb_40s.ogg", 2999, 3999));
    urls.push_back(playitem("testdata/mp3/dtb_40s.mp3", 3999, 4999));
    urls.push_back(playitem("testdata/wav/dtb_40s.wav", 4999, 5999));
    urls.push_back(playitem("testdata/ogg/dtb_40s.ogg", 5999, 6999));
    urls.push_back(playitem("testdata/mp3/dtb_40s.mp3", 6999, 7999));
    urls.push_back(playitem("testdata/wav/dtb_40s.wav", 7999, 8999));
    urls.push_back(playitem("testdata/ogg/dtb_40s.ogg", 8999, 9999));
    urls.push_back(playitem("testdata/mp3/dtb_48s.mp3", 0, 999));
    urls.push_back(playitem("testdata/wav/dtb_48s.wav", 999, 1999));
    urls.push_back(playitem("testdata/ogg/dtb_48s.ogg", 1999, 2999)); //change start of this to 1800 and the test will pass
    urls.push_back(playitem("testdata/mp3/dtb_48s.mp3", 2999, 3999));
    urls.push_back(playitem("testdata/wav/dtb_48s.wav", 3999, 4999));
    urls.push_back(playitem("testdata/ogg/dtb_48s.ogg", 4999, 5999));
    urls.push_back(playitem("testdata/wav/dtb_48s.wav", 5999, 6999));
    return urls;
}

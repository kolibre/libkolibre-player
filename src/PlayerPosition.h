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


#ifndef PLAYERPOSITION_H
#define PLAYERPOSITION_H

#include <string>
#include <ctime>

class PlayerImpl;

class PlayerPosition
{
    public:
        explicit PlayerPosition( PlayerImpl& p );
        PlayerPosition();
        bool open( PlayerImpl& p, long long int offsetms = 0 );
        bool olderThan( double seconds );
        bool isValid();
    private:
        std::string filename;
        long long int start;
        long long int current;
        long long int end;
        bool valid;
        time_t creationTime;
};

#endif

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


#include "PlayerPosition.h"
#include "PlayerImpl.h"

using namespace std;

PlayerPosition::PlayerPosition( PlayerImpl& p ):
    filename(p.getFilename()),
    start(p.getStartms()),
    current(p.getPos()),
    end(p.getStopms()),
    valid(true)
{
    time( &creationTime );
    if( filename == "" )
        valid = false;
}

PlayerPosition::PlayerPosition():
    start(0),
    current(0),
    end(0),
    valid(false)
{
    time( &creationTime );
}

bool PlayerPosition::open( PlayerImpl& p, long long int offsetms )
{
    if( valid ) {
        p.open( filename, max(current+offsetms,start), end );
        return true;
    }
    return false;
}

bool PlayerPosition::olderThan( double seconds )
{
    time_t now;
    time(&now);

    return difftime(now, creationTime) > seconds;
}

bool PlayerPosition::isValid()
{
    return valid;
}

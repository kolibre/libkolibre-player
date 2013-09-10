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

#include <cstdlib>
#include "PlayerImpl.h"

#include "setup_logging.h"

bool playerMessageSlot( Player::playerMessage message )
{
    std::cout << "Got player message " << message << std::endl;
    switch (message)
    {
    case Player::PLAYER_CONTINUE:
        return false;
    case Player::PLAYER_ERROR:
        return true;
    }
}

int main(int argc, char *argv[])
{
    setup_logging();

    PlayerImpl Player;

    Player.doOnPlayerMessage(playerMessageSlot);

    bool res = Player.sendERRORSignal();
    assert(res);

    res = Player.sendCONTSignal();
    assert(!res);


    return 0;
}






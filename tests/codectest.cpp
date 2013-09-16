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
#include <Player.h>

#include "data.h"
#include "setup_logging.h"
#include <boost/bind.hpp>

using namespace std;

class PlayerControl
{
    public:
        Player *player;
        int var_argc;
        char **var_argv;
        bool atEOS;
        bool error;
        string source;
        PlayerControl();
        void play();
        bool enable(int argc, char **argv);
        void setSource(string src);
        bool playerMessageSlot(Player::playerMessage message);
        bool playerStateSlot( playerState state );
};

PlayerControl::PlayerControl():
    player(Player::Instance()),
    atEOS(false),
    error(false),
    source()
{
    player->doOnPlayerMessage( boost::bind(&PlayerControl::playerMessageSlot, this, _1) );
    player->doOnPlayerState( boost::bind(&PlayerControl::playerStateSlot, this, _1) );
}

bool PlayerControl::playerMessageSlot( Player::playerMessage message )
{
    cout << "Got player message " << message << endl;
    switch (message)
    {
        case Player::PLAYER_CONTINUE:
            return true;
        case Player::PLAYER_ERROR:
            error = true;
        case Player::PLAYER_ATEOS:
            atEOS = true;
            return true;
    }

    return false;
}

bool PlayerControl::playerStateSlot( playerState state )
{
    cout << "Got player state " << state << endl;
    switch (state)
    {
        case INACTIVE:
        case BUFFERING:
        case PLAYING:
        case PAUSING:
        case STOPPED:
        case EXITING:
            return true;
    }

    return false;
}

void PlayerControl::play()
{
    player->open( source );
    player->resume();
    while (!atEOS) sleep(1);

    // Assert that test run til end and no errors occurred!
    assert( atEOS == true );
    assert( error == false );
}

bool PlayerControl::enable( int argc, char **argv )
{
    var_argc = argc;
    var_argv = argv;

    player->enable(&var_argc, &var_argv);
}

void PlayerControl::setSource(string src)
{
    source = src;
}

int main(int argc, char *argv[])
{
    setup_logging();

    PlayerControl playerControl;

    if (argc > 1) playerControl.setSource(argv[1]);

    playerControl.enable(argc, argv);

    playerControl.play();
}

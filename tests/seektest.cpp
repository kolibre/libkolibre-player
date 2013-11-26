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

#include <ctime>
#include <cstdlib>
#include <Player.h>

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
    source()
{
    player->doOnPlayerMessage( boost::bind(&PlayerControl::playerMessageSlot, this, _1) );
    player->doOnPlayerState( boost::bind(&PlayerControl::playerStateSlot, this, _1) );
}

bool PlayerControl::playerMessageSlot( Player::playerMessage message )
{
    switch (message)
    {
        case Player::PLAYER_CONTINUE:
        case Player::PLAYER_ATEOS:
        case Player::PLAYER_ERROR:
            exit(1); // these messages shold not be triggered
    }

    return false;
}

bool PlayerControl::playerStateSlot( playerState state )
{
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

    int counter = 1;

    // Give player thread 2 seconds to process
    sleep(2);

    while (!atEOS && counter < 10 )
    {
        // Open a random position between 0 and 40 seconds
        srand ( time(NULL) );
        long long int pos = (rand() % 40)*1000;
        player->seekPos(pos);

        // Give player thread 1 second to process
        sleep(1);

        // Get current position
        long long int getpos = player->getPos();

        // Assert that current position is greater than desired position,
        // but not larger than 2 seconds from desired position
        assert(getpos >= pos && getpos <= pos+2000);

        // Increase counter and assert that we did not reach end of file
        counter++;
        assert(!atEOS);

    }

    delete player;
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

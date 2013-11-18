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
            return true;
        case Player::PLAYER_ERROR:
            assert(false);
        case Player::PLAYER_ATEOS:
            atEOS = true;
            return true;
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

    sleep(1);
    assert(!atEOS);

    player->setPitch(1.2);
    player->pause();
    player->resume();
    assert(player->getPitch() == 1.2);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setPitch(1.5);
    player->pause();
    player->resume();
    assert(player->getPitch() == 1.5);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setPitch(20.0);
    player->pause();
    player->resume();
    assert(player->getPitch() == PLAYER_MAX_PITCH);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setPitch(1.0);
    player->pause();
    player->resume();
    assert(player->getPitch() == 1.0);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setTempo(1.1);
    player->pause();
    player->resume();
    assert(player->getTempo()==1.1);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setTempo(1.2);
    player->pause();
    player->resume();
    assert(player->getTempo()==1.2);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setTempo(1.3);
    player->pause();
    player->resume();
    assert(player->getTempo()==1.3);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setTempo(1.4);
    player->pause();
    player->resume();
    assert(player->getTempo()==1.4);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setTempo(1.5);
    player->pause();
    player->resume();
    assert(player->getTempo()==1.5);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setTempo(220.0);
    player->pause();
    player->resume();
    assert(player->getTempo()==PLAYER_MAX_TEMPO);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setTempo(1.0);
    player->pause();
    player->resume();
    assert(player->getTempo()==1.0);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setTempo(0.5);
    player->pause();
    player->resume();
    assert(player->getTempo()==0.5);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setTempo(0.7);
    player->pause();
    player->resume();
    assert(player->getTempo()==0.7);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setTempo(1.0);
    player->setPitch(1.0);
    player->pause();
    player->resume();
    assert(player->getTempo()==1.0);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setTempo(1.5);
    player->setPitch(1.5);
    player->pause();
    player->resume();
    assert(player->getTempo()==1.5);
    assert(player->getPitch()==1.5);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setTempo(1.2);
    player->setPitch(1.2);
    player->pause();
    player->resume();
    assert(player->getTempo()==1.2);
    assert(player->getPitch()==1.2);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setTempo(1.6);
    player->setPitch(1.6);
    player->pause();
    player->resume();
    assert(player->getTempo()==1.6);
    assert(player->getPitch()==1.6);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setTempo(0.6);
    player->setPitch(0.6);
    player->pause();
    player->resume();
    assert(player->getTempo()==0.6);
    assert(player->getPitch()==0.6);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setTempo(1.0);
    player->setPitch(1.0);
    player->pause();
    player->resume();
    assert(player->getTempo()==1.0);
    assert(player->getPitch()==1.0);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setTempo(0.5);
    player->setPitch(1.5);
    player->pause();
    player->resume();
    assert(player->getTempo()==0.5);
    assert(player->getPitch()==1.5);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setTempo(1.5);
    player->setPitch(0.5);
    player->pause();
    player->resume();
    assert(player->getTempo()==1.5);
    assert(player->getPitch()==0.5);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->setTempo(1.0);
    player->setPitch(1.0);
    player->pause();
    player->resume();
    assert(player->getTempo()==1.0);
    assert(player->getPitch()==1.0);
    sleep(1);
    assert(player->isPlaying());
    assert(!atEOS);

    player->stop();
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

    return 0;
}

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
#include "data.h"
#include <boost/bind.hpp>

using namespace std;

class PlayerControl
{
    public:
        int currentitem;
        Player *player;
        Urls urls;
        int maxitems;
        int var_argc;
        char **var_argv;
        bool atEOS;
        string srcDir;
        PlayerControl();
        void play();
        bool enable(int argc, char **argv);
        bool playerMessageSlot(Player::playerMessage message);
        bool playerStateSlot( playerState state );
};

PlayerControl::PlayerControl():
    currentitem(0),
    player(Player::Instance()),
    urls(initvector()),
    atEOS(false)
{
    player->doOnPlayerMessage( boost::bind(&PlayerControl::playerMessageSlot, this, _1) );
    player->doOnPlayerState( boost::bind(&PlayerControl::playerStateSlot, this, _1) );
    maxitems = urls.size();
    char* srcdir = getenv("srcdir");
    if(!srcdir)
        srcdir = ".";
    srcDir = string(srcdir);
}

bool PlayerControl::playerMessageSlot( Player::playerMessage message )
{
    cout << "got player message " << message << endl;
    switch (message)
    {
        case Player::PLAYER_CONTINUE:
            {
                currentitem++;
                if (currentitem < maxitems) {
                    cout << "\nincrementing current item, next segment: ["<< urls[currentitem].startms << ", " << urls[currentitem].stopms << "] "<< endl;
                    player->open( srcDir + "/" + urls[currentitem].url, urls[currentitem].startms, urls[currentitem].stopms );
                    cout << "\nPLAYER CONTINUE" << endl;
                    return true;
                }
                else {
                    player->stop();
                    atEOS = true;
                }
                break;
            }
        case Player::PLAYER_ATEOS:
            {
                currentitem++;
                if(currentitem < maxitems) {
                    cout << "\nPLAYER AT END OF STREAM, NEXT AUDIO FILE" << endl;
                    player->open( srcDir + "/" + urls[currentitem].url, urls[currentitem].startms, urls[currentitem].stopms );
                    cout << "\nincrementing current item, next segment: ["<< urls[currentitem].startms << ", " << urls[currentitem].stopms << "] "<< endl;
                    return true;
                }
                else {
                    cout << "\nPLAYER AT END OF STREAM AND NO SEGMENTS LEFT TO PLAY -> Quit" << endl;
                    atEOS = true;
                }
                break;
            }
        case Player::PLAYER_ERROR:
            {
                atEOS = true;
            }
    }
    return false;
}

bool PlayerControl::playerStateSlot( playerState state )
{
    cout << "\nGot player state, id: "<< state << endl;
    switch (state)
    {
        case INACTIVE:
            cout << "\nGOT PLAYER STATE: INACTIVE" << endl;
            return true;
        case BUFFERING:
            cout << "\nGOT PLAYER STATE: BUFFERING" << endl;
            return true;
        case PLAYING:
            cout << "\nGOT PLAYER STATE: PLAYING" << endl;
            return true;
        case PAUSING:
            cout << "\nGOT PLAYER STATE: PAUSING" << endl;
            return true;
        case STOPPED:
            cout << "\nGOT PLAYER STATE: STOPPED" << endl;
            return true;
        case EXITING:
            cout << "\nGOT PLAYER STATE: EXITING" << endl;
            return true;
    }

    return false;
}

void PlayerControl::play()
{
    player->open( srcDir + "/" + urls[currentitem].url, urls[currentitem].startms, urls[currentitem].stopms);
    player->resume();
    cout << "\nWaiting for eos or max segments" << maxitems << endl;
    int time = 0;
    while(!atEOS && currentitem < maxitems && time < 60){
        sleep(1);
        time++;
    }

    cout << "\nPlayer stopped. segments played: " << currentitem << endl;
    assert(currentitem == maxitems);
    assert(time < 60);
}

bool PlayerControl::enable( int argc, char **argv )
{
    var_argc = argc;
    var_argv = argv;

    player->enable(&var_argc, &var_argv);
}

int main(int argc, char *argv[])
{
    setup_logging();

    PlayerControl playerControl;

    playerControl.enable(argc, argv);

    playerControl.play();
}

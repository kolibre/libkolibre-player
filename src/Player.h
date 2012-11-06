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


// Player header file
#ifndef PLAYER_H
#define PLAYER_H

#include <iostream>
#include <vector>
#include <string>
#include <boost/signals2.hpp>

#include "PlayerState.h"

// Features
//#define ENABLE_EQUALIZER
#define ENABLE_FADEIN

// Limits
#define MIN_TEMPO 0.5
#define MAX_TEMPO 1.5
#define MIN_PITCH 0.5
#define MAX_PITCH 1.5
#define MIN_VOLUMEGAIN 0.5
#define MAX_VOLUMEGAIN 1.5
#define MIN_BASS -0.5
#define MAX_BASS 0.5
#define MIN_TREBLE -0.5
#define MAX_TREBLE 0.5

struct PlayerImpl;

class Player
{
    public:

        static Player *Instance();
        ~Player();

        bool enable(int *argc, char **argv[]);

        void open(const char *url, const char *startms, const char *stopms);
        void open(std::string filename, long long startms, long long stopms);
        void open(std::string filename, long long startms);
        void open(std::string filename);
        void stop();
        void reopen();
        void pause();
        void resume();

        std::string getFilename();

        long long int getPos();
        void seekPos(long long int seektime);
        long long int getStartms();
        long long int getStopms();

        playerState getState();
        playerState getRealState();
        std::string getState_str();
        void setState(playerState state);

        bool loadTrackData();
        std::vector<long long int> getTracks();
        unsigned int getTrack();
        unsigned int getNumTracks();
        std::string getCDADiscid();

        double getTempo();
        void setTempo(double);
        void adjustTempo(double);

        double getPitch();
        void setPitch(double);
        void adjustPitch(double);

        double getVolumeGain();
        void setVolumeGain(double);
        void adjustVolumeGain(double);

        double getBass();
        void setBass(double);
        void adjustBass(double);

        double getTreble();
        void setTreble(double);
        void adjustTreble(double);

        void setDebugmode(bool);
        void setUseragent(std::string);

        enum playerMessage { PLAYER_CONTINUE, PLAYER_ATEOS, PLAYER_BUFFERING, PLAYER_ERROR };

        /**
         * Holds data about an audio segment. The data is sent with the OnPlayerTime signal.
         */
        typedef struct {
            long current;
            long duration;
            long segmentstart;
            long segmentstop;
        } timeData;

        typedef boost::signals2::signal<bool (playerMessage)> OnPlayerMessage;
        typedef boost::signals2::signal<bool (playerState)> OnPlayerState;
        typedef boost::signals2::signal<bool (timeData)> OnPlayerTime;

        void doOnPlayerMessage(OnPlayerMessage::slot_type slot);
        void doOnPlayerState(OnPlayerState::slot_type slot);
        void doOnPlayerTime(OnPlayerTime::slot_type slot);
        bool isPlaying();

        Player();
        /*! \cond PRIVATE */
        // Instance of the player class
        static Player *pinstance;

        PlayerImpl* p_impl;
        /*! \endcond */
};

#endif

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

//Player Implementation header file
#ifndef PLAYERIMPL_H
#define PLAYERIMPL_H
#include <string>
#include <glib.h>
#include <gst/gst.h>
#include <pthread.h>

#include "Player.h"
#include "PlayerPosition.h"
#include "PlayerState.h"

struct PlayerImpl
{
    PlayerImpl();
    ~PlayerImpl();

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

    bool isPlaying();

    boost::signals2::connection doOnPlayerMessage(Player::OnPlayerMessage::slot_type slot);
    boost::signals2::connection doOnPlayerState(Player::OnPlayerState::slot_type slot);
    boost::signals2::connection doOnPlayerTime(Player::OnPlayerTime::slot_type slot);


    // PRIVATE

    Player::OnPlayerMessage onPlayerMessage;
    Player::OnPlayerState onPlayerState;
    Player::OnPlayerTime onPlayerTime;

    bool sendCONTSignal();
    bool sendEOSSignal();
    bool sendBUFFERINGSignal();
    bool sendERRORSignal();


    int *var_argc;
    char ***var_argv;

    bool initGstreamer();

    GstBus *pBus;
    GstElement
        // Pipeline and source
        *pPipeline,
        *pDatasource,
        *pCddasrc,
        *pQueue2,

        // Decoder for ogg
        *pOggdemux,
        *pVorbisdec,

        // Decoder for mp3
        *pFlump3dec,

        // Decoder for aac
        *pFaaddec,

        // Decoder for wav
        *pWavparse,

        // Decoder for other formats
        *pDecodebin,

        // Post processing
        *pAudioconvert1,
        *pPitch,
        *pEqualizer,
        *pAudioconvert2,
        *pQueue,   // Queue
        *pAudiodynamic,   // Audio dynamics adjust
        *pLevel,   // Level indicator
        *pAmplify,  // Level adjust
        *pAudiosink;

    GstClock *pClock;

    //GstController *pFadeController;
    //GValue mFadeControllerVolume;

    GstElement *setupPostprocessing(GstBin *);
    GstElement *setupDatasource(GstBin *);

    bool setupOGGPipeline();
    bool setupMP3Pipeline();
    bool setupAACPipeline();
    bool setupWAVPipeline();
    bool setupCDAPipeline();
    bool setupUnknownPipeline();

    enum pipelineType {
        OGGPIPE,  // Ogg-Vorbis special pipeline
        MP3PIPE,  // Mp3 special pipeline
        AACPIPE,  // AAC special pipeline
        WAVPIPE,  // Wav special pipeline
        CDAPIPE,  // Audio CD pipeline
        ANYPIPE,  // Fallback pipeline
        NOPIPE      // Not yet initialized
    } pipeType;

    bool setupPipeline();
    bool destroyPipeline();

    void lockMutex(pthread_mutex_t *theMutex);
    void unlockMutex(pthread_mutex_t *theMutex);

    friend void *player_thread(void *player);
    friend gboolean start_time_callback (GstClock *clock, GstClockTime time, GstClockID id, gpointer player_object);
    friend gboolean stop_time_callback (GstClock *clock, GstClockTime time, GstClockID id, gpointer player_object);
    friend gboolean cb_data_probe (GstPad *pad, GstBuffer *buffer, gpointer player_object);
    friend void parse_tag (const GstTagList *list, const char *tag, gpointer player_object);

    // END OF GST DATA AND FUNCTIONS

    // This is the incoming file the player should start operating on
    std::string mFilename;

    std::string mTrack;
    int mNumTracks;
    std::string mCDADiscid;
    std::vector <long long int> vTracks;

    long long int mStartms, mStopms;
    int mOpenRetries;
    time_t mOpentime;

    // Open function has recently been called
    bool bOpenSignal;
    bool bMutePlayback;

    // We should fade in the next few buffers
    bool bFadeIn;

    // Don't startseek since this is a continuation of the previous clip
    bool bContinuationClip;

    // This is the file the player is currently operating on
    std::string mPlayingFilename;
    long long int
        mPlayingms,       //Current ms in playing file
        mPlayingStartms,  //Start of current playing segment
        mPlayingStopms,   //Stop of current playing segment
        mPlayingEndms,    //End of current playing segment (from file)
        mUnderrunms;    //Position where underrun occurred

    gint64 position, duration;

    bool mPlayingWaiting; // Flag is set when we're waiting for new position
    bool mGotFinalVolume;
    bool bStartseek;
    bool bWaitAsync;
    float mPlayingVolume;
    float mCompressorRatio;
    float mCurrentVolume;
    float mAverageVolume;
    float mAverageFactor;
    float mCurrentdB;
    double mPlayingTempo;
    double mPlayingPitch;
    double mPlayingVolumeGain;
    double mPlayingTreble;
    double mPlayingBass;

    double mTempo;       // This is the playback speed
    double mPitch;      // This is the playback speed
    double mVolumeGain; // This is the volume gain on playback
    double mTreble;     // This is the treble gain
    double mBass;       // This is the bass gain

    void setEqualizer(GstElement *equalizer, double bass, double treble);

    void setRealState(GstState gstState, GstState gstPending);
    bool waitStateChange();

    playerState curState;
    playerState realState;
    std::string strState(playerState state);
    std::string shortstrState(playerState state);
    GstState mGstState;
    GstState mGstPending;


    bool setupThread();

    bool bDebugmode;
    bool getDebugmode();
    std::string mUseragent;
    std::string getUseragent();

    // Playback thread
    pthread_t playbackThread;

    pthread_mutex_t *stateMutex;    // Change of states
    pthread_mutex_t *dataMutex;     // Change mPlaying* data

    PlayerPosition pausePosition;
    bool serverTimedOut;


    bool bEOSCalledAlreadyForThisFile; // HACK for Higgins - Kovalla Kadella
};

#endif

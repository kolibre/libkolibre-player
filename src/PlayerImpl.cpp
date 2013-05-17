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


#include <string>
#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <iomanip>
#include <climits>
#include <cmath>
#include <ctime>
#include <unistd.h>
#include <log4cxx/logger.h>

#include "SmilTime.h"
#include "PlayerImpl.h"

//#define DEBUG 1
//#define DEBUG2 1

// Tweakable values
#define FADEIN_MS 50
#define SEEKMARGIN_MS 300
#define NEWPOSFLEX_MS 300
#define REOPEN_AFTER_PAUSING_SEC 240
#define PAUSE_SEEKS_BACKWARDS_MS -1000
#define CDA_READSPEED 12

#define levelInterval 100 * GST_MSECOND
#define levelPeakttl 1000 * GST_MSECOND
#define levelPeakfalloff 0.5

// helper function to build a clock string from a GstClockTime object
std::string gst_time_string(GstClockTime gstClockTime)
{
    if (not GST_CLOCK_TIME_IS_VALID (gstClockTime))
        return "99:99:99.999";

    // build a time string from the received clock time
    std::ostringstream oss;

    // hours
    int hours = gstClockTime / (GST_SECOND * 60 * 60);
    oss << hours;
    oss << ":";

    //minutes
    int minutes = (gstClockTime / (GST_SECOND * 60)) % 60;
    if (minutes < 10) oss << "0";
    oss << minutes;
    oss << ":";

    // seconds
    int seconds = (gstClockTime / (GST_SECOND)) % 60;
    if (seconds < 10) oss << "0";
    oss << seconds;
    oss << ".";

    // mseconds
    int mseconds = (gstClockTime / (GST_MSECOND)) % 1000;
    if (mseconds < 10) oss << "00";
    else if (mseconds < 100) oss << "0";
    oss << mseconds;

    return oss.str();
}

#define TIME_FORMAT "u:%02u:%02u.%03u"

#define TIME_ARGS(t) \
    GST_CLOCK_TIME_IS_VALID (t) ? \
(guint) (((GstClockTime)(t)) / (GST_SECOND * 60 * 60)) : 99, \
GST_CLOCK_TIME_IS_VALID (t) ? \
(guint) ((((GstClockTime)(t)) / (GST_SECOND * 60)) % 60) : 99, \
GST_CLOCK_TIME_IS_VALID (t) ? \
(guint) ((((GstClockTime)(t)) / GST_SECOND) % 60) : 99, \
GST_CLOCK_TIME_IS_VALID (t) ? \
(guint) (((GstClockTime)(t)) / GST_MSECOND % 1000) : 999

#define TIME_STR(t) gst_time_string(t)

// create a logger which will become a child to logger kolibre.player
log4cxx::LoggerPtr playerImplLog(log4cxx::Logger::getLogger("kolibre.player.playerimpl"));

char spinner[] = { '-', '\\', '|', '/' };

void *player_thread(void *player);
bool handle_bus_message(GstMessage *message, PlayerImpl *p);

#define bError true
#define bOk false

using namespace std;

/*
 * PlayerImpl private Player implementation
 */

/**
 * Player constructor
 */
PlayerImpl::PlayerImpl()
{
    // Setup mutexes and condition variable
    stateMutex = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
    pthread_mutex_init (stateMutex, NULL);

    dataMutex = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
    pthread_mutex_init (dataMutex, NULL);

    // Set the state to inactive
    curState = INACTIVE;
    realState = INACTIVE;

    mFilename = mPlayingFilename = "";
    mPlayingVolume = 2.5;
    mPlayingStartms = mStartms = mPlayingStopms = mStopms = 0;

    bOpenSignal = false;
    bMutePlayback = false;

    bDebugmode = false;
    mUseragent = "Kolibre/3";

    // NULL the callbackfuncs so they don't get called

    mPlayingTempo = mTempo = 1.0;
    mPlayingPitch = mPitch = 1.0;
    mPlayingVolumeGain = mVolumeGain = 1.0;
    mPlayingTreble = mTreble = 0.0;
    mPlayingBass = mBass = 0.0;
    mCurrentVolume = mPlayingVolume;
    mAverageVolume = 1.0;
    mAverageFactor = 0.5;
    mCurrentdB = 0.0;

    pipeType = NOPIPE;

    pBus = NULL;
    pPipeline = NULL;
    pDatasource = NULL;
    pCddasrc = NULL;

    pOggdemux = NULL;
    pVorbisdec = NULL;

    pFlump3dec = NULL;

    pFaaddec = NULL;

    pWavparse = NULL;

    pDecodebin = NULL;

    pQueue = NULL;
    pLevel = NULL;
    pAmplify = NULL;
    pAudioconvert1 = NULL;
    pPitch = NULL;
    pEqualizer = NULL;
    pAudioconvert2 = NULL;
    pAudiosink = NULL;

    serverTimedOut = false;
    bEOSCalledAlreadyForThisFile = false;
}

/**
 * Player destructor
 */
PlayerImpl::~PlayerImpl()
{
    LOG4CXX_TRACE(playerImplLog, "Destructor");

    // Tell the playbackThread to exit
    setState(EXITING);
    pthread_join (playbackThread, NULL);
    gst_deinit();
}

/**
 * Enable gstreamer and initialize the player control thread
 *
 * @param argc arguments passed on the command line
 * @param argv arguments passed on the command line
 *
 * @return false if OK true if ERROR
 */
bool PlayerImpl::enable(int *argc, char **argv[])
{
    var_argc = argc;
    var_argv = argv;
    LOG4CXX_INFO(playerImplLog, "Enabling player");

    switch (getState())
    {
        case INACTIVE:
            if(setupThread()) {
                LOG4CXX_ERROR(playerImplLog, "Failed to initialize thread");
                return bError;
            } else {
                int count = 0;
                while(count++ < 100 && getState() == INACTIVE) usleep(100000);

                if(getState() != EXITING) return bOk;
                return bError;
            }
            break;

        case EXITING:
            LOG4CXX_ERROR(playerImplLog, "Player in wrong state: '"<< getState_str() << "'");
            return bOk;

        default:
            LOG4CXX_ERROR(playerImplLog, "Player already enabled: '" << getState_str() << "'");
            return bOk;
    }
}

/**
 * Set the REAL state variable
 *
 * @param gstState real state
 * @param gstPending wanted state
 */
void PlayerImpl::setRealState(GstState gstState, GstState gstPending)
{
    lockMutex(stateMutex);

    // If we have no state currently pending, check what the real state is
    if(gstPending == GST_STATE_VOID_PENDING) {
        switch(gstState)
        {
            case GST_STATE_NULL:
            case GST_STATE_READY:
                realState = STOPPED;
                onPlayerState(realState);
                break;
            case GST_STATE_PAUSED:
                realState = PAUSING;
                onPlayerState(realState);
                break;
            case GST_STATE_PLAYING:
                realState = PLAYING;
                onPlayerState(realState);
                break;
            /*
            case GST_STATE_BUFFERING:
                realState = BUFFERING;
                onPlayerState(realState);
                break;
            */
            default:
                break;
        }
    }

    // If we have a pending state we must either be trying to open a file ..
    else if(gstPending == GST_STATE_PAUSED || gstPending == GST_STATE_PLAYING) {
        realState = BUFFERING;
        onPlayerState(realState);
    }

    // .. or trying to close it
    else if(gstPending == GST_STATE_READY)
        realState = STOPPED;

    unlockMutex(stateMutex);
}

/**
 * Get the state of the player
 *
 * @return the state of the player
 */
playerState PlayerImpl::getRealState()
{
    playerState state;
    lockMutex(stateMutex);
    state = realState;
    unlockMutex(stateMutex);
    return state;
}

/**
 * Set the state of the player
 *
 * @param state state to set
 */
void PlayerImpl::setState(playerState state)
{
    lockMutex(stateMutex);

    if(curState != EXITING)
        switch(state)
        {
            case STOPPED:
            case PAUSING:
            case PLAYING:
            case EXITING:
                curState = state;
                break;
            default:
                LOG4CXX_ERROR(playerImplLog, "Could not change state to to '" << strState(state) << "'");
                break;
        }
    else LOG4CXX_ERROR(playerImplLog, "Playerthread is exiting, could not change state to to '" << strState(state) << "'");

    unlockMutex(stateMutex);
}

/**
 * Get the state of the player
 *
 * @return the state of the player
 */
playerState PlayerImpl::getState()
{
    playerState state;
    lockMutex(stateMutex);
    state = curState;
    unlockMutex(stateMutex);
    return state;
}

/**
 * Convert a state into a string
 *
 * @param state the state to convert
 *
 * @return a string describing the state
 */
string PlayerImpl::strState(playerState state)
{
    switch(state)
    {
        case INACTIVE:  return "INACTIVE";   // Player has not yet been activated
        case BUFFERING: return "BUFFERING";  // Player is buffering the file
        case PLAYING:   return "PLAYING";    // Player is playing the file
        case PAUSING:   return "PAUSING";    // Player is pausing
        case STOPPED:   return "STOPPED";    // Player has stopped
        case EXITING:   return "EXITING";    // Playerthread should Setup
    }
    return "UNKNOWN";
}

/**
 * Convert a state into a string
 *
 * @param state the state to convert
 *
 * @return a string describing the state
 */
string PlayerImpl::shortstrState(playerState state)
{
    switch(state)
    {
        case INACTIVE:  return "N/A";    // Player has not yet been activated
        case BUFFERING: return "BUF";    // Player is buffering the file
        case PLAYING:   return "PLA";    // Player is playing the file
        case PAUSING:   return "PAU";    // Player is pausing
        case STOPPED:   return "STO";    // Player has stopped
        case EXITING:   return "EXI";    // Playerthread should Setup
    }
    return "UNK";
}

/**
 * Get the current state of the player as a string
 *
 *
 * @return string of player state
 */
string PlayerImpl::getState_str()
{
    playerState state = getState();
    return strState(state);
}

/**
 * Set the signal slot for when current file has problem buffering
 *
 * @param slot function pointer
 */
boost::signals2::connection PlayerImpl::doOnPlayerMessage(Player::OnPlayerMessage::slot_type slot)
{
    return onPlayerMessage.connect(slot);
}

/**
 * Set the signal slot for when player state changes
 *
 * @param slot function pointer
 */
boost::signals2::connection PlayerImpl::doOnPlayerState(Player::OnPlayerState::slot_type slot)
{
    return onPlayerState.connect(slot);
}

/**
 * Set the a signal slot for when player time changes
 *
 * @param slot function pointer
 */
boost::signals2::connection PlayerImpl::doOnPlayerTime(Player::OnPlayerTime::slot_type slot)
{
    return onPlayerTime.connect(slot);
}

/**
 * Send the audio-finished-playing signal
 *
 * @return TRUE if we should continue, FALSE if we should not
 */
bool PlayerImpl::sendCONTSignal()
{
    //Set mPlayWaiting before knowing the result of onPlayerMessage since a race condition might occur
    //and setting it to true after might be to late
    lockMutex(dataMutex);
    mPlayingWaiting = true;
    unlockMutex(dataMutex);

    bool result = *onPlayerMessage( Player::PLAYER_CONTINUE );

    //In case result is false reset it to false before returning
    if (not result){
        lockMutex(dataMutex);
        mPlayingWaiting = false;
        unlockMutex(dataMutex);
    }

    LOG4CXX_DEBUG(playerImplLog, "Sending 'Continue?' signal");
    //if ( not result ){
    //return *onSegmentFinishedPlaying();
    //}

    return result;
}

/**
 * Send the EOS signal
 *
 * @return ?
 */
bool PlayerImpl::sendEOSSignal()
{
    LOG4CXX_DEBUG(playerImplLog, "Sending 'EOS' signal");
    return onPlayerMessage( Player::PLAYER_ATEOS );
}

/**
 * Send the Buffering signal
 *
 * @return ?
 */
bool PlayerImpl::sendBUFFERINGSignal()
{
    LOG4CXX_WARN(playerImplLog, "Sending 'BUFFERING' signal");
    return onPlayerMessage( Player::PLAYER_BUFFERING );
}

/**
 * Call the Error signal
 *
 * @return ?
 */
bool PlayerImpl::sendERRORSignal()
{
    LOG4CXX_WARN(playerImplLog, "Sending 'ERROR' signal");
    return onPlayerMessage( Player::PLAYER_ERROR );
}

/**
 * Setup the playback thread
 *
 * @return TRUE if an error occurred FALSE if we're ok
 */
bool PlayerImpl::setupThread()
{
    LOG4CXX_DEBUG(playerImplLog, "Setting up playback thread");
    if(pthread_create(&playbackThread, NULL, player_thread, this)) {
        usleep(500000);
        return bError;
    }
    return bOk;
}

/**
 * Open a file and go to paused state
 *
 * @param filename URL of file to open
 * @param startms startms
 * @param stopms stopms
 */
void PlayerImpl::open(const char *filename, const char *startms, const char *stopms)
{
    SmilTimeCode startStop(startms, stopms);

    // This is just a conversion function
    open(filename, startStop.getStart(), startStop.getEnd());

}

/**
 * Open a file and go to paused state
 *
 * @param filename URL of file to open
 * @param startms startms
 * @param stopms stopms
 */
void PlayerImpl::open(string filename, long long startms, long long stopms)
{
    switch(getState())
    {
        case BUFFERING:
        case PAUSING:
        case PLAYING:
        case STOPPED:
            LOG4CXX_INFO(playerImplLog, "Opening '" << filename << "'");
            if(startms != 0 && stopms != UINT_MAX)
                LOG4CXX_INFO(playerImplLog, "from time " << TIME_STR((gint64)startms*GST_MSECOND) << " -> " << TIME_STR((gint64)stopms*GST_MSECOND));

            if(getState() != PLAYING) setState(PAUSING);

            lockMutex(dataMutex);
            mFilename = filename;
            mStartms = startms;
            mStopms = stopms;
            mUnderrunms = 0;
            mOpenRetries = 5;

            bOpenSignal = true;

            unlockMutex(dataMutex);
            usleep(100000);

            break;
        case INACTIVE:
            LOG4CXX_ERROR(playerImplLog, "player thread not yet started");
            break;

        case EXITING:
            LOG4CXX_ERROR(playerImplLog, "player thread exited");
            break;
    }
}

/**
 * Open a file and go to paused state
 *
 * @param filename URL of file to open
 */
void PlayerImpl::open(std::string filename) {
    open(filename, 0, UINT_MAX);
}

/**
 * Open a file and go to paused state
 *
 * @param filename URL of file to open
 * @param startms startms
 */
void PlayerImpl::open(string filename, long long startms)
{
    open(filename, startms, UINT_MAX);
}

/**
 * Seek to a position in the stream
 *
 * @param seektime ms to seek to
 */
void PlayerImpl::seekPos(long long int seektime)
{
    switch(getState())
    {
        case PAUSING:
        case PLAYING:
            {
                if(seektime < 0) seektime = 0;
                lockMutex(dataMutex);
                double tempo = mPlayingTempo;
                string filename = mFilename;
                bFadeIn = true;
                unlockMutex(dataMutex);

                gint64 c_seektime = (gint64) ((double)seektime / tempo) * GST_MSECOND;

                LOG4CXX_INFO(playerImplLog, "Seeking to " << seektime << " ms (" << c_seektime << ")");

                if (!gst_element_seek_simple (pPipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, c_seektime))
                {
                    LOG4CXX_ERROR(playerImplLog, "Seek to " << seektime << " in '" << filename << "' failed");
                    return;
                }
                else
                {
                    LOG4CXX_DEBUG(playerImplLog, "Seek OK");
                    lockMutex(dataMutex);
                    bFadeIn = true;
                    unlockMutex(dataMutex);
                }

                //usleep(500000);
                //LOG4CXX_WARN(playerImpleLog, "setting amplify to 0.0 between ("<< TIME_STR(o_seektime) << ") and (" << TIME_STR(o_seektime+500*GST_MSECOND) << ")");
                break;
            }
        default:
            LOG4CXX_INFO(playerImplLog, "Not seeking to " << seektime << " ms");
            break;
    }
}

/**
 * Stop playback
 *
 */
void PlayerImpl::stop()
{
    pausePosition = PlayerPosition(*this);
    lockMutex(dataMutex);
    mFilename = mPlayingFilename = "";
    mPlayingStartms = mStartms = mPlayingStopms = mStopms = 0;
    unlockMutex(dataMutex);

    setState(STOPPED);
}

/**
 * Reopen if stopped or if paused too long
 *
 */
void PlayerImpl::reopen()
{
    if( pausePosition.isValid() )
    {
        lockMutex(dataMutex);
        serverTimedOut = pausePosition.olderThan( REOPEN_AFTER_PAUSING_SEC );
        unlockMutex(dataMutex);
        pausePosition.open(*this, -1000);
    }
}

/**
 * Pause playback
 *
 */
void PlayerImpl::pause()
{
    playerState state = getState();

    switch(state)
    {
        case PLAYING:
            pausePosition = PlayerPosition(*this);
            setState(PAUSING);
            for(int c = 0; c < 10; c++)
                if(getRealState() == PAUSING || getRealState() == BUFFERING) break;
                else {
                    LOG4CXX_DEBUG(playerImplLog, "Waiting for player to enter PAUSING state");
                    usleep(100000);
                }
            break;
        default:
            if(state != PAUSING)
                LOG4CXX_WARN(playerImplLog, "Player in " << strState(state) << " state, could not pause");
    }
}

/**
 * Resume playback
 *
 */
void PlayerImpl::resume()
{
    LOG4CXX_INFO(playerImplLog, "resuming playback...");
    playerState state = getState();

    switch(state)
    {
        case STOPPED:
            LOG4CXX_DEBUG(playerImplLog, "Player in " << strState(state) << " state, Ready to continue playback");
            setState(PLAYING);
            break;
        case PAUSING:
            LOG4CXX_DEBUG(playerImplLog, "Player in " << strState(state) << " state, Ready to continue playback");
            setState(PLAYING);
            break;
        default:
            if(state != PLAYING)
                LOG4CXX_WARN(playerImplLog, "Player in " << strState(state) << " state, could not resume");
    }
}

/**
 * Get the current state of the player
 *
 * @return TRUE if we're playing, FALSE if not
 */
bool PlayerImpl::isPlaying() {
    playerState state = getState();

    if(state == PLAYING) return true;
    else return false;
}

/**
 * Get the current position in the playing file
 *
 * @return current file position (ms)
 */
long long int PlayerImpl::getPos()
{
    long long int ms = 0;

    switch(getState())
    {
        case PLAYING:
        case PAUSING:
            lockMutex(dataMutex);
            ms = mPlayingms;
            unlockMutex(dataMutex);
            return ms;
        default:
            LOG4CXX_WARN(playerImplLog, "getPos failed, player not in paused or playing state");
            return 0;
    }
}

/**
 * Get the current playing file
 *
 * @return current file
 */
string PlayerImpl::getFilename()
{
    string filename = "";

    switch(getState())
    {
        case PLAYING:
        case PAUSING:
            lockMutex(dataMutex);
            filename = mPlayingFilename;
            unlockMutex(dataMutex);
            return filename;
        default:
            LOG4CXX_WARN(playerImplLog, "getFilename failed, player not in paused or playing state");
            return "";
    }
}

/**
 * Get the current startms in the playing file
 *
 * @return current file startms (ms)
 */
long long int PlayerImpl::getStartms()
{
    long long int ms = 0;

    switch(getState())
    {
        case PLAYING:
        case PAUSING:
            lockMutex(dataMutex);
            ms = mStartms;
            unlockMutex(dataMutex);
            return ms;
        default:
            LOG4CXX_WARN(playerImplLog, "getStartms failed, player not in paused or playing state");
            return 0;
    }
}

/**
 * Get the current stopms in the playing file
 *
 * @return current file stopms (ms)
 */
long long int PlayerImpl::getStopms()
{
    long long int ms = 0;

    switch(getState())
    {
        case PLAYING:
        case PAUSING:
            lockMutex(dataMutex);
            ms = mStopms;
            unlockMutex(dataMutex);
            return ms;
        default:
            LOG4CXX_WARN(playerImplLog, "getStopms failed, player not in paused or playing state");
            return 0;
    }
}

/**
 * Get the currently opened track
 *
 * @return currently open track
 */
unsigned int PlayerImpl::getTrack()
{
    unsigned int track;
    lockMutex(dataMutex);
    track = atoi(mTrack.c_str());
    unlockMutex(dataMutex);
    return track;
}

/**
 * Get the number of tracks on this CD
 *
 * @return number of tracks
 */
unsigned int PlayerImpl::getNumTracks()
{
    unsigned int numTracks;
    lockMutex(dataMutex);
    numTracks = mNumTracks;
    unlockMutex(dataMutex);
    return numTracks;
}

/**
 * Return a vector containing the track lengths
 *
 * @return the tracks vector
 */
vector<long long int> PlayerImpl::getTracks()
{
    vector<long long int> tracks;
    lockMutex(dataMutex);
    tracks = vTracks;
    unlockMutex(dataMutex);
    return vTracks;
}

/**
 * Get the musicbrainz discid
 *
 * @return discid string
 */
string PlayerImpl::getCDADiscid()
{
    string id;
    lockMutex(dataMutex);
    id = mCDADiscid;
    unlockMutex(dataMutex);
    return id;
}

/**
 * Loads track data for an AudioCD
 *
 *
 * @return bOk if all went ok
 */
bool PlayerImpl::loadTrackData()
{
    GstFormat format;
    gint64 duration;
    int total_tracks = 0;
    int track;

    GstElement *pipeline, *cddasrc, *fakesink;

    vTracks.clear();

    LOG4CXX_INFO(playerImplLog, "Setting up temporary pipeline..");

    pipeline = gst_element_factory_make("pipeline", "Pipeline");
    cddasrc = gst_element_factory_make("cdparanoiasrc", "CDParanoiasrc");
    fakesink = gst_element_factory_make("fakesink", "Fakesink");

    if(!pipeline || !cddasrc || !fakesink) goto fail;

    gst_bin_add_many(GST_BIN(pipeline), cddasrc, fakesink, NULL);
    if(!gst_element_link_many(cddasrc, fakesink, NULL)) goto fail;

    if(gst_element_set_state(pipeline, GST_STATE_PAUSED) == FALSE) goto fail;

    // How many tracks do we have on this CD?
    format = gst_format_get_by_nick("track");

    if(gst_element_query_duration (cddasrc, &format, &duration))
        total_tracks = duration;

    if(duration == 0) goto fail;

    LOG4CXX_INFO(playerImplLog, "There are " << total_tracks << " tracks on this AudioCD");
    // Get the total time for each track
    for(track = 0; track < total_tracks; track++) {
        // Seek to the track to query
        format = gst_format_get_by_nick ("track");
        if (!gst_element_seek (pipeline, 1.0, format, GST_SEEK_FLAG_FLUSH,
                    GST_SEEK_TYPE_SET, track,

                    GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
            LOG4CXX_ERROR(playerImplLog, "Seek to track " << track << " failed");
        }
        duration = 0;

        format = GST_FORMAT_TIME;

        if(gst_element_query_duration (cddasrc, &format, &duration)){};
        LOG4CXX_DEBUG(playerImplLog, "Track " << track << " - " << TIME_STR(duration));
        vTracks.push_back(duration / GST_MSECOND);
    }

    if(gst_element_set_state(pipeline, GST_STATE_NULL) == FALSE) {
        LOG4CXX_ERROR(playerImplLog, "Unable to NULL pipeline");
        return false;
    }

    if(pipeline != NULL)
        gst_object_unref(GST_OBJECT(pipeline));

    return bOk;

fail:
    LOG4CXX_ERROR(playerImplLog, "Unable to retrieve track-data on AudioCD");
    if(pipeline != NULL)
        gst_object_unref(GST_OBJECT(pipeline));
    return bError;
}


/**
 * Adjust the playback tempo
 *
 * @param adjustment +/- tempo
 */
void PlayerImpl::adjustTempo(double adjustment)
{
    lockMutex(dataMutex);
    double value = mTempo + adjustment;
    unlockMutex(dataMutex);

    return setTempo(value);
}


/**
 * Get the current playback tempo
 *
 * @return tempo
 */
double PlayerImpl::getTempo()
{
    lockMutex(dataMutex);
    double tempo = mTempo;
    unlockMutex(dataMutex);

    return tempo;
}

/**
 * Set the playback tempo
 *
 * @param value tempo (between PLAYER_MAX_TEMPO and PLAYER_MIN_TEMPO)
 */
void PlayerImpl::setTempo(double value)
{
    if(value <= PLAYER_MIN_TEMPO) value = PLAYER_MIN_TEMPO;
    if(value >= PLAYER_MAX_TEMPO) value = PLAYER_MAX_TEMPO;
    lockMutex(dataMutex);
    mTempo = value;
    mPlayingTempo = value;
    unlockMutex(dataMutex);

    switch(getState())
    {
        case PAUSING:
        case PLAYING:
            if(pPitch != NULL)
            {
                LOG4CXX_INFO(playerImplLog, "setting tempo to: " << value);
                g_object_set(pPitch, "tempo", value, NULL);
            }
            break;
        default:
            break;
    }

    return;
}

/**
 * Adjust the playback pitch
 *
 * @param adjustment +/- pitch
 */
void PlayerImpl::adjustPitch(double adjustment)
{
    lockMutex(dataMutex);
    double value = mPitch + adjustment;
    unlockMutex(dataMutex);

    return setPitch(value);
}

/**
 * Get the current playback pitch
 *
 * @return pitch
 */
double PlayerImpl::getPitch()
{
    lockMutex(dataMutex);
    double pitch = mPitch;
    unlockMutex(dataMutex);

    return pitch;
}

/**
 * Set the playback pitch
 *
 * @param value pitch (between PLAYER_MAX_PITCH and PLAYER_MIN_PITCH)
 */
void PlayerImpl::setPitch(double value)
{
    if(value <= PLAYER_MIN_PITCH) value = PLAYER_MIN_PITCH;
    if(value >= PLAYER_MAX_PITCH) value = PLAYER_MAX_PITCH;
    lockMutex(dataMutex);
    mPitch = value;
    unlockMutex(dataMutex);

    switch(getState())
    {
        case PAUSING:
        case PLAYING:
            LOG4CXX_INFO(playerImplLog, "setting pitch to: " << value);
            break;
        default:
            break;
    }

    return;
}

/**
 * Adjust the playback volumegain
 *
 * @param adjustment +/- volumegain
 */
void PlayerImpl::adjustVolumeGain(double adjustment)
{
    lockMutex(dataMutex);
    double value = mVolumeGain + adjustment;
    unlockMutex(dataMutex);

    return setVolumeGain(value);
}

/**
 * Get the current playback volumegain
 *
 * @return volumegain
 */
double PlayerImpl::getVolumeGain()
{
    lockMutex(dataMutex);
    double volumegain = mVolumeGain;
    unlockMutex(dataMutex);

    return volumegain;
}

/**
 * Set the playback volumegain
 *
 * @param value volumegain (between PLAYER_MAX_VOLUMEGAIN and PLAYER_MIN_VOLUMEGAIN)
 */
void PlayerImpl::setVolumeGain(double value)
{
    if(value <= PLAYER_MIN_VOLUMEGAIN) value = PLAYER_MIN_VOLUMEGAIN;
    if(value >= PLAYER_MAX_VOLUMEGAIN) value = PLAYER_MAX_VOLUMEGAIN;
    lockMutex(dataMutex);
    mVolumeGain = value;
    unlockMutex(dataMutex);

    switch(getState())
    {
        case PAUSING:
        case PLAYING:
            LOG4CXX_INFO(playerImplLog, "setting volumegain to: " << value);
            break;
        default:
            break;
    }

    return;
}

/**
 * Adjust the playback treble
 *
 * @param adjustment +/- treble
 */
void PlayerImpl::adjustTreble(double adjustment)
{
    lockMutex(dataMutex);
    double value = mTreble + adjustment;
    unlockMutex(dataMutex);

    return setTreble(value);
}

/**
 * Get the current playback treble
 *
 * @return treble
 */
double PlayerImpl::getTreble()
{
    lockMutex(dataMutex);
    double treble = mTreble;
    unlockMutex(dataMutex);

    return treble;
}

/**
 * Set the playback treble
 *
 * @param value treble (between PLAYER_MAX_TREBLE and PLAYER_MIN_TREBLE)
 */
void PlayerImpl::setTreble(double value)
{
    if(value <= PLAYER_MIN_TREBLE) value = PLAYER_MIN_TREBLE;
    if(value >= PLAYER_MAX_TREBLE) value = PLAYER_MAX_TREBLE;
    lockMutex(dataMutex);
    mTreble = value;
    unlockMutex(dataMutex);

    switch(getState())
    {
        case PAUSING:
        case PLAYING:
            LOG4CXX_INFO(playerImplLog, "setting treble to: " << value);
            break;
        default:
            break;
    }

    return;
}

/**
 * Adjust the playback bass
 *
 * @param adjustment +/- bass
 */
void PlayerImpl::adjustBass(double adjustment)
{
    lockMutex(dataMutex);
    double value = mBass + adjustment;
    unlockMutex(dataMutex);

    return setBass(value);
}

/**
 * Get the current playback bass
 *
 * @return bass
 */
double PlayerImpl::getBass()
{
    lockMutex(dataMutex);
    double bass = mBass;
    unlockMutex(dataMutex);

    return bass;
}

/**
 * Set the playback bass
 *
 * @param value bass (between PLAYER_MAX_BASS and PLAYER_MIN_BASS)
 */
void PlayerImpl::setBass(double value)
{
    if(value <= PLAYER_MIN_BASS) value = PLAYER_MIN_BASS;
    if(value >= PLAYER_MAX_BASS) value = PLAYER_MAX_BASS;
    lockMutex(dataMutex);
    mBass = value;
    unlockMutex(dataMutex);

    switch(getState())
    {
        case PAUSING:
        case PLAYING:
            LOG4CXX_INFO(playerImplLog, "setting bass to: " << value);
            break;
        default:
            break;
    }

    return;
}

void PlayerImpl::setDebugmode(bool setting)
{
    lockMutex(dataMutex);
    bDebugmode = setting;
    unlockMutex(dataMutex);
}

bool PlayerImpl::getDebugmode()
{
    bool setting = false;
    lockMutex(dataMutex);
    setting = bDebugmode;
    unlockMutex(dataMutex);
    return setting;
}

void PlayerImpl::setUseragent(std::string useragent)
{
    lockMutex(dataMutex);
    mUseragent = useragent;
    unlockMutex(dataMutex);
}

string PlayerImpl::getUseragent()
{
    string useragent = "Kolibre";
    lockMutex(dataMutex);
    useragent = mUseragent;
    unlockMutex(dataMutex);
    return useragent;
}

/**
 * lockMutex function for debugging
 *
 * @param theMutex mutex to change
 */
void PlayerImpl::lockMutex(pthread_mutex_t *theMutex)
{
#ifdef DEBUG
    if(theMutex == stateMutex)
        LOG4CXX_WARN(playerImplLog, "Locking stateMutex");
    else if (theMutex == dataMutex)
        LOG4CXX_WARN(playerImplLog, "Locking dataMutex");
#endif

    pthread_mutex_lock(theMutex);
}

/**
 * unlockMutex function for debugging
 *
 * @param theMutex mutex to change
 */
void PlayerImpl::unlockMutex(pthread_mutex_t *theMutex)
{
#ifdef DEBUG
    if(theMutex == stateMutex)
        LOG4CXX_WARN(playerImplLog, "unLocking stateMutex");
    else if (theMutex == dataMutex)
        LOG4CXX_WARN(playerImplLog, "unLocking dataMutex");
#endif

    pthread_mutex_unlock(theMutex);
}

/**
 * Initialize GStreamer 0.10
 *
 * @return FALSE if we're OK, TRUE if there was an error
 */
bool PlayerImpl::initGstreamer()
{
    static bool gstreamer_initialized = false;

    // If we already called this function return 0
    if(gstreamer_initialized) return bOk;

    GError *error;

    if(gst_init_check(var_argc, var_argv, &error) == true) {
        gstreamer_initialized = true;
        return bOk;
    }


    LOG4CXX_ERROR(playerImplLog, "GStreamer init failed: '" << error->message << "'");
    g_error_free (error);

    return bError;
}

gboolean cb_data_probe (GstPad *pad, GstBuffer *buffer, gpointer player_object)
{
    PlayerImpl *p = (PlayerImpl *)player_object;
    static int fadeinms = 0;
    static gint64 skippedlength = 0;

    p->lockMutex(p->dataMutex);
    gint64 timestamp = (gint64) ((double)buffer->timestamp * p->mPlayingTempo);
    p->mPlayingms = ( (timestamp % GST_SECOND) / GST_MSECOND ) + ( (timestamp) / GST_SECOND * 1000);

    if(p->mPlayingms < p->mPlayingStartms-FADEIN_MS && p->mPlayingms + 5000 > p->mPlayingStartms) {
        p->unlockMutex(p->dataMutex);
        LOG4CXX_DEBUG(playerImplLog, "Skipping buffer " << TIME_STR(buffer->timestamp) <<  " -> " << TIME_STR(buffer->timestamp+buffer->duration));
        skippedlength += buffer->duration;
        return FALSE;
    } else if(skippedlength > 0) {
        LOG4CXX_DEBUG(playerImplLog, "Skipped seek margin " << TIME_STR(skippedlength));
        skippedlength = 0;
    }


#ifdef ENABLE_FADEIN
    if(p->bFadeIn) {
        p->bFadeIn = false;
        fadeinms = FADEIN_MS; // Milliseconds to apply fade to
    }

    // Fade in the first few buffers
    if(fadeinms > 0) {
        //int size =  GST_BUFFER_SIZE(buffer);

        int samples = GST_BUFFER_SIZE(buffer) / sizeof(short);
        gint64 lengthms = (gint64) ((double)buffer->duration * p->mPlayingTempo);
        lengthms = ( (lengthms % GST_SECOND) / GST_MSECOND ) + ( (lengthms) / GST_SECOND * 1000);

        gint64 startms = (gint64) ((double)buffer->timestamp * p->mPlayingTempo);
        startms = ( (startms % GST_SECOND) / GST_MSECOND ) + ( (startms) / GST_SECOND * 1000);

        float mspersample = (float) lengthms / (float) samples;

        //LOG4CXX_WARN(playerImpleLog, "Fading  " << fadeinms << " ms samples " << samples << ", pos " << startms << " duration " << lengthms << " ms/sample " << mspersample << " start at " << p->mPlayingStartms);

        short *data = (short *)GST_BUFFER_DATA(buffer);

        int fadecounter = 0;
        for (int i = 0; i < samples && (i*mspersample) < fadeinms; i++) {
            //printf("setting sample %d: %d -> %d\n",FADEIN_MS-fadecounter,
            //     data[i],
            //     (short)(data[i] * ((float) (FADEIN_MS-fadecounter) / (float) FADEIN_MS)));

            if(startms + ((float) i * mspersample) + (FADEIN_MS/2) < p->mPlayingStartms) {
                //if(i%10==0) LOG4CXX_WARN(playerImpleLog, "Nulling since " <<  (gint64) (startms + ((float) i * mspersample) + FADEIN_MS/2) << " < " << p->mPlayingStartms);
                data[i] = 0;
            } else {
                //if(i%10 == 0) LOG4CXX_WARN(playerImpleLog, "Fading sample to " << ((float) (FADEIN_MS-(fadeinms - (float)fadecounter*mspersample)) / (float) FADEIN_MS) << " since " << (gint64) (startms + ((float) i * mspersample) + FADEIN_MS/2) << " > " << p->mPlayingStartms);
                data[i] = (short)(data[i] * ((float) (FADEIN_MS-(fadeinms - (float)fadecounter*mspersample)) / (float) FADEIN_MS));
                fadecounter++;
            }
        }
        fadeinms -= (int)ceil((float) fadecounter * mspersample);
        if(fadeinms < 0) fadeinms = 0;

    }
#endif

    if(p->mPlayingms > p->mPlayingStopms // If we have played past the current segment
            && !p->mPlayingWaiting            // and we haven't yet called the continue callback
            //&& p->mOpentime != time(NULL)     // and opentime isn't now
            && !p->bMutePlayback              // and we arent't in mute mode
      ) {
        //Sometimes there is a delayed input making the cases below send us extra next commands. These will then make the reader leave out
        //beginning of sentences. Workaround is to check the goal state (p-getState()) and make sure we want it to be playing and sending
        //these commands. However, this needs robust testing.
        if(p->getState() != PLAYING){
            p->unlockMutex(p->dataMutex);
            return true;
        }

        if((p->position + 760 * GST_MSECOND) > p->duration) // Check that we aren't almost the very end of the current file
        {
            LOG4CXX_INFO(playerImplLog, "not calling continue callback at end of file " << p->position/GST_MSECOND << "/" << p->duration/GST_MSECOND);
            p->unlockMutex(p->dataMutex);
        } else {

            p->unlockMutex(p->dataMutex);
            if(p->sendCONTSignal()) {
                LOG4CXX_INFO(playerImplLog, "Continuing playback at: " << p->mPlayingms);
                return TRUE;
            } else {
                LOG4CXX_INFO(playerImplLog, "Starting to mute buffers");
                p->bMutePlayback = true;
            }
        }
    }


    if(p->bMutePlayback) {
        //int size =  GST_BUFFER_SIZE(buffer);
        LOG4CXX_INFO(playerImplLog, "Muting buffer");

        int samples = GST_BUFFER_SIZE(buffer) / sizeof(short);
        short *data = (short *)GST_BUFFER_DATA(buffer);

        for (int i = 0; i < samples; i++) {
            data[i] = 0;
        }
    }

    p->unlockMutex(p->dataMutex);

    return TRUE;
}

static gboolean cb_event_probe (GstPad *pad, GstEvent *event, gpointer player_object)
{
    //Player *p = (Player *)player_object;
    if(GST_EVENT_TYPE(event) == GST_EVENT_QOS) {
        gdouble proportion;
        GstClockTimeDiff diff;
        GstClockTime timestamp;
        gst_event_parse_qos(event, &proportion, &diff, &timestamp);

        LOG4CXX_DEBUG(playerImplLog, "Got QOS event proportion: " << proportion << ", diff " << diff << ", timestamp " << timestamp);


    } else LOG4CXX_DEBUG(playerImplLog, "Got event " << gst_event_type_get_name(GST_EVENT_TYPE(event)));



    return TRUE;
}

/**
 * Gstreamer callback for linking two GstPads
 *
 * @param templ ?
 * @param newpad The sourcepad
 * @param data The targetpad
 */
static void dynamic_link (GstPadTemplate * templ, GstPad * newpad, gpointer data)
{
    GstPad *target = (GstPad *) data;
    LOG4CXX_DEBUG(playerImplLog, "Linking elements '" << gst_element_get_name(gst_pad_get_parent(newpad)) << "' and '" << gst_element_get_name(gst_pad_get_parent(target)));
    gst_pad_link (newpad, target);
}

/**
 * Creates a source pipeline, for either http, https or file data source
 *
 * @return last element to link against
 */
GstElement *PlayerImpl::setupDatasource(GstBin *bin)
{
    enum { http, https, file } sourcetype;

    // Get the filename
    string filename = mPlayingFilename;

    // Remove leading and trailing whitespaces
    static const char whitespace[] = " \n\t\v\r\f";
    filename.erase( 0, filename.find_first_not_of(whitespace) );
    filename.erase( filename.find_last_not_of(whitespace) + 1U );

    // Transform filename into lowercase (for comparison)
    std::transform(filename.begin(), filename.end(),
            filename.begin(), (int(*)(int))tolower);

    // Check if we have a request for a https source
    if(filename.substr(0, 7) == "http://") sourcetype = http;

    // Check if we have a request for a http source
    else if(filename.substr(0, 8) == "https://") sourcetype = https;

    // otherwise assume it's a filesource
    else sourcetype = file;

    // Get the useragent string
    string useragent = getUseragent();
    bool debugmode = getDebugmode();

    // Setup the datasource depending on the sorucetype
    switch(sourcetype) {
        case http:
        case https:
            pDatasource = gst_element_factory_make("souphttpsrc", "pDatasource");
            g_object_set(pDatasource, "timeout", 5, NULL);
            if(pDatasource != NULL) {
                g_object_set(pDatasource, "user-agent", useragent.c_str(), NULL);
                if(debugmode) g_object_set(pDatasource, "soup-http-debug", 1, NULL);
            }
            break;
        default:
            pDatasource = gst_element_factory_make("filesrc", "pDatasource");
            break;
    }

    if(!pDatasource)
        goto fail;

    gst_bin_add(bin, pDatasource);

    // Setup the source location
    g_object_set(pDatasource, "location", mPlayingFilename.c_str(), NULL);

    return pDatasource;

fail:
    LOG4CXX_ERROR(playerImplLog, "filesrc:        " << (pDatasource ? "OK" : "failed"));
    return NULL;
}


#ifdef ENABLE_EQUALIZER
/**
 * Sets up the 10 band equalizer based on the bass and treble gain values
 *
 * @return null
 */
void PlayerImpl::setEqualizer(GstElement *element, double bass, double treble)
{
    if(element == NULL)
        LOG4CXX_ERROR(playerImplLog, "setEqualizer element was NULL");

    g_object_set(element,
            "band0",    bass * (4.0 * 0.25) + 0.3,
            "band1",    bass * (3.0 * 0.25) + 0.3,
            "band2",    bass * (2.0 * 0.25) + 0.3,
            "band3",    bass * (1.0 * 0.25) + 0.3,
            "band4",  treble * (1.0 * 0.166) + 0.3,
            "band5",  treble * (2.0 * 0.166) + 0.3,
            "band6",  treble * (3.0 * 0.166) + 0.3,
            "band7",  treble * (4.0 * 0.166) + 0.3,
            "band8",  treble * (5.0 * 0.166) + 0.3,
            "band9",  treble * (6.0 * 0.166) + 0.3, NULL);
}
#endif

/**
 * Creates an audio processing pipeline, tempo, amplify, compressor and sink elements
 *
 * @return first element to link against
 */
GstElement *PlayerImpl::setupPostprocessing(GstBin *bin)
{
    GstPad *pad;

    pAudioconvert1 = gst_element_factory_make("audioconvert", "pAudioconvert1");
    pPitch = gst_element_factory_make("pitch", "pPitch");
    pAmplify = gst_element_factory_make("audioamplify", "pAmplify");
#ifdef ENABLE_EQUALIZER
    pEqualizer = gst_element_factory_make("equalizer-10bands", "pEqualizer");
#endif
    pAudioconvert2 = gst_element_factory_make("audioconvert", "pAudioconvert2");
    pLevel = gst_element_factory_make("level", "pLevel");

#ifdef WIN32
    pAudiosink = gst_element_factory_make("directsoundsink", "pAudiosink");
    g_object_set (pAudiosink, "buffer-time", (gint64)500000, NULL);
    //g_object_set (pAudiosink, "slave-method", (gint64)1, NULL); // skew
    //g_object_set (pAudiosink, "preroll-queue-len", (gint64)50, NULL); // playback sometimes does not start
    //g_object_set (pAudiosink, "max-lateness", (gint64)10 * GST_MSECOND, NULL); // no effect?
#else
    pAudiosink = gst_element_factory_make("alsasink", "pAudiosink");
#endif
    // Check that the elements got set up
    if (!pAudioconvert1 ||
            !pPitch         ||
            !pAudioconvert2 ||
#ifdef ENABLE_EQUALIZER
            !pEqualizer     ||
#endif
            !pLevel         ||
            !pAmplify       ||
            !pAudiosink) goto fail;


    // Add the elements to the bin
    gst_bin_add_many (bin, pAudioconvert1, pPitch,
#ifdef ENABLE_EQUALIZER
            pEqualizer,
#endif
            pAudioconvert2,
            pLevel, pAmplify,
            pAudiosink, NULL);


    mPlayingTempo = mTempo;
    g_object_set(pPitch, "tempo", mPlayingTempo, NULL);

    mPlayingPitch = mPitch;
    g_object_set(pPitch, "pitch", mPlayingPitch, NULL);

#ifdef ENABLE_EQUALIZER
    mPlayingBass = mBass;
    mPlayingTreble = mTreble;
    setEqualizer(pEqualizer, mPlayingBass, mPlayingTreble);
#endif

    g_object_set(pLevel, "peak-ttl", levelPeakttl, NULL);
    g_object_set(pLevel, "interval", levelInterval, NULL);
    g_object_set(pLevel, "peak-falloff", levelPeakfalloff, NULL);

    mPlayingVolumeGain = mVolumeGain;
    g_object_set(pAmplify, "amplification", mPlayingVolume*mPlayingVolumeGain, NULL);

    //g_object_set(pAmplify, "amplification", 0.0, NULL);
    //g_object_set(pAmplify, "clipping-method",

    //g_object_set(pAudiosink, "qos", TRUE, NULL);
    //g_object_set(pAudiosink, "preroll-queue-len", 10, NULL);
    //g_object_set(pAudiosink, "max-lateness", 1500 * GST_MSECOND, NULL);
    g_object_set(pAudiosink, "sync", FALSE, NULL);
    //g_object_set(pAudiosink, "provide-clock", TRUE, NULL);

    // Link the elements
    if(!gst_element_link_many (pAudioconvert1, pPitch,
#ifdef ENABLE_EQUALIZER
                pEqualizer,
#endif
                pAudioconvert2,
                pLevel, pAmplify,
                pAudiosink, NULL)) goto fail;


    // Add a data probe
    pad = gst_element_get_pad (pAudiosink, "sink");
    gst_pad_add_buffer_probe (pad, G_CALLBACK (cb_data_probe), this);
    gst_pad_add_event_probe (pad, G_CALLBACK (cb_event_probe), this);
    gst_object_unref (pad);

    // Return the first element in this chain
    return pAudioconvert1;

fail:
    LOG4CXX_ERROR(playerImplLog, "audioconvert1:  " << (pAudioconvert1 ? "OK" : "failed"));
    LOG4CXX_ERROR(playerImplLog, "pitch:          " << (pPitch ? "OK" : "failed"));
#ifdef ENABLE_EQUALIZER
    LOG4CXX_ERROR(playerImplLog, "equalizer:      " << (pEqualizer ? "OK" : "failed"));
#endif
    LOG4CXX_ERROR(playerImplLog, "audioconvert2:  " << (pAudioconvert2 ? "OK" : "failed"));
    LOG4CXX_ERROR(playerImplLog, "level:          " << (pLevel ? "OK" : "failed"));
    LOG4CXX_ERROR(playerImplLog, "amplify:        " << (pAmplify ? "OK" : "failed"));
    LOG4CXX_ERROR(playerImplLog, "audiosink:      " << (pAudiosink ? "OK" : "failed"));

    return NULL;
}

/**
 * Creates an aac pipeline
 *
 * @return bOk if ok
 */
bool PlayerImpl::setupAACPipeline()
{
    GstElement *datasource = NULL;
    GstElement *postprocessing = NULL;

    // Create the pipeline
    pPipeline = gst_element_factory_make("pipeline", "pPipeline");
    pBus = gst_element_get_bus (GST_ELEMENT (pPipeline));

    if(!pPipeline) goto fail;

    // Setup datasource
    datasource = setupDatasource(GST_BIN(pPipeline));

    // Setup decoding
    pFaaddec = gst_element_factory_make("faad", "pFaaddec");
    gst_bin_add(GST_BIN(pPipeline), pFaaddec);

    // Setup postprocessing
    postprocessing = setupPostprocessing(GST_BIN(pPipeline));

    if (!datasource ||
            !pFaaddec ||
            !postprocessing) goto fail;

    // Add the elements to the pPipeline
    gst_element_link_many(datasource, pFaaddec, postprocessing, NULL);

    // We should now have the pipeline setup
    return bOk;

fail:
    LOG4CXX_ERROR(playerImplLog, "faaddec:        " << (pFaaddec ? "OK" : "failed"));
    LOG4CXX_ERROR(playerImplLog, "pipeline:       " << (pPipeline ? "OK" : "failed"));
    LOG4CXX_ERROR(playerImplLog, "Internal error, please check your GStreamer installation.");

    destroyPipeline();
    return bError;
}

/**
 * Creates an wav pipeline
 *
 * @return bOk if ok
 */
bool PlayerImpl::setupWAVPipeline()
{
    GstPad *pad;
    GstElement *datasource = NULL;
    GstElement *postprocessing = NULL;

    // Create the pipeline
    pPipeline = gst_element_factory_make("pipeline", "pPipeline");
    pBus = gst_element_get_bus (GST_ELEMENT (pPipeline));

    if(!pPipeline) goto fail;

    // Setup datasource
    datasource = setupDatasource(GST_BIN(pPipeline));

    // Setup decoding
    pWavparse = gst_element_factory_make("wavparse", "pWavparse");
    gst_bin_add(GST_BIN(pPipeline), pWavparse);

    // Setup postprocessing
    postprocessing = setupPostprocessing(GST_BIN(pPipeline));


    if (!datasource ||
            !pWavparse ||
            !postprocessing) goto fail;

    // Add the elements to the pPipeline
    gst_element_link_many(datasource, pWavparse, NULL);

    // Setup dynamic link
    pad = gst_element_get_pad(postprocessing, "sink");
    g_signal_connect(G_OBJECT(pWavparse), "pad-added", G_CALLBACK(dynamic_link), pad);
    gst_object_unref(pad);

    // We should now have the pipeline setup
    return bOk;

fail:
    LOG4CXX_ERROR(playerImplLog, "wavparse:       " << (pWavparse ? "OK" : "failed"));
    LOG4CXX_ERROR(playerImplLog, "pipeline:       " << (pPipeline ? "OK" : "failed"));
    LOG4CXX_ERROR(playerImplLog, "Internal error, please check your GStreamer installation.");
    destroyPipeline();

    return bError;
}

/**
 * Creates an mp3 pipeline
 *
 * @return bOk if ok
 */
bool PlayerImpl::setupMP3Pipeline()
{
    GstElement *datasource = NULL;
    GstElement *postprocessing = NULL;

    // Create the pipeline
    pPipeline = gst_element_factory_make("pipeline", "pPipeline");
    pBus = gst_element_get_bus (GST_ELEMENT (pPipeline));

    if(!pPipeline) goto fail;

    // Setup datasource
    datasource = setupDatasource(GST_BIN(pPipeline));

    // Setup decoding
    pFlump3dec = gst_element_factory_make("flump3dec", "pFlump3dec");
    gst_bin_add(GST_BIN(pPipeline), pFlump3dec);

    // Setup postprocessing
    postprocessing = setupPostprocessing(GST_BIN(pPipeline));


    if (!datasource ||
            !pFlump3dec ||
            !postprocessing) goto fail;

    // Add the elements to the pPipeline
    gst_element_link_many(datasource, pFlump3dec, postprocessing, NULL);

    // We should now have the pipeline setup
    return bOk;

fail:
    LOG4CXX_ERROR(playerImplLog, "flump3dec:      " << (pFlump3dec ? "OK" : "failed"));
    LOG4CXX_ERROR(playerImplLog, "pipeline:       " << (pPipeline ? "OK" : "failed"));
    LOG4CXX_ERROR(playerImplLog, "Internal error, please check your GStreamer installation.");
    destroyPipeline();

    return bError;
}

/**
 * Creates an ogg/vorbis pipeline
 *
 * @return bOk if ok
 */
bool PlayerImpl::setupOGGPipeline()
{
    GstPad *pad;
    GstElement *datasource = NULL;
    GstElement *postprocessing = NULL;

    // Create the pipeline
    pPipeline = gst_element_factory_make("pipeline", "pPipeline");
    pBus = gst_element_get_bus (GST_ELEMENT (pPipeline));

    if(!pPipeline) goto fail;

    // Setup datasource
    datasource = setupDatasource(GST_BIN(pPipeline));

    // Setup decoding
    pOggdemux = gst_element_factory_make("oggdemux", "pOggdemux");
    pVorbisdec = gst_element_factory_make("vorbisdec", "pVorbisdec");
    gst_bin_add_many(GST_BIN(pPipeline), pOggdemux, pVorbisdec, NULL);

    // Setup postprocessing
    postprocessing = setupPostprocessing(GST_BIN(pPipeline));

    if (!datasource     ||
            !pOggdemux      ||
            !pVorbisdec     ||
            !postprocessing) goto fail;

    // Link the elements
    if(!gst_element_link_many (pDatasource, pOggdemux, NULL)) goto fail;

    // Setup dynamic link
    pad = gst_element_get_pad(pVorbisdec, "sink");
    g_signal_connect(G_OBJECT(pOggdemux), "pad-added", G_CALLBACK(dynamic_link), pad);
    gst_object_unref(pad);

    // Link the other elements
    if(!gst_element_link_many (pVorbisdec, postprocessing, NULL)) goto fail;

    return bOk;

fail:
    LOG4CXX_ERROR(playerImplLog, "%oggdemux:       " << (pOggdemux ? "OK" : "failed"));
    LOG4CXX_ERROR(playerImplLog, "%vorbisdec:      " << (pVorbisdec ? "OK" : "failed"));
    LOG4CXX_ERROR(playerImplLog, "%Internal error, please check your GStreamer installation.");
    destroyPipeline();

    return bError;
}

/**
 * Creates a decodebin pipeline
 *
 * @return bOk if ok
 */
bool PlayerImpl::setupUnknownPipeline()
{
    GstPad *pad;
    GstElement *datasource = NULL;
    GstElement *postprocessing = NULL;

    // Create the pipeline
    pPipeline = gst_element_factory_make("pipeline", "pPipeline");
    pBus = gst_element_get_bus (GST_ELEMENT (pPipeline));

    if(!pPipeline) goto fail;

    // Setup datasource
    datasource = setupDatasource(GST_BIN(pPipeline));

    // Setup decoding
    pDecodebin = gst_element_factory_make("decodebin", "pDecodebin");
    gst_bin_add_many(GST_BIN(pPipeline), pDecodebin, NULL);

    // Setup postprocessing
    postprocessing = setupPostprocessing(GST_BIN(pPipeline));

    if (!datasource  ||
            !pDecodebin  ||
            !postprocessing) goto fail;


    // Link the elements
    if(!gst_element_link_many (pDatasource, pDecodebin, NULL)) goto fail;

    // Setup dynamic link
    pad = gst_element_get_pad(postprocessing, "sink");
    g_signal_connect(G_OBJECT(pDecodebin), "pad-added", G_CALLBACK(dynamic_link), pad);
    gst_object_unref (pad);

    return bOk;

fail:
    LOG4CXX_ERROR(playerImplLog, "pipeline:       " << (pPipeline ? "OK" : "failed"));
    LOG4CXX_ERROR(playerImplLog, "decodebin:      " << (pDecodebin ? "OK" : "failed"));
    LOG4CXX_ERROR(playerImplLog, "Internal error, please check your GStreamer installation.");
    destroyPipeline();

    return bError;
}

/**
 * Creates an AudioCD pipeline
 *
 * @return bOk if ok
 */
bool PlayerImpl::setupCDAPipeline()
{
    GstPad *pad;
    gint track;

    GstElement *postprocessing = NULL;

    // Create the pipeline
    pPipeline = gst_element_factory_make("pipeline", "pPipeline");
    pBus = gst_element_get_bus (GST_ELEMENT (pPipeline));

    if(!pPipeline) goto fail;

    // Setup datasource and a queue
    pCddasrc = gst_element_factory_make("cdparanoiasrc", "pCddasrc");
    pQueue = gst_element_factory_make("queue", "pQueue");

    // Setup postprocessing
    postprocessing = setupPostprocessing(GST_BIN(pPipeline));

    if (!pCddasrc       ||
            !pQueue         ||
            !postprocessing) goto fail;

    // Add the elements to the pPipeline
    gst_bin_add_many (GST_BIN(pPipeline), pCddasrc, pQueue,
            postprocessing, NULL);

    g_object_set(pQueue, "max-size-bytes", 262144, NULL);
    g_object_set(pQueue, "min-threshold-bytes", 65536, NULL);

    g_object_set (pCddasrc, "read-speed", CDA_READSPEED, NULL);

    // Link the elements
    if(!gst_element_link_many (pCddasrc, pQueue, postprocessing, NULL))
        goto fail;

    // Add a data probe
    pad = gst_element_get_pad (pAudiosink, "sink");
    gst_pad_add_buffer_probe (pad, G_CALLBACK (cb_data_probe), this);
    gst_pad_add_event_probe (pad, G_CALLBACK (cb_event_probe), this);
    gst_object_unref (pad);

    mNumTracks = 0;
    mCDADiscid = "";

    track = atoi(mTrack.c_str());
    g_object_set(pCddasrc, "mode", 0, NULL);
    g_object_set(pCddasrc, "track", track+1, NULL);

    return bOk;

fail:
    LOG4CXX_ERROR(playerImplLog, "pipeline:       " << (pPipeline ? "OK" : "failed"));
    LOG4CXX_ERROR(playerImplLog, "cddasrc:        " << (pCddasrc ? "OK" : "failed"));
    LOG4CXX_ERROR(playerImplLog, "queue:          " << (pQueue ? "OK" : "failed"));
    LOG4CXX_ERROR(playerImplLog, "Internal error, please check your GStreamer installation.");
    destroyPipeline();

    return bError;
}

/**
 * Destroys the current pipeline (or the parts of it that are created)
 *
 *
 * @return bOk if ok
 */
bool PlayerImpl::destroyPipeline()
{
    if (pPipeline != NULL) {
        GstObject *parent = NULL;

        //Reset the location so we close the file as soon as possible
        if(pDatasource != NULL) {
            g_object_set(pDatasource, "location", NULL, NULL);
        }
        LOG4CXX_DEBUG(playerImplLog, "Setting state to NULL");
        gst_element_set_state (GST_ELEMENT(pPipeline), GST_STATE_NULL);
        if(waitStateChange() == bError) usleep(3000000);

        if(pFlump3dec != NULL) parent = gst_element_get_parent(GST_OBJECT(pFlump3dec));
        if(pOggdemux != NULL) parent = gst_element_get_parent(GST_OBJECT(pOggdemux));
        if(pCddasrc != NULL) parent = gst_element_get_parent(GST_OBJECT(pCddasrc));
        if(pFaaddec != NULL) parent = gst_element_get_parent(GST_OBJECT(pFaaddec));
        if(pWavparse != NULL) parent = gst_element_get_parent(GST_OBJECT(pWavparse));
        if(pDecodebin != NULL) parent = gst_element_get_parent(GST_OBJECT(pDecodebin));

        if(parent == NULL) {
            LOG4CXX_ERROR(playerImplLog, "Destroying objects separately");
            if(pOggdemux != NULL) gst_object_unref(pOggdemux);
            if(pVorbisdec != NULL) gst_object_unref(pVorbisdec);
            if(pFlump3dec != NULL) gst_object_unref(pFlump3dec);
            if(pFaaddec != NULL) gst_object_unref(pFaaddec);
            if(pWavparse != NULL) gst_object_unref(pWavparse);
            if(pCddasrc != NULL) gst_object_unref(pCddasrc);
            if(pDecodebin != NULL) gst_object_unref(pDecodebin);
            if(pAudioconvert1 != NULL) gst_object_unref(pAudioconvert1);
            if(pPitch != NULL) gst_object_unref(pPitch);
#ifdef ENABLE_EQUALIZER
            if(pEqualizer != NULL) gst_object_unref(pEqualizer);
#endif
            if(pAudioconvert2 != NULL) gst_object_unref(pAudioconvert1);
            if(pLevel != NULL) gst_object_unref(pLevel);
            if(pQueue != NULL) gst_object_unref(pQueue);
            if(pAmplify != NULL) gst_object_unref(pAmplify);
            if(pAudiosink != NULL) gst_object_unref(pAudiosink);
            if(pBus != NULL) gst_object_unref(pBus);
            if(pPipeline != NULL) gst_object_unref(pAudiosink);
            LOG4CXX_DEBUG(playerImplLog, "Objects destroyed");

        } else {
            if(pBus != NULL) gst_object_unref(pBus);

            LOG4CXX_DEBUG(playerImplLog, "Destroying pipeline with refcount: " << GST_OBJECT_REFCOUNT(pPipeline));
            if(parent != NULL) gst_object_unref (parent);
            if(pPipeline != NULL) gst_object_unref (pPipeline);
        }

        if(GST_OBJECT_REFCOUNT(pPipeline) == 0)
        {
            LOG4CXX_DEBUG(playerImplLog, "Pipeline destroyed");
        }
        else
        {
            LOG4CXX_ERROR(playerImplLog, "Failed to destroy pipeline");
        }

    }

    // Pipeline and source
    pPipeline = NULL;
    pBus = NULL;
    pDatasource = NULL;

    // AudioCD source
    pCddasrc = NULL;

    // Queue element
    pQueue = NULL;

    // Decoder for ogg
    pOggdemux = NULL;
    pVorbisdec = NULL;

    // Decoder for mp3
    pFlump3dec = NULL;

    // Decoder for aac
    pFaaddec = NULL;

    // Decoder for wav
    pWavparse = NULL;

    // Decoder for other formats
    pDecodebin = NULL;

    // Postprocessing
    pAudioconvert1 = NULL;
    pPitch = NULL;
    pEqualizer = NULL;
    pAudioconvert2 = NULL;
    pLevel = NULL;
    pAmplify = NULL;
    pAudiosink = NULL;

    mNumTracks = 0;
    mCDADiscid = "";

    return bOk;
}

/**
 * Wait for gstreamer to change state
 *
 * @return bOk if ok
 */
bool PlayerImpl::waitStateChange()
{
    GstState curState, pendingState;
    int count = 0;
    int spincnt = 0;
    GstStateChangeReturn stateret;

    usleep(10000);
    if(pPipeline == NULL) return bError;

    for(count = 0; count < 1200; count++) {
        stateret = gst_element_get_state(pPipeline, &curState, &pendingState, 10 * GST_MSECOND);

        // Check the success codes
        if(stateret == GST_STATE_CHANGE_SUCCESS ||
                stateret == GST_STATE_CHANGE_ASYNC ||
                stateret == GST_STATE_CHANGE_NO_PREROLL) {

            if(pendingState == GST_STATE_VOID_PENDING) {
                LOG4CXX_DEBUG(playerImplLog, "State change OK");
                return bOk;
            }


            // Check the error codes
        } else if(stateret == GST_STATE_CHANGE_FAILURE) {

            const char *tmpstr = "";
            if(stateret == GST_STATE_CHANGE_FAILURE) {
                tmpstr = "GST_STATE_CHANGE_FAILURE";
            } else if(stateret == GST_STATE_CHANGE_SUCCESS) {
                tmpstr = "GST_STATE_CHANGE_SUCCESS";
            } else if(stateret == GST_STATE_CHANGE_ASYNC) {
                tmpstr = "GST_STATE_CHANGE_ASYNC";
            } else if(stateret == GST_STATE_CHANGE_NO_PREROLL) {
                tmpstr = "GST_STATE_CHANGE_NO_PREROLL";
            }

            LOG4CXX_ERROR(playerImplLog, "State change FAILED (" << tmpstr << ") while changing from " << gst_element_state_get_name(curState) << " to " << gst_element_state_get_name(pendingState));
            return bError;
        }

        printf("\r: Waiting for state change %s [%c]\r",
                gst_element_state_get_name(pendingState), spinner[spincnt]);
        spincnt++; if(spincnt > 3) spincnt = 0;
        usleep(10000);
    }

    const char *tmpstr = "";
    if(stateret == GST_STATE_CHANGE_FAILURE) {
        tmpstr = "GST_STATE_CHANGE_FAILURE";
    } else if(stateret == GST_STATE_CHANGE_SUCCESS) {
        tmpstr = "GST_STATE_CHANGE_SUCCESS";
    } else if(stateret == GST_STATE_CHANGE_ASYNC) {
        tmpstr = "GST_STATE_CHANGE_ASYNC";
    } else if(stateret == GST_STATE_CHANGE_NO_PREROLL) {
        tmpstr = "GST_STATE_CHANGE_NO_PREROLL";
    }

    LOG4CXX_ERROR(playerImplLog, "State change TIMEOUT (" << tmpstr << ") while changing from " << gst_element_state_get_name(curState) << " to " << gst_element_state_get_name(pendingState));
    return bError;
}

/**
 * Setup the pipeline for a specific type of file, reuse if possible
 *
 * @return FALSE if we're ok, TRUE if an error occurred
 */
bool PlayerImpl::setupPipeline()
{
    LOG4CXX_DEBUG(playerImplLog, "Setting up correct pipeline type");

    // Check the file extension, decide what kind of codec to use
    string file_ext = "unknown";
    string file_pre = "unknown";
    pipelineType newPipetype = NOPIPE;

    lockMutex(dataMutex);
    string filename = mPlayingFilename;
    unlockMutex(dataMutex);

    // Check if we have a request for an AudioCD track
    if(filename.length() > 8) {
        file_pre = filename.substr(0, 8);
        std::transform(file_pre.begin(), file_pre.end(),
                file_pre.begin(), (int(*)(int))tolower);
    }

    // Get the extension of a file
    if(file_pre == "audiocd:") {
        mTrack = filename.substr(8, filename.length());
        LOG4CXX_WARN(playerImplLog, "Got AudioCD '" << file_pre << "' track '" << mTrack<< "'");
    }    else {
        // Check extension if we don't have an audio CD
        mTrack = "";
        if(filename.length() > 4) {
            file_ext = filename.substr(filename.length()-4, filename.length());
            std::transform(file_ext.begin(), file_ext.end(),
                    file_ext.begin(), (int(*)(int))tolower);
            LOG4CXX_DEBUG(playerImplLog, "Got extension '" << file_ext << "'");
        }
    }

    // Setup the correct type of pipeline depending on the extension
    if(file_pre == "audiocd:") newPipetype = CDAPIPE;
    else if(file_ext == ".ogg") newPipetype = OGGPIPE;
    else if(file_ext == ".mp3") newPipetype = MP3PIPE;
    else if(file_ext == ".mpg") newPipetype = MP3PIPE;
    else if(file_ext == "mpeg") newPipetype = MP3PIPE;
    else if(file_ext == ".wav") newPipetype = WAVPIPE;
    else if(file_ext == ".aac") newPipetype = AACPIPE;
    else newPipetype = ANYPIPE;

    if(pPipeline != NULL) {
        gst_element_set_state(GST_ELEMENT(pPipeline), GST_STATE_NULL);
        if(waitStateChange() == bError) usleep(1000000);

        // Unlink elements
        switch(pipeType) {
            case OGGPIPE:
                LOG4CXX_DEBUG(playerImplLog, "Destroying OGGPIPE");
                destroyPipeline();
                break;

            case MP3PIPE:
                LOG4CXX_DEBUG(playerImplLog, "Destroying MP3PIPE");
                destroyPipeline();
                break;

            case AACPIPE:
                LOG4CXX_DEBUG(playerImplLog, "Destroying AACPIPE");
                destroyPipeline();
                break;

            case WAVPIPE:
                LOG4CXX_DEBUG(playerImplLog, "Destroying WAVPIPE");
                destroyPipeline();
                break;

            case CDAPIPE:
                LOG4CXX_DEBUG(playerImplLog, "Destroying CDAPIPE");
                destroyPipeline();
                break;

            case ANYPIPE:
                LOG4CXX_DEBUG(playerImplLog, "Destroying ANYPIPE");
                destroyPipeline();
                break;

            default:
                LOG4CXX_ERROR(playerImplLog, "Pipeline type not defined");
                break;
        }
    }


    lockMutex(stateMutex);
    realState = BUFFERING;
    unlockMutex(stateMutex);

    // Setup the new pipeline type
    switch(newPipetype) {
        case OGGPIPE:
            LOG4CXX_INFO(playerImplLog, "Setting up OGGPIPE");
            if(setupOGGPipeline() == bOk) {
                pipeType = newPipetype;
            } else {
                LOG4CXX_ERROR(playerImplLog, "ERROR SETTING UPP OGGPIPE");
                return bError;
            }

            break;

        case MP3PIPE:
            LOG4CXX_INFO(playerImplLog, "Setting up MP3PIPE");
            if(setupMP3Pipeline() == bOk) {
                pipeType = newPipetype;
            } else {
                LOG4CXX_ERROR(playerImplLog, "ERROR SETTING UPP MP3PIPE");
                return bError;
            }
            break;

        case AACPIPE:
            LOG4CXX_INFO(playerImplLog, "Setting up AACPIPE");
            if(setupAACPipeline() == bOk) {
                pipeType = newPipetype;
            } else {
                LOG4CXX_ERROR(playerImplLog, "ERROR SETTING UPP AACPIPE");
                return bError;
            }
            break;

        case WAVPIPE:
            LOG4CXX_INFO(playerImplLog, "Setting up WAVPIPE");
            if(setupWAVPipeline() == bOk) {
                pipeType = newPipetype;
            } else {
                LOG4CXX_ERROR(playerImplLog, "ERROR SETTING UPP WAVPIPE");
                return bError;
            }
            break;

        case CDAPIPE:
            LOG4CXX_INFO(playerImplLog, "Setting up CDAPIPE");
            if(setupCDAPipeline() == bOk)
                pipeType = newPipetype;
            else {
                LOG4CXX_ERROR(playerImplLog, "ERROR SETTING UPP CDAPIPE");
                return bError;
            }
            break;

        case ANYPIPE:
            LOG4CXX_INFO(playerImplLog, "Setting up ANYPIPE");
            if(setupUnknownPipeline() == bOk)
                pipeType = newPipetype;
            else {
                LOG4CXX_ERROR(playerImplLog, "ERROR SETTING UPP ANYPIPE");
                return bError;
            }
            break;

        default:
            LOG4CXX_ERROR(playerImplLog, "Pipeline type not defined");
            return bError;
            break;
    }

    LOG4CXX_DEBUG(playerImplLog, "Setting pipeline to READY");

    if(gst_element_set_state (GST_ELEMENT (pPipeline), GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE) {

        if(waitStateChange() == bError) usleep(1000000);
        LOG4CXX_DEBUG(playerImplLog, "Setting pipeline to READY OK");
    } else {
        LOG4CXX_DEBUG(playerImplLog, "Setting pipeline to READY ERROR");
        return bError;
    }

    return bOk;
}

/**
 * Tag parse function
 *
 * @param list taglist
 * @param tag the tag
 * @param player_object pointer to player object
 */
void parse_tag (const GstTagList *list, const char *tag, gpointer player_object)
{
    int count;
    const GValue *val;
    PlayerImpl *p = (PlayerImpl *)player_object;

    count = gst_tag_list_get_tag_size (list, tag);
    if (count < 1) {
        return;
    }

    val = gst_tag_list_get_value_index (list, tag, 0);

    p->lockMutex(p->dataMutex);
    if (!strcmp(tag, GST_TAG_TRACK_NUMBER)) { // Get the current track number
        char tmp[512];
        sprintf(tmp, "%d", g_value_get_uint(val) - 1);
        LOG4CXX_DEBUG(playerImplLog, "Track change from " << gst_tag_get_nick(tag) << " : " << p->mTrack << " -> " << tmp);
        p->mTrack = tmp;
    }
    else if(!strcmp(tag, GST_TAG_TRACK_COUNT)) { // Get the track count for the audiocd
        p->mNumTracks = g_value_get_uint(val);
        LOG4CXX_DEBUG(playerImplLog, "Track count from " << gst_tag_get_nick(tag) << ": " << p->mNumTracks);

    }
    else if(!strcmp(gst_tag_get_nick(tag), "musicbrainz-discid")) { // Get the discid
        char *tmp = g_strdup_value_contents(val);
        tmp[strlen(tmp)-1] = '\0';
        LOG4CXX_DEBUG(playerImplLog, "Got discid " << gst_tag_get_nick(tag) << ": " << tmp);
        p->mCDADiscid = tmp + 1;
        g_free(tmp);

    } else {
        switch(G_VALUE_TYPE(val)) {
            case G_TYPE_UINT:
                LOG4CXX_DEBUG(playerImplLog, "Got uint " << gst_tag_get_nick(tag) << ": " << g_value_get_uint(val));
                break;
            case G_TYPE_UINT64:
                LOG4CXX_DEBUG(playerImplLog, "Got uint64 " << gst_tag_get_nick(tag) << ": " << g_value_get_uint64(val));
                break;
            case G_TYPE_STRING:
                LOG4CXX_DEBUG(playerImplLog, "Got string " << gst_tag_get_nick(tag) << ": " << g_value_get_string(val));
                break;
            case G_TYPE_BOOLEAN:
                LOG4CXX_DEBUG(playerImplLog, "Got boolean " << gst_tag_get_nick(tag) << ": " << g_value_get_boolean(val));
                break;
            case G_TYPE_LONG:
                LOG4CXX_DEBUG(playerImplLog, "Got long " << gst_tag_get_nick(tag) << ": " << g_value_get_long(val));
                break;
            default:
                LOG4CXX_DEBUG(playerImplLog, "Got unknown type " << gst_tag_get_nick(tag) << ": " << g_strdup_value_contents(val));
                break;
        }
    }

    p->unlockMutex(p->dataMutex);


    /*
    if (IS_TAG (tag, GST_TAG_TITLE)) {
        //reader->title = g_value_dup_string (val);
        //g_print ("%s\n", reader->reader->title);
    }
    else if (IS_TAG (tag, GST_TAG_ALBUM)) {
        reader->album = g_value_dup_string (val);
    }
    else if (IS_TAG (tag, GST_TAG_ARTIST)) {
        reader->artist = g_value_dup_string (val);
    }
    else if (IS_TAG (tag, GST_TAG_GENRE)) {
        reader->genre = g_value_dup_string (val);
    }
    else if (IS_TAG (tag, GST_TAG_DURATION)) {
        reader->duration = g_value_get_uint64 (val) / GST_SECOND;
    }
    else if (IS_TAG (tag, GST_TAG_TRACK_NUMBER)) {
        reader->track = g_value_get_uint (val);
    }
    else if (IS_TAG (tag, GST_TAG_DATE)) {
        const GDate*date;
        GDateYear  year;

        date = gst_value_get_date (val);
        if (date) {
            year = g_date_get_year (gst_value_get_date (val));

            reader->year = year;
        }
    }
    */
    /* FIXME: more... */

}

/**
 * player control thread
 *
 * @param player instance
 *
 * @return NULL when finished
 */
void *player_thread(void *player)
{
    PlayerImpl *p = (PlayerImpl *)player;
    bool ret = 0;
    bool openNewFile = false;
    bool openNewPosition = false;
    p->bStartseek = false;

    double currentTempo = p->mPlayingTempo;
    double currentPitch;
    double currentVolumeGain;
    bool serverTimedOut = false;
#ifdef ENABLE_EQUALIZER
    double currentBass;
    double currentTreble;
#endif

    long lastPosms = 0;
    p->mGstState = GST_STATE_NULL;
    p->mGstPending = GST_STATE_VOID_PENDING;

    GstMessage *message;

    // Initialize gstreamer 0.10
    ret = p->initGstreamer();
    if(ret == bOk) {
        p->setState(STOPPED);
        p->setRealState(GST_STATE_NULL, GST_STATE_VOID_PENDING);
    } else {
        LOG4CXX_ERROR(playerImplLog, "FAILED TO SETUP GSTREAMER");
        p->setState(EXITING);
        p->setRealState(GST_STATE_NULL, GST_STATE_VOID_PENDING);
    }

    //return NULL;

    playerState state;
    state = p->getState();

    while(state != EXITING) {

        // Check if we should open a new file?
        p->lockMutex(p->dataMutex);
        serverTimedOut = p->serverTimedOut;
        p->serverTimedOut = false;
        //LOG4CXX_DEBUG(playerImpleLog, "Getting wanted filename");
        if(p->mFilename != p->mPlayingFilename || serverTimedOut) {
            printf("\n                                                                                       \r");
            // If we have are in PAUSING state, wait a couple of seconds to check
            // that this is really the file the user wants
            if(p->getState() == PAUSING) {
                string filename = "";
                int spincnt = 0;
                while(filename != p->mFilename) {
                    filename = p->mFilename;
                    // check if user is pressing next-next-next
                    p->unlockMutex(p->dataMutex);
                    for(int i = 0; i < 30 && !p->getState() != EXITING; i++) {
                        usleep(10000);
                        spincnt++; if(spincnt > 3) spincnt = 0;
                        printf("\rWaiting before committing [%c]      \r", spinner[spincnt]); fflush(stdout);
                    }
                    p->lockMutex(p->dataMutex);
                }
                printf("\rWaiting before committing [done]      \r");

                LOG4CXX_INFO(playerImplLog, "Got new Filename: '" << p->mFilename << "': " << TIME_STR(p->mStartms*GST_MSECOND) <<  "->" << TIME_STR(p->mStopms*GST_MSECOND));

                //} else {
                //LOG4CXX_WARN(playerImpleLog, "Got new Filename: '" << p->mFilename << "': '" << p->mStartms << "'->'" << p->mStopms << "'");

        }

        // Do not try to open an empty file
        if(p->mFilename != "")
            openNewFile = true;

        p->mPlayingFilename = p->mFilename;
        p->mPlayingStartms = p->mStartms;
        p->mPlayingStopms = p->mStopms;
        p->mPlayingWaiting = false;
        p->mPlayingms = 0;
        p->mOpentime = time(NULL);
        p->bOpenSignal = false;
        p->bMutePlayback = false;
        p->bFadeIn = true;
        p->bEOSCalledAlreadyForThisFile = false;

        } else if(p->mStartms != p->mPlayingStartms ||
                p->mStopms != p->mPlayingStopms ||
                p->bOpenSignal == true) {
            LOG4CXX_INFO(playerImplLog, "Got new Positions: '" << p->mFilename << "': " << TIME_STR(p->mStartms*GST_MSECOND) <<  "->'" << TIME_STR(p->mStopms*GST_MSECOND));

            // If this is a continuation clip,
            // and the mPlayingms is already in this clip -> don't seek
            if(p->mPlayingStopms == p->mStartms &&
                    p->mPlayingms >= p->mStartms &&
                    p->mPlayingms <= p->mStopms) {
                openNewPosition = false;
                // If eos has already been called for this file, call it again
                // This happens in "Kovalla Kädellä - Higgs" when open is called on the last segments, after EOS was sent.
                if( p->bEOSCalledAlreadyForThisFile ) {
                    p->unlockMutex(p->dataMutex);
                    LOG4CXX_WARN(playerImplLog, "Continue requested after EOS, resending EOS");
                    p->sendEOSSignal();
                    p->lockMutex(p->dataMutex);
                }
            } else {
                openNewPosition = true;
                p->bFadeIn = true;
            }

            p->mPlayingStartms = p->mStartms;
            p->mPlayingStopms = p->mStopms;
            p->mPlayingWaiting = false;
            p->mOpentime = time(NULL);
            p->bOpenSignal = false;
            p->bMutePlayback = false;
        }
        p->unlockMutex(p->dataMutex);

        // DO THE REQUIRED ACTIONS
        if(openNewFile) {
            // Open a new file
            openNewFile = false;
            p->setRealState(GST_STATE_READY, GST_STATE_PAUSED);
            if( p->setupPipeline() ){
                 LOG4CXX_ERROR(playerImplLog, "Failed to setup pipeline, player thread exiting!");
                 p->setState(EXITING);
                 continue;
            }
            p->lockMutex(p->dataMutex);
            if(p->mPlayingStartms != 0) p->bStartseek = true;
            p->mAverageFactor = 0.5;
            p->mAverageVolume = 1.0;
            p->mGotFinalVolume = false;
            p->unlockMutex(p->dataMutex);


        } else if(openNewPosition) {
            // Seek within the file
            openNewPosition = false;

            p->lockMutex(p->dataMutex);
            if(p->mPlayingms < p->mPlayingStartms || p->mPlayingms - NEWPOSFLEX_MS > p->mPlayingStartms) {
                if(p->mPlayingms < p->mPlayingStartms)
                    LOG4CXX_DEBUG(playerImplLog, "Startseek necessary since: " << p->mPlayingms << " < " << p->mPlayingStartms);

                if(p->mPlayingms - NEWPOSFLEX_MS > p->mPlayingStartms)
                    LOG4CXX_INFO(playerImplLog, "Startseek necessary since: " << p->mPlayingms - NEWPOSFLEX_MS << " > " << p->mPlayingStartms);

                LOG4CXX_DEBUG(playerImplLog, "Setting state to PAUSED");
                gst_element_set_state(p->pPipeline, GST_STATE_PAUSED);

                g_object_set(G_OBJECT (p->pAmplify), "amplification", p->mPlayingVolume*p->mPlayingVolumeGain, NULL);

                p->bStartseek = true;
            }
            p->unlockMutex(p->dataMutex);
        }

        // Handle messages here
        if(p->pBus && gst_bus_have_pending(p->pBus)){
            GstMessage *bus_message;
            bus_message = gst_bus_pop(p->pBus);
            handle_bus_message(bus_message, p);
            gst_message_unref(bus_message);
        }

        if(p->mGstPending == GST_STATE_VOID_PENDING) {

            // Get the Gstreamer current state
            switch(p->mGstState)
            {
                case GST_STATE_PAUSED:
                    {
                        switch(state)
                        {
                            case PLAYING:
                                // set gst state to playing or seek
                                if(p->getRealState() == BUFFERING) break;

                                if(!p->bStartseek) {
                                    LOG4CXX_DEBUG(playerImplLog, "Setting Realstate to PLAYING");
                                    gst_element_set_state(p->pPipeline, GST_STATE_PLAYING);
                                    p->mGstPending = GST_STATE_PLAYING;
                                } else {
                                    p->lockMutex(p->dataMutex);
                                    gint64 c_seektime = (gint64) ((double)p->mPlayingStartms / p->mPlayingTempo) * GST_MSECOND;

                                    // If jumping backwards, go to a point a littlebit before the seekpoint
                                    //if(p->mPlayingms > p->mPlayingStartms)

                                    c_seektime -= (SEEKMARGIN_MS * GST_MSECOND);
                                    if(c_seektime < 0) c_seektime = 0;

                                    LOG4CXX_INFO(playerImplLog, "Startseeking1 from " << TIME_STR(p->mPlayingms * GST_MSECOND) << " to " << TIME_STR(p->mPlayingStartms * GST_MSECOND) << " (" << TIME_STR(c_seektime) << ")");

                                    p->unlockMutex(p->dataMutex);

                                    if (!gst_element_seek_simple (p->pPipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, c_seektime))
                                    {
                                        LOG4CXX_ERROR(playerImplLog, "Seek failed");
                                    }
                                    else
                                    {
                                        LOG4CXX_DEBUG(playerImplLog, "Seek ok");
                                    }

                                    p->lockMutex(p->dataMutex);
                                    // Fade in the first few buffers
                                    p->bFadeIn = true;
                                    p->bStartseek = false;
                                    p->unlockMutex(p->dataMutex);
                                }

                                break;

                            case PAUSING:
                                // EXECUTE STARTSEEK
                                if(p->bStartseek) {
                                    p->lockMutex(p->dataMutex);
                                    gint64 c_seektime = (gint64) ((double)p->mPlayingStartms / p->mPlayingTempo) * GST_MSECOND;

                                    // If jumping backwards, go to a point a littlebit before the seekpoint
                                    //if(p->mPlayingms > p->mPlayingStartms)

                                    c_seektime -= (SEEKMARGIN_MS * GST_MSECOND);
                                    if(c_seektime < 0) c_seektime = 0;

                                    LOG4CXX_INFO(playerImplLog, "Startseeking2 from " << TIME_STR(p->mPlayingms * GST_MSECOND) << " to " << TIME_STR(p->mPlayingStartms * GST_MSECOND) << " (" << TIME_STR(c_seektime) << ")");

                                    p->unlockMutex(p->dataMutex);

                                    if (!gst_element_seek_simple (p->pPipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, c_seektime))
                                    {
                                        LOG4CXX_ERROR(playerImplLog, "Seek failed");
                                    }
                                    else
                                    {
                                        LOG4CXX_DEBUG(playerImplLog, "Seek OK");
                                    }

                                    p->lockMutex(p->dataMutex);
                                    // Fade in the first few buffers
                                    p->bFadeIn = true;
                                    p->bStartseek = false;
                                    p->unlockMutex(p->dataMutex);
                                }

                                // Check for speed change
                                p->lockMutex(p->dataMutex);
                                if(p->mTempo != p->mPlayingTempo || p->mPitch != p->mPlayingPitch) {

                                    p->mPlayingStartms = -1;

                                    currentTempo = p->mPlayingTempo = p->mTempo;
                                    currentPitch = p->mPlayingPitch = p->mPitch;
                                    p->unlockMutex(p->dataMutex);

                                    LOG4CXX_INFO(playerImplLog, "Setting tempo to: '" << currentTempo << "'");
                                    g_object_set(p->pPitch, "tempo", currentTempo, NULL);

                                    LOG4CXX_INFO(playerImplLog, "Setting pitch to: '" << currentPitch << "'");
                                    g_object_set(p->pPitch, "pitch", currentPitch, NULL);

                                    g_object_set(G_OBJECT (p->pAmplify), "amplification", p->mPlayingVolume*p->mPlayingVolumeGain, NULL);

                                } else {
                                    p->unlockMutex(p->dataMutex);
                                }

#ifdef ENABLE_EQUALIZER
                                // Check for bass or treble change
                                p->lockMutex(p->dataMutex);
                                if(p->mBass != p->mPlayingBass || p->mTreble != p->mPlayingTreble) {

                                    currentBass = p->mPlayingBass = p->mBass;
                                    currentTreble = p->mPlayingTreble = p->mTreble;
                                    p->unlockMutex(p->dataMutex);

                                    LOG4CXX_INFO(playerImplLog, "Setting bass to: '" << currentBass << "'");
                                    LOG4CXX_INFO(playerImplLog, "Setting treble to: '" << currentTreble << "'");
                                    p->setEqualizer(p->pEqualizer, currentBass, currentTreble);
                                } else {
                                    p->unlockMutex(p->dataMutex);
                                }
#endif

                                // Check for volume gain change
                                p->lockMutex(p->dataMutex);
                                if(p->mVolumeGain != p->mPlayingVolumeGain) {

                                    currentVolumeGain = p->mPlayingVolumeGain = p->mVolumeGain;
                                    float volume = p->mPlayingVolume;
                                    p->unlockMutex(p->dataMutex);

                                    LOG4CXX_INFO(playerImplLog, "Setting volumegain to: '" << currentVolumeGain << "'");
                                    g_object_set(G_OBJECT (p->pAmplify), "amplification", volume*currentVolumeGain, NULL);
                                } else {
                                    p->unlockMutex(p->dataMutex);
                                }
                                break;

                            case STOPPED:
                                LOG4CXX_DEBUG(playerImplLog, "Setting Realstate to READY");
                                gst_element_set_state(p->pPipeline, GST_STATE_READY);
                                if(p->waitStateChange() == bError) usleep(1000000);
                                p->mGstPending = GST_STATE_READY;
                                break;

                            default:
                                break;
                        }

                        break;
                    }

                case GST_STATE_PLAYING:
                    {
                        switch(state)
                        {
                            case PLAYING:
                                break;
                            case PAUSING:
                                LOG4CXX_DEBUG(playerImplLog, "Setting Realstate to PAUSED");
                                gst_element_set_state(p->pPipeline, GST_STATE_PAUSED);
                                p->mGstPending = GST_STATE_PAUSED;
                                // seek backwards to compensate for fadein and half frames
                                p->mStartms = max(p->mPlayingms+PAUSE_SEEKS_BACKWARDS_MS,p->mStartms);
                                p->mPlayingStartms = -1;

                                break;
                            case STOPPED:
                                LOG4CXX_DEBUG(playerImplLog, "Setting Realstate to READY");
                                gst_element_set_state(p->pPipeline, GST_STATE_READY);
                                if(p->waitStateChange() == bError) usleep(1000000);
                                p->mGstPending = GST_STATE_READY;
                                break;
                            default:
                                break;
                        }

                        break;
                    }

                case GST_STATE_READY:
                    {
                        switch(state)
                        {
                            case PLAYING:
                                break;
                            case PAUSING:
                                break;
                            case STOPPED:
                                break;
                            default:
                                break;
                        }
                        break;
                    }

                default:
                    break;
            }
        }

        GstFormat fmt = GST_FORMAT_TIME;

        if(GST_IS_ELEMENT(p->pPipeline) && (state == PLAYING || state == PAUSING) && //) {
            //if (
            gst_element_query_position (p->pPipeline, &fmt, &p->position)
                //&& gst_element_query_duration (p->pPipeline, &fmt, &p->duration)
                ) {
                    //g_print ("\rTime: %" TIME_FORMAT " / %" TIME_FORMAT ,
                    //             TIME_ARGS (pos), TIME_ARGS (len));
                    p->lockMutex(p->dataMutex);
                    //cerr << "Pos: " << (p->position * p->mPlayingTempo) / GST_MSECOND << "(" << p->mPlayingms << ") / "
                    //         << (p->duration * p->mPlayingTempo) / GST_MSECOND << " @ " << currentVolume << "       \r" << flush;

                    //double amplify = 0.0;
                    //g_object_get(G_OBJECT(p->pAmplify),"amplification",&amplify,NULL);

                    if(lastPosms != p->position / 100) {
                        Player::timeData td;
                        // Scale current position and duration according to currentTempo
                        td.current = (double(p->position) * currentTempo) / GST_MSECOND;
                        td.duration = (double(p->duration) * currentTempo) / GST_MSECOND;
                        td.segmentstart = p->mPlayingStartms;
                        td.segmentstop = p->mPlayingStopms;
                        p->onPlayerTime(td);
                        lastPosms = p->position / 100;
                    }

#ifdef WIN32
                    /*
                    printf("%s(%s) %" TIME_FORMAT " (%" TIME_FORMAT") "
                            "/ %" TIME_FORMAT" (-%" TIME_FORMAT ") %s:%2.2f\r",
                            p->shortstrState(p->getRealState()).c_str(),
                            p->shortstrState(state).c_str(),
                            TIME_ARGS((gint64) (p->position)),
                            TIME_ARGS((gint64) (p->mPlayingms * GST_MSECOND)),
                            TIME_ARGS((gint64) (p->duration)),
                            TIME_ARGS((gint64) ((p->mPlayingStopms - p->mPlayingms) * GST_MSECOND)),
                            (averageFactor > 0.01) ? "Vc" : "V",
                            p->mPlayingVolume,
                            currentdB
                            );
                    */
#else
                    printf("%s(%s) %" TIME_FORMAT " (%" TIME_FORMAT") "
                            "/ %" TIME_FORMAT" (-%" TIME_FORMAT ") (%s:%2.2f D:%2.2f)           \r",
                            p->shortstrState(p->getRealState()).c_str(),
                            p->shortstrState(state).c_str(),
                            TIME_ARGS((gint64) (p->position)),
                            TIME_ARGS((gint64) (p->mPlayingms * GST_MSECOND)),
                            TIME_ARGS((gint64) (p->duration)),
                            TIME_ARGS((gint64) ((p->mPlayingStopms - p->mPlayingms) * GST_MSECOND)),
                            (p->mAverageFactor > 0.01) ? "Vc" : "V",
                            p->mPlayingVolume,
                            p->mCurrentdB
                            );
                    fflush(stdout);
#endif
                    //cerr << "Pos: " << (p->position * p->mPlayingTempo) / GST_MSECOND << "(" << p->mPlayingms << ") / "
                    //     << (duration * p->mPlayingTempo) / GST_MSECOND << " @ " << currentVolume << "       \r" << flush;
                    p->unlockMutex(p->dataMutex);
                    //}
                }

                /*
                else
                {
                    printf("RealState: %s FakeState %s | Pos: unknown / unknown           \r",
                    p->strState(p->getRealState()).c_str(),
                    p->strState(state).c_str());
                }
                */

        /*
        if(p->pPipeline != NULL) {
            gst_element_query_position (p->pPipeline, time_format, &p->position);
            gst_element_query_duration (p->pPipeline, time_format, &p->duration);

            g_print ("Pipeline time: %" TIME_FORMAT " / %" TIME_FORMAT "\n",
            TIME_ARGS (p->position), TIME_ARGS (p->duration));
        }
        */

        usleep(10000);
        state = p->getState();
    }

    LOG4CXX_WARN(playerImplLog, "Shutting down playbackthread");
    //if(p->pCddasrc != NULL) {
    //LOG4CXX_WARN(playerImpleLog, "Not destroying AudioCD pipeline");
    //gst_element_set_state(p->pPipeline, GST_STATE_PAUSED);
    //} else {
    p->destroyPipeline();
    //}

    return NULL;
}

bool handle_bus_message(GstMessage *message, PlayerImpl *p){
    // Check and process messages from GStreamer
    bool ret;
    LOG4CXX_TRACE(playerImplLog, "Got message from " << gst_element_get_name(GST_MESSAGE_SRC(message)) << " type: " << gst_message_type_get_name(GST_MESSAGE_TYPE(message)));
    if(p->pBus != NULL) {

        switch (message->type) {
            case GST_MESSAGE_EOS:
                LOG4CXX_WARN(playerImplLog, "Recieved EOS at " << TIME_STR((gint64) (p->position)) << " (" << TIME_STR((gint64) (p->mPlayingms * GST_MSECOND)) << ") / " << TIME_STR((gint64) (p->duration)) << " (-" << TIME_STR((gint64) ((p->mPlayingStopms - p->mPlayingms) * GST_MSECOND)) << ")");

                ret = false;
                p->lockMutex(p->dataMutex);
                if(p->mPlayingFilename == "reopening") ret = true;
                p->unlockMutex(p->dataMutex);

                if(ret)
                {
                    LOG4CXX_WARN(playerImplLog, "Not calling EOS callback, reopening file");
                }
                else
                {
                    p->lockMutex(p->dataMutex);
                    p->bEOSCalledAlreadyForThisFile = true;
                    p->unlockMutex(p->dataMutex);

                    p->sendEOSSignal();
                }

                break;

            case GST_MESSAGE_WARNING:
            case GST_MESSAGE_ERROR:
                {
                    GError *gerror;
                    gchar *debug;

                    string type = "None";

                    if(message->type == GST_MESSAGE_ERROR) {
                        type = "Error";
                        gst_message_parse_error (message, &gerror, &debug);
                    } else {
                        type = "Warning";
                        gst_message_parse_warning(message, &gerror, &debug);
                    }

                    //gst_object_default_error (GST_MESSAGE_SRC (message), gerror, debug);

                    LOG4CXX_ERROR(playerImplLog, type << " from: " << gst_object_get_name(GST_MESSAGE_SRC(message)) << " '" << gerror->message << "'(" << gerror->code << ") '" << debug << "'");

                    // Workaround for bug..?
                    if(GST_MESSAGE_SRC(message) == GST_OBJECT(p->pAudiosink)) {
                        LOG4CXX_WARN(playerImplLog, "Audiosink error, setting state to PLAYING");
                        gst_element_set_state(p->pPipeline, GST_STATE_PLAYING);
                        p->mGstPending = GST_STATE_PLAYING;
                    }

                    // If we recieved an error from datasource retry opening the file
                    if(GST_MESSAGE_SRC(message) == GST_OBJECT(p->pDatasource)) {

                        // Get variables needed
                        p->lockMutex(p->dataMutex);
                        long long int lastplayedms;
                        if(p->mPlayingms > p->mUnderrunms) lastplayedms = p->mUnderrunms = p->mPlayingms;
                        else lastplayedms = p->mUnderrunms;
                        string filename = p->mPlayingFilename;
                        int retries = p->mOpenRetries;
                        time_t retrytime = p->mOpentime;
                        p->unlockMutex(p->dataMutex);

                        if(retries == 0) {
                            LOG4CXX_ERROR(playerImplLog, "Number of retires exceeded, calling ERROR callback");
                            if(p->sendERRORSignal() == false) {
                                // Set state to pausing
                                p->setState(PAUSING);

                                // Call EOS callback
                                p->sendEOSSignal();
                            }
                            p->mOpenRetries--;
                        } else if(retries > 0) {
                            // Call the buffering callback, set state to paused
                            if(p->sendBUFFERINGSignal() && filename != "reopening") {

                                // Set state to pausing
                                p->setState(PAUSING);

                                // Try reopening from last played position
                                LOG4CXX_ERROR(playerImplLog, "Reopening " << filename << " at " << lastplayedms << " ms (" << retries -1 << " more retries)");

                                p->lockMutex(p->dataMutex);
                                p->mPlayingFilename = "reopening";
                                p->mStartms = lastplayedms;
                                p->mOpenRetries--;
                                p->unlockMutex(p->dataMutex);

                                // Check retrytime, wait if less than 10 seconds
                                int spincnt = 0;
                                if(retrytime + 10 > time(NULL)) {
                                    LOG4CXX_ERROR(playerImplLog, "Reopening too fast, waiting..");
                                    for(int i = 0; i < 100 && !p->getState() != EXITING; i++) {
                                        usleep(50000);
                                        spincnt++; if(spincnt > 3) spincnt = 0;
                                        printf("\rWaiting to reopen [%c]      \r",
                                                spinner[spincnt]); fflush(stdout);
                                    }
                                    printf("\rWaiting to reopen [done]\n");
                                }
                            } else {
                                LOG4CXX_WARN(playerImplLog, "Application handled error");
                                p->setState(STOPPED);
                            }
                        }
                    }

                    g_error_free (gerror);
                    g_free (debug);
                    break;
                }

            case GST_MESSAGE_STATE_CHANGED:
                {
                    if(GST_MESSAGE_SRC(message) == (GstObject*)p->pPipeline) {
                        GstState oldstate;
                        gst_message_parse_state_changed (message, &oldstate, &p->mGstState, &p->mGstPending);

                        // Report state changes from PAUSED TO PLAYING AND PLAYING TO PAUSED

                        if(p->mGstPending == GST_STATE_VOID_PENDING &&
                                (oldstate == GST_STATE_PLAYING || oldstate == GST_STATE_PAUSED ||
                                 p->mGstState == GST_STATE_PLAYING || p->mGstState == GST_STATE_PLAYING)) {
                            LOG4CXX_INFO(playerImplLog, gst_element_state_get_name(oldstate) << " -> " << gst_element_state_get_name(p->mGstState) << " at " << TIME_STR((gint64) (p->position)) <<  " (" << TIME_STR((gint64) (p->mPlayingms * GST_MSECOND)) << ") / " << TIME_STR((gint64) (p->duration)) << " (-" << TIME_STR((gint64) ((p->mPlayingStopms - p->mPlayingms) * GST_MSECOND)) << ")");
                        } else {
                            LOG4CXX_DEBUG(playerImplLog, gst_element_state_get_name(oldstate) << " -> " << gst_element_state_get_name(p->mGstState) << " (pending: " << gst_element_state_get_name(p->mGstPending) << ")");
                        }

                        // Query duration if we come out of PAUSED state
                        GstFormat fmt = GST_FORMAT_TIME;
                        if(p->mGstPending == GST_STATE_VOID_PENDING &&  oldstate == GST_STATE_PAUSED) {
                            gst_element_query_duration (p->pPipeline, &fmt, &p->duration);
                        }


                        // Store the actual pipeline state in the class variable
                        p->setRealState(p->mGstState, p->mGstPending);

                        // EXECUTE STARTSEEK
                        if(p->bStartseek &&
                                // p->mGstState == GST_STATE_PAUSED &&
                                p->mGstPending == GST_STATE_VOID_PENDING) {
                            p->lockMutex(p->dataMutex);
                            gint64 c_seektime = (gint64) ((double)p->mPlayingStartms / p->mPlayingTempo) * GST_MSECOND;

                            // If jumping backwards, go to a point a littlebit before the seekpoint
                            //if(p->mPlayingms > p->mPlayingStartms)

                            c_seektime -= (SEEKMARGIN_MS * GST_MSECOND);
                            if(c_seektime < 0) c_seektime = 0;

                            LOG4CXX_INFO(playerImplLog, "Startseeking3 from " << TIME_STR(p->mPlayingms * GST_MSECOND) << " to " << TIME_STR(p->mPlayingStartms * GST_MSECOND) << " (" << TIME_STR(c_seektime) << ")");

                            // Fade in the first few buffers
                            p->bFadeIn = true;
                            p->bStartseek = false;
                            p->unlockMutex(p->dataMutex);

                            if (!gst_element_seek_simple (p->pPipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, c_seektime))
                            {
                                LOG4CXX_ERROR(playerImplLog, "Seek failed");
                            }
                            else
                            {
                                LOG4CXX_DEBUG(playerImplLog, "Seek OK");
                            }


                        }
                    }
                    break;
                }

            case GST_MESSAGE_TAG:
                {
                    GstTagList *tags;

                    LOG4CXX_DEBUG(playerImplLog, "Got tag message from " << gst_element_get_name(GST_MESSAGE_SRC(message)));

                    gst_message_parse_tag (message, &tags);
                    gst_tag_list_foreach (tags, (GstTagForeachFunc)parse_tag, p);

                    gst_tag_list_free (tags);
                    break;
                }
            case GST_MESSAGE_ELEMENT:
                {
                    const GstStructure *s = gst_message_get_structure (message);
                    const gchar *name = gst_structure_get_name (s);

                    if (strcmp (name, "level") == 0) {
                        gint channels;
                        GstClockTime endtime;
                        gdouble //rms_dB, peak_dB,
                                decay_dB;
                        //gdouble rms, peak, decay;

                        gdouble currentVol = 1.0;

                        const GValue *list;
                        const GValue *value;
                        gint i;
                        if (!gst_structure_get_clock_time (s, "endtime", &endtime))
                            g_warning ("Could not parse endtime");
                        /* we can get the number of channels as the length of any of the value lists */

                        list = gst_structure_get_value (s, "decay");
                        channels = gst_value_list_get_size (list);
                        //g_print ("endtime: %" TIME_FORMAT ", channels: %d\n", TIME_ARGS (endtime), channels);
                        for (i = 0; i < channels; ++i) {
                            list = gst_structure_get_value (s, "decay");
                            value = gst_value_list_get_value (list, i);
                            decay_dB = g_value_get_double (value);
                            if(decay_dB < -20.0) decay_dB = -20.0;

                            currentVol = (pow(20, -decay_dB / 25));
                            p->lockMutex(p->dataMutex);
                            p->mCurrentdB = decay_dB;
                            p->unlockMutex(p->dataMutex);
                        }

                        p->lockMutex(p->dataMutex);

                        float volumeDiff = 0;

                        // Try to get the average volume

                        if(p->mAverageFactor > 0.01) p->mAverageFactor -= 0.005;
                        else p->mAverageFactor = 0.01;

                        if(currentVol > p->mCurrentVolume) p->mCurrentVolume += p->mAverageFactor;
                        else p->mCurrentVolume = currentVol;

                        p->mAverageVolume = (p->mCurrentVolume * p->mAverageFactor) + (p->mAverageVolume * (1-p->mAverageFactor));

                        if(p->mAverageFactor > 0.01) volumeDiff = p->mCurrentVolume - p->mPlayingVolume;
                        else volumeDiff = p->mAverageVolume - p->mPlayingVolume;

                        volumeDiff = floor(volumeDiff * 100) / 100.0;

                        // Adjust the playback volume accordingly
                        if(volumeDiff > 0.01 || volumeDiff < -0.01) {
                            // Take it easy on the volume increase while calibrating
                            if(p->mAverageVolume > p->mPlayingVolume)
                                p->mPlayingVolume += (0.5 - p->mAverageFactor) * volumeDiff;
                            else
                                p->mPlayingVolume = p->mAverageVolume;

                            g_object_set(G_OBJECT (p->pAmplify), "amplification", p->mPlayingVolume*p->mPlayingVolumeGain, NULL);

                            if(p->mAverageFactor <= 0.1 && !p->mGotFinalVolume) {
                                p->mGotFinalVolume = true;
                                LOG4CXX_WARN(playerImplLog, "setting amplify to " << p->mPlayingVolume);
                            }
                        }

                        p->unlockMutex(p->dataMutex);
                    }

                    break;
                }

            default:
                LOG4CXX_DEBUG(playerImplLog, "Got message from " << gst_element_get_name(GST_MESSAGE_SRC(message)) << " type: " << gst_message_type_get_name(GST_MESSAGE_TYPE(message)));

                break;
        }
    }
}

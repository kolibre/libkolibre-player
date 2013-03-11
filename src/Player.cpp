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

/**
 * \class Player
 *
 * \brief This is main interface for the kolibre player. It can play all types of audio files supported by GStreamer 0.10. When using the Player, this class is the only one you should need to include.
 *
 * \note Remeber to register slots for the signals that the player sends.
 *
 * \author Kolibre (www.kolibre.org)
 *
 * Contact: info@kolibre.org
 *
 */

using namespace std;

#include "Player.h"
#include "PlayerImpl.h"

Player * Player::pinstance = 0;

/**
 * Get the player instance and create if it does not exist.
 *
 * @return pointer to player instance
 */
Player * Player::Instance()
{
    if(pinstance == 0) {
        pinstance = new Player;
    }

    return pinstance;
}

/**
 * Initialize the player
 *
 */
Player::Player():
    p_impl( new PlayerImpl )
{
}

Player::~Player()
{
    delete p_impl;
}

/**
 * Enable gstreamer and initialize the player control thread
 *
 * @param argc arguments passed on the command line
 * @param argv arguments passed on the command line
 *
 * @return false if OK true if ERROR
 */
bool Player::enable(int *argc, char **argv[])
{
    return p_impl->enable( argc, argv );
}

/**
 * Get the state of the player
 *
 * @return the state of the player
 */
playerState Player::getRealState()
{
    return p_impl->getRealState();
}

/**
 * Set the state of the player
 *
 * @param state state to set
 */
void Player::setState(playerState state)
{
    p_impl->setState( state );
}

/**
 * Get the state of the player
 *
 * @return the state of the player
 */
playerState Player::getState()
{
    return p_impl->getState();
}

/**
 * Get the current state of the player as a string
 *
 * @return string of player state
 */
string Player::getState_str()
{
    return p_impl->getState_str();
}

/**
 * Set the a signal slot for when current file has problem buffering
 *
 * @param slot function pointer to the slot
 *
 * @return connection object for the message reporting signal-slot connection
 */
boost::signals2::connection Player::doOnPlayerMessage(OnPlayerMessage::slot_type slot)
{
    return p_impl->doOnPlayerMessage(slot);
}

/**
 * Set the a signal slot for when player state changes
 *
 * @param slot function pointer to the slot
 *
 * @return connection object for the state reporting signal-slot connection
 */
boost::signals2::connection Player::doOnPlayerState(OnPlayerState::slot_type slot)
{
    return p_impl->doOnPlayerState(slot);
}

/**
 * Set the a signal slot for when player time changes
 *
 * @param slot function pointer to the slot
 *
 * @return connection object for the time reporting signal-slot connection
 */
boost::signals2::connection Player::doOnPlayerTime(OnPlayerTime::slot_type slot)
{
    return p_impl->doOnPlayerTime(slot);
}

/**
 * Open a file and go to paused state
 *
 * @param filename URL of file to open
 * @param startms startms
 * @param stopms stopms
 */
void Player::open(const char *filename, const char *startms, const char *stopms)
{
    p_impl->open(filename, startms, stopms);
}

/**
 * Open a file and go to paused state
 *
 * @param filename URL of file to open
 * @param startms startms
 * @param stopms stopms
 */
void Player::open(string filename, long long startms, long long stopms)
{
    p_impl->open(filename, startms, stopms);
}

/**
 * Open a file and go to paused state
 *
 * @param filename URL of file to open
 */
void Player::open(string filename)
{
    p_impl->open(filename);
}

/**
 * Open a file and go to paused state
 *
 * @param filename URL of file to open
 * @param startms startms
 */
void Player::open(string filename, long long startms)
{
    p_impl->open(filename, startms);
}

/**
 * Seek to a position in the stream
 *
 * @param seektime ms to seek to
 */
void Player::seekPos(long long int seektime)
{
    p_impl->seekPos(seektime);
}

/**
 * Stop playback
 *
 */
void Player::stop()
{
    p_impl->stop();
}

/**
 * Reopen (resume/open)
 *
 */
void Player::reopen()
{
    p_impl->reopen();
}

/**
 * Pause playback
 *
 */
void Player::pause()
{
    p_impl->pause();
}

/**
 * Resume playback
 *
 */
void Player::resume()
{
    p_impl->resume();
}

/**
 * Get the current state of the player
 *
 * @return TRUE if we're playing, FALSE if not
 */
bool Player::isPlaying()
{
    return p_impl->isPlaying();
}

/**
 * Get the current position in the playing file
 *
 * @return current file position (ms)
 */
long long int Player::getPos()
{
    return p_impl->getPos();
}

/**
 * Get the current playing file
 *
 * @return current file
 */
string Player::getFilename()
{
    return p_impl->getFilename();
}

/**
 * Get the current startms in the playing file
 *
 * @return current file startms (ms)
 */
long long int Player::getStartms()
{
    return p_impl->getStartms();
}

/**
 * Get the current stopms in the playing file
 *
 * @return current file stopms (ms)
 */
long long int Player::getStopms()
{
    return p_impl->getStopms();
}

/**
 * Get the currently opened track
 *
 * @return currently open track
 */
unsigned int Player::getTrack()
{
    return p_impl->getTrack();
}

/**
 * Get the number of tracks on this CD
 *
 * @return number of tracks
 */
unsigned int Player::getNumTracks()
{
    return p_impl->getNumTracks();
}

/**
 * Return a vector containing the track lengths
 *
 * @return the tracks vector
 */
vector<long long int> Player::getTracks()
{
    return p_impl->getTracks();
}

/**
 * Get the musicbrainz discid
 *
 * @return discid string
 */
string Player::getCDADiscid()
{
    return p_impl->getCDADiscid();
}

/**
 * Loads track data for an AudioCD
 *
 *
 * @return true if all went ok
 */
bool Player::loadTrackData()
{
    return p_impl->loadTrackData();
}

/**
 * Adjust the playback tempo
 *
 * @param adjustment +/- tempo
 */
void Player::adjustTempo(double adjustment)
{
    p_impl->adjustTempo(adjustment);
}

/**
 * Get the current playback tempo
 *
 * @return tempo
 */
double Player::getTempo()
{
    return p_impl->getTempo();
}

/**
 * Set the playback tempo
 *
 * @param value tempo (between PLAYER_MAX_TEMPO and PLAYER_MIN_TEMPO)
 */
void Player::setTempo(double value)
{
    p_impl->setTempo(value);
}

/**
 * Adjust the playback pitch
 *
 * @param adjustment +/- pitch
 */
void Player::adjustPitch(double adjustment)
{
    p_impl->adjustPitch(adjustment);
}

/**
 * Get the current playback pitch
 *
 * @return pitch
 */
double Player::getPitch()
{
    return p_impl->getPitch();
}

/**
 * Set the playback pitch
 *
 * @param value pitch (between PLAYER_MAX_PITCH and PLAYER_MIN_PITCH)
 */
void Player::setPitch(double value)
{
    p_impl->setPitch(value);
}

/**
 * Adjust the playback volumegain
 *
 * @param adjustment +/- volumegain
 */
void Player::adjustVolumeGain(double adjustment)
{
    p_impl->adjustVolumeGain(adjustment);
}

/**
 * Get the current playback volumegain
 *
 * @return volumegain
 */
double Player::getVolumeGain()
{
    return p_impl->getVolumeGain();
}

/**
 * Set the playback volumegain
 *
 * @param value volumegain (between PLAYER_MAX_VOLUMEGAIN and PLAYER_MIN_VOLUMEGAIN)
 */
void Player::setVolumeGain(double value)
{
    p_impl->setVolumeGain(value);
}

/**
 * Adjust the playback treble
 *
 * @param adjustment +/- treble
 */
void Player::adjustTreble(double adjustment)
{
    p_impl->adjustTreble(adjustment);
}

/**
 * Get the current playback treble
 *
 * @return treble
 */
double Player::getTreble()
{
    return p_impl->getTreble();
}

/**
 * Set the playback treble
 *
 * @param value treble (between PLAYER_MAX_TREBLE and PLAYER_MIN_TREBLE)
 */
void Player::setTreble(double value)
{
    p_impl->setTreble(value);
}

/**
 * Adjust the playback bass
 *
 * @param adjustment +/- bass
 */
void Player::adjustBass(double adjustment)
{
    p_impl->adjustBass(adjustment);
}

/**
 * Get the current playback bass
 *
 * @return bass
 */
double Player::getBass()
{
    return p_impl->getBass();
}

/**
 * Set the playback bass
 *
 * @param value bass (between PLAYER_MAX_BASS and PLAYER_MIN_BASS)
 */
void Player::setBass(double value)
{
    p_impl->setBass(value);
}

/**
 * Toggle debugmode on or off
 *
 * @param setting true for on, false for off
 */
void Player::setDebugmode(bool setting)
{
    p_impl->setDebugmode(setting);
}

#ifdef ENABLE_EQUALIZER
/**
 * Sets up the 10 band equalizer based on the bass and treble gain values
 *
 * @param element gstreamer element
 * @param bass bass value
 * @param treble treble value
 *
 * @return null
 */
void Player::setEqualizer(GstElement *element, double bass, double treble)
{
    p_impl->setEqualizer(element, bass, treble);
}
#endif

/**
 * Set user agent string that is used when fetching online resources
 *
 * @param useragent the user agent string to be set
 */
void Player::setUseragent(string useragent)
{
    p_impl->setUseragent(useragent);
}

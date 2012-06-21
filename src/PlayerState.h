/*
Copyright (C) 2012  The Kolibre Foundation

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


#ifndef PLAYERSTATE_H
#define PLAYERSTATE_H

enum playerState {
    INACTIVE,   // Player has not yet been activated
    BUFFERING,  // Player is buffering file
    PLAYING,    // Player is playing the file
    PAUSING,    // Player is pausing
    STOPPED,    // Player has stopped
    EXITING,    // Playerthread should exit
};
#endif

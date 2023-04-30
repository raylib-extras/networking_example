/**********************************************************************************************
*
*   raylib_networking_smaple * a sample network game using raylib and enet
*
*   LICENSE: ZLIB
*
*   Copyright (c) 2023 Jeffery Myers
*
*   Permission is hereby granted, free of charge, to any person obtaining a copy
*   of this software and associated documentation files (the "Software"), to deal
*   in the Software without restriction, including without limitation the rights
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*   copies of the Software, and to permit persons to whom the Software is
*   furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included in all
*   copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*
**********************************************************************************************/

// client networking
// The network game play interface header.
// This includes the functions and common data used by both the graphical client and the network/game systems
// This header is what insulates raylib from windows.h calls in enet (https://github.com/zpl-c/enet)
#pragma once

#include <stdint.h>
#include <stdbool.h>

// It is ok to include raymath, since raymath doesn't have any conflict with windows.h
#include "raymath.h"

// Connect to the server (localhost by default)
void Connect(const char* serverAddress);

// Process one frame of updates
void Update(double now, float deltaT);

// Disconnect from the server
void Disconnect();

// True if we are connected to the server and have a valid player id.
bool Connected();

// Tell the network game play how far we wanted to move this frame
void UpdateLocalPlayer(Vector2* movementDelta, float deltaT);

// get the id that the server assigned to us
int GetLocalPlayerId();

// get the position info for a player from the local simulation that has the latest network data in it
// returns false if the player id is not valid
bool GetPlayerPos(int id, Vector2* pos);

/**********************************************************************************************
*
*   raylib_networking_smaple * a sample network game using raylib and enet
*
*   LICENSE: ZLIB
*
*   Copyright (c) 2021 Jeffery Myers
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

// constants for networking, does not include networking
#pragma once

#define MAX_PLAYERS 8

#define MAX_PLAYERS 8
// how big the screen is for all players
#define FieldSizeWidth 1280
#define FieldSizeHeight  800

// how big a player is
#define PlayerSize 10

// All the different commands that can be sent over the network
typedef enum
{
	// Server -> Client, You have been accepted. Contains the id for the client player to use
	AcceptPlayer = 1,

	// Server -> Client, Add a new player to your simulation, contains the ID of the player and a position
	AddPlayer = 2,

	// Server -> Client, Remove a player from your simulation, contains the ID of the player to remove
	RemovePlayer = 3,

	// Server -> Client, Update a player's position in the simulation, contains the ID of the player and a position
	UpdatePlayer = 4,

	// Client -> Server, Provide an updated location for the client's player, contains the postion to update
	UpdateInput = 5,
}NetworkCommands;

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

// implementation code for network game play interface

#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <windows.h>

#include "PCG_TYPES.h"

#include "net_common.h"
#include "listenserver.h"
#include "net_listenclient.h"

// the server thread
static HANDLE serverThread = NULL;

// server thread stop flag
static volatile bool serverRunning = false;

// the player id of this client
int LocalPlayerId = -1;

// the enet address we are connected to
ENetAddress address = { 0 };

// the server object we are connecting to
ENetPeer* server = { 0 };

// the client peer we are using
ENetHost* client = { 0 };

// time data for the network tick so that we don't spam the server with one update every drawing frame

// how long in seconds since the last time we sent an update
double LastInputSend = -100;

// how long to wait between updates (20 update ticks a second)
double InputUpdateInterval = 1.0f / 20.0f;

double LastNow = 0;

bool WantDisconnect = false;

// Data about players
typedef struct
{
	// true if the player is active and valid
	bool Active;

	// the last known location of the player on the field
	Vector2 Position;

	// the direction they were going
	Vector2 Direction;

	// the time we got the last update
	double UpdateTime;

	//where we think this item is right now based on the movement vector
	Vector2 ExtrapolatedPosition;

	int Health;
}RemotePlayer;

typedef struct {
	bool Active;

	Vector2 Position;

	Vector2 Direction;

	double UpdateTime;

	Vector2 ExtrapolatedPosition;

	int ownerId;
} RemoteBullet;

// The list of all possible players
// this is the local simulation that represents the current game state
// it includes the current local player and the last known data from all remote players
// the client checks this every frame to see where everyone is on the field
RemotePlayer Players[MAX_PLAYERS] = { 0 };
RemoteBullet Bullets[MAX_BULLETS] = { 0 };

// Start server when serverThread is created
DWORD WINAPI ServerThreadFunc(LPVOID arg) {
	RunServer();
	return 0;
}

// Start server thread
void StartListenServer() {
	serverThread = CreateThread(
		NULL,
		0,
		ServerThreadFunc,
		NULL,
		0,
		NULL
	);
}

// Stop server thread
void StopListenServer() {
	serverRunning = false;

	// wait for thread to finish
	WaitForSingleObject(serverThread, 5000);
	CloseHandle(serverThread);
	serverThread = NULL;
}

// Connect to a server
void Connect(const char* serverAddress)
{
	if (WantDisconnect)
		return;

	// startup the network library
	enet_initialize();

	// create a client that we will use to connect to the server
	client = enet_host_create(NULL, 1, 1, 0, 0);

	// set the address and port we will connect to
	enet_address_set_host(&address, serverAddress);
	address.port = 4545;

	// start the connection process. Will be finished as part of our update
	server = enet_host_connect(client, &address, 1, 0);
}

// Utility functions to read data out of a packet

/// <summary>
/// Read a player position from the network packet
/// player positions are sent as two signed shorts and converted into floats for display
/// since this sample does everything in pixels, this is fine, but a more robust game would want to send floats
/// </summary>
/// <param name="packet"></param>
/// <param name="offset"></param>
/// <returns>A raylib Vector with the position in the data</returns>
Vector2 ReadPosition(ENetPacket* packet, size_t* offset)
{
	Vector2 pos = { 0 };
	pos.x = ReadShort(packet, offset);
	pos.y = ReadShort(packet, offset);

	return pos;
}

// functions to handle the commands that the server will send to the client
// these take the data from enet and read out various bits of data from it to do actions based on the command that was sent

// A new remote player was added to our local simulation
void HandleAddPlayer(ENetPacket* packet, size_t* offset)
{
	// find out who the server is talking about
	int remotePlayer = ReadByte(packet, offset);
	if (remotePlayer >= MAX_PLAYERS || remotePlayer == LocalPlayerId)
		return;

	// set them as active and update the location
	Players[remotePlayer].Active = true;
	Players[remotePlayer].Position = ReadPosition(packet, offset);
	Players[remotePlayer].Direction = ReadPosition(packet, offset);
	Players[remotePlayer].UpdateTime = LastNow;
	Players[remotePlayer].Health = 3;

	// In a more robust game, this message would have more info about the new player, such as what sprite or model to use, player name, or other data a client would need
	// this is where static data about the player would be sent, and any initial state needed to setup the local simulation
}

// A remote player has left the game and needs to be removed from the local simulation
void HandleRemovePlayer(ENetPacket* packet, size_t* offset)
{
	// find out who the server is talking about
	int remotePlayer = ReadByte(packet, offset);
	if (remotePlayer >= MAX_PLAYERS || remotePlayer == LocalPlayerId)
		return;

	// remove the player from the simulation. No other data is needed except the player id
	Players[remotePlayer].Active = false;
}

// The server has a new position for a player in our local simulation
void HandleUpdatePlayer(ENetPacket* packet, size_t* offset)
{
	// find out who the server is talking about
	int remotePlayer = ReadByte(packet, offset);
	if (remotePlayer >= MAX_PLAYERS || remotePlayer == LocalPlayerId || !Players[remotePlayer].Active)
		return;

	// update the last known position and movement
	Players[remotePlayer].Position = ReadPosition(packet, offset);
	Players[remotePlayer].Direction = ReadPosition(packet, offset);
	Players[remotePlayer].UpdateTime = LastNow;

	// in a more robust game this message would have a tick ID for what time this information was valid, and extra info about
	// what the input state was so the local simulation could do prediction and smooth out the motion
}

// Read bullets from server into local sim
void HandleSpawnBullet(ENetPacket* packet, size_t* offset)
{
	int bulletId = ReadByte(packet, offset);
	if (bulletId < 0 || bulletId >= MAX_BULLETS) return;

	int ownerId = ReadByte(packet, offset);

	Bullets[bulletId].Active = true;
	Bullets[bulletId].ownerId = ownerId;
	Bullets[bulletId].Position = ReadPosition(packet, offset);
	Bullets[bulletId].Direction = ReadPosition(packet, offset);
	Bullets[bulletId].UpdateTime = LastNow;
}

// Remove bullet from local sim if server says so
void HandleDestroyBullet(ENetPacket* packet, size_t* offset)
{
	int bulletId = ReadByte(packet, offset);
	if (bulletId < 0 || bulletId >= MAX_BULLETS) return;

	Bullets[bulletId].Active = false;
}

// Read map from server into local tileArray
void HandleSyncMap(ENetPacket* packet, size_t* offset, TileType _tileArray[MAP_ROWS][MAP_COLUMNS]) {
	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLUMNS; x++) {
			_tileArray[y][x] = (TileType)ReadByte(packet, offset);
		}
	}
}

void HandleDamagePlayer(ENetPacket* packet, size_t* offset) {
	int targetID = ReadByte(packet, offset);
	if (targetID < 0 || targetID >= MAX_PLAYERS) return;

	Players[targetID].Health = ReadShort(packet, offset);
}

// process one frame of updates
void Update(double now, float deltaT, TileType _tileArray[MAP_ROWS][MAP_COLUMNS])
{
	LastNow = now;
	// if we are not connected to anything yet, we can't do anything, so bail out early
	if (server == NULL)
		return;

	// Check if we have been accepted, and if so, check the clock to see if it is time for us to send the updated position for the local player
	// we do this so that we don't spam the server with updates 60 times a second and waste bandwidth
	// in a real game we'd send our normalized movement vector or input keys along with what the current tick index was
	// this way the server can know how long it's been since the last update and can do interpolation to know were we are between updates.
	if (!WantDisconnect && LocalPlayerId >= 0 && now - LastInputSend > InputUpdateInterval)
	{
		// Pack up a buffer with the data we want to send
		uint8_t buffer[9] = { 0 }; // 9 bytes for a 1 byte command number and two bytes for each X and Y value
		buffer[0] = (uint8_t)UpdateInput;   // this tells the server what kind of data to expect in this packet
		*(int16_t*)(buffer + 1) = (int16_t)Players[LocalPlayerId].Position.x;
		*(int16_t*)(buffer + 3) = (int16_t)Players[LocalPlayerId].Position.y;
		*(int16_t*)(buffer + 5) = (int16_t)Players[LocalPlayerId].Direction.x;
		*(int16_t*)(buffer + 7) = (int16_t)Players[LocalPlayerId].Direction.y;

		// copy this data into a packet provided by enet (TODO : add pack functions that write directly to the packet to avoid the copy)
		ENetPacket* packet = enet_packet_create(buffer, 9, ENET_PACKET_FLAG_RELIABLE);

		// send the packet to the server
		enet_peer_send(server, 0, packet);

		// NOTE enet_host_service will handle releasing send packets when the network system has finally sent them,
		// you don't have to destroy them

		// mark that now was the last time we sent an update
		LastInputSend = now;
	}

	// read one event from enet and process it
	ENetEvent Event = { 0 };

	// Check to see if we even have any events to do. Since this is a a client, we don't set a timeout so that the client can keep going if there are no events
	if (enet_host_service(client, &Event, 0) > 0)
	{
		// see what kind of event it is
		switch (Event.type)
		{
			// the server sent us some data, we should process it
		case ENET_EVENT_TYPE_RECEIVE:
		{
			// we know that all valid packets have a size >= 1, so if we get this, something is bad and we ignore it.
			if (Event.packet->dataLength < 1 || WantDisconnect)
			{
				enet_packet_destroy(Event.packet);
				break;
			}

			// keep an offset of what data we have read so far
			size_t offset = 0;

			// read off the command that the server wants us to do
			NetworkCommands command = (NetworkCommands)ReadByte(Event.packet, &offset);

			// if the server has not accepted us yet, we are limited in what packets we can receive
			if (LocalPlayerId == -1)
			{
				if (command == AcceptPlayer)    // this is the only thing we can do in this state, so ignore anything else
				{
					// See who the server says we are
					LocalPlayerId = ReadByte(Event.packet, &offset);

					// Make sure that it makes sense
					if (LocalPlayerId < 0 || LocalPlayerId > MAX_PLAYERS)
					{
						LocalPlayerId = -1;
						break;
					}

					// Force the next frame to do an update by pretending it's been a very long time since our last update
					LastInputSend = -InputUpdateInterval;

					// We are active
					Players[LocalPlayerId].Active = true;

					// Set our player at some location on the field.
					// optimally we would do a much more robust connection negotiation where we tell the server what our name is, what we look like
					// and then the server tells us where we are
					// But for this simple test, everyone starts at the same place on the field
					Players[LocalPlayerId].Position = (Vector2){ 100, 100 };
					Players[LocalPlayerId].Health = 3;
				}
			}
			else // we have been accepted, so process play messages from the server
			{
				// see what the server wants us to do
				switch (command)
				{
				case AddPlayer:
					HandleAddPlayer(Event.packet, &offset);
					break;

				case RemovePlayer:
					HandleRemovePlayer(Event.packet, &offset);
					break;

				case UpdatePlayer:
					HandleUpdatePlayer(Event.packet, &offset);
					break;
				
				case SpawnBullet:
					HandleSpawnBullet(Event.packet, &offset);
					break;

				case DestroyBullet:
					HandleDestroyBullet(Event.packet, &offset);
					break;

				case SyncMap:
					HandleSyncMap(Event.packet, &offset, _tileArray);
					break;
				case DamagePlayer:
					HandleDamagePlayer(Event.packet, &offset);
					break;
				}

			}
			// tell enet that it can recycle the packet data
			enet_packet_destroy(Event.packet);
			break;
		}

		// we were disconnected, we have a sad
		case ENET_EVENT_TYPE_DISCONNECT:
		{
			// close our client
			if (client != NULL)
				enet_host_destroy(client);

			client = NULL;
			server = NULL;

			// clean up enet
			enet_deinitialize();

			server = NULL;
			LocalPlayerId = -1;

			WantDisconnect = false;
		}
		break;
		}
	}

	// update all the remote players with an interpolated position based on the last known good pos and how long it has been since an update
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (i == LocalPlayerId || !Players[i].Active)
			continue;
		double delta = LastNow - Players[i].UpdateTime;
		Players[i].ExtrapolatedPosition = Vector2Add(Players[i].Position, Vector2Scale(Players[i].Direction, (float)delta));
	}

	for (int i = 0; i < MAX_BULLETS; i++)
	{
		if (!Bullets[i].Active)
			continue;
		double delta = LastNow - Bullets[i].UpdateTime;
		Bullets[i].ExtrapolatedPosition = Vector2Add(Bullets[i].Position, Vector2Scale(Bullets[i].Direction, (float)delta));
	}
}

// force a disconnect by shutting down enet
void Disconnect()
{
	// start to close our connection to the server
	if (server != NULL)
	{
		WantDisconnect = true;
		enet_peer_disconnect(server, 0);
	}
}

// true if we are connected and have been accepted
bool Connected()
{
	return server != NULL;
}

int GetLocalPlayerId()
{
	return LocalPlayerId;
}

// add the input to our local position and make sure we are still inside the field
void UpdateLocalPlayer(Vector2* movementDelta, float deltaT, TileType tileArray[MAP_ROWS][MAP_COLUMNS])
{
	// if we are not accepted, we can't update
	if (LocalPlayerId < 0)
		return;

	Vector2 pos = Players[LocalPlayerId].Position;

	// try X axis independently
	Vector2 newPosX = { pos.x + movementDelta->x * deltaT, pos.y };
	if (newPosX.x < 0) newPosX.x = 0;
	if (newPosX.x > FieldSizeWidth - PlayerSize) newPosX.x = FieldSizeWidth - PlayerSize;

	Vector2 cornersX[4] = {
		{ newPosX.x,              newPosX.y },
		{ newPosX.x + PlayerSize, newPosX.y },
		{ newPosX.x,              newPosX.y + PlayerSize },
		{ newPosX.x + PlayerSize, newPosX.y + PlayerSize },
	};
	bool blockedX = false;
	for (int i = 0; i < 4; i++) {
		int col = (int)(cornersX[i].x / TILE_SIZE);
		int row = (int)(cornersX[i].y / TILE_SIZE);
		if (col < 0 || col >= MAP_COLUMNS || row < 0 || row >= MAP_ROWS) continue;
		if (tileArray[row][col] == TILE_TYPE_ROCK) { blockedX = true; break; }
	}
	if (!blockedX) pos.x = newPosX.x;

	// try Y axis independently
	Vector2 newPosY = { pos.x, pos.y + movementDelta->y * deltaT };
	if (newPosY.y < 0) newPosY.y = 0;
	if (newPosY.y > FieldSizeHeight - PlayerSize) newPosY.y = FieldSizeHeight - PlayerSize;

	Vector2 cornersY[4] = {
		{ newPosY.x,              newPosY.y },
		{ newPosY.x + PlayerSize, newPosY.y },
		{ newPosY.x,              newPosY.y + PlayerSize },
		{ newPosY.x + PlayerSize, newPosY.y + PlayerSize },
	};
	bool blockedY = false;
	for (int i = 0; i < 4; i++) {
		int col = (int)(cornersY[i].x / TILE_SIZE);
		int row = (int)(cornersY[i].y / TILE_SIZE);
		if (col < 0 || col >= MAP_COLUMNS || row < 0 || row >= MAP_ROWS) continue;
		if (tileArray[row][col] == TILE_TYPE_ROCK) { blockedY = true; break; }
	}
	if (!blockedY) pos.y = newPosY.y;

	Players[LocalPlayerId].Position = pos;
	Players[LocalPlayerId].Direction = *movementDelta;
}

void SpawnLocalBullet(Vector2 cursorPos) {
	if (LocalPlayerId < 0)
		return;

	// get player aiming direction
	Vector2 dir = Vector2Subtract(cursorPos, Players[LocalPlayerId].Position);
	dir = Vector2Normalize(dir);

	// pack the spawn request — server will assign the bullet ID
	uint8_t buffer[9] = { 0 };
	buffer[0] = (uint8_t)SpawnBullet;
	*(int16_t*)(buffer + 1) = (int16_t)Players[LocalPlayerId].Position.x;
	*(int16_t*)(buffer + 3) = (int16_t)Players[LocalPlayerId].Position.y;
	*(int16_t*)(buffer + 5) = (int16_t)(dir.x * 200);
	*(int16_t*)(buffer + 7) = (int16_t)(dir.y * 200);

	ENetPacket* packet = enet_packet_create(buffer, 9, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(server, 0, packet);
}

void SendMapSync(TileType _tileArray[MAP_ROWS][MAP_COLUMNS]) {
	
	// setup buffer based on map size
	int tileCount = MAP_ROWS * MAP_COLUMNS;
	int bufferSize = 1 + tileCount;

	// no vectors in C but want to have resizable buffer so must use malloc()
	uint8_t* buffer = (uint8_t*)malloc(bufferSize);
	if (buffer == NULL) return;
	buffer[0] = (uint8_t)SyncMap;

	for (int y = 0; y < MAP_ROWS; y++)
		for (int x = 0; x < MAP_COLUMNS; x++)
			buffer[1 + y * MAP_COLUMNS + x] = (uint8_t)_tileArray[y][x];

	ENetPacket* packet = enet_packet_create(buffer, bufferSize, ENET_PACKET_FLAG_RELIABLE);
	enet_peer_send(server, 0, packet);
	// cleanup buffer
	free(buffer);
}

bool GetBulletPos(int id, Vector2* pos, int* ownerId)
{
	if (id < 0 || id >= MAX_BULLETS || !Bullets[id].Active)
		return false;

	*pos = Bullets[id].ExtrapolatedPosition;
	*ownerId = Bullets[id].ownerId;
	return true;
}

// get the info for a particular player
bool GetPlayerPos(int id, Vector2* pos)
{
	// make sure the player is valid and active
	if (id < 0 || id >= MAX_PLAYERS || !Players[id].Active)
		return false;

	// copy the location (real or extrapolated)
	if (id == LocalPlayerId)
		*pos = Players[id].Position;
	else
		*pos = Players[id].ExtrapolatedPosition;
	return true;
}

int GetPlayerHealth(int id) {
	if (id < 0 || id >= MAX_PLAYERS) return 0;
	return Players[id].Health;
}

bool IsHost() {
	return serverThread != NULL;
}

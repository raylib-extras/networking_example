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

// server code
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define ENET_IMPLEMENTATION
#include "net_common.h"
#include "PCG_TYPES.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// the info we are tracking about each player in the game
typedef struct
{
	// is this player slot active
	bool Active;

	// have they sent us a valid position yet?
	bool ValidPosition;

	// the network connection they use
	ENetPeer* Peer;

	// the last known location in X and Y
	int16_t X;
	int16_t Y;

	int16_t DX;
	int16_t DY;

	int16_t Health;
}PlayerInfo;

typedef struct {
	int id;
	int ownerID;
	float x, y;
	float velX, velY;
	bool active;
} BulletState;

static volatile bool serverRunning = false;

static uint8_t ServerMapData[1 + MAP_ROWS * MAP_COLUMNS] = { 0 };
static bool ServerMapValid = false;


// The list of all possible players
// this is the server state of the game that represents the current game state
// this is what server code would check to see where all the players are and what they are doing
PlayerInfo ServerPlayers[MAX_PLAYERS] = { 0 };

BulletState ServerBullets[MAX_BULLETS] = { 0 };

void StopServer() {
	serverRunning = false;
}

// finds the player slot that goes with the player connection
// the peer has the void* ENetPeer::data that can be used to store arbitary application data
// but that involves managing structure pointers so it is kept out of this example
int GetPlayerId(ENetPeer* peer)
{
	// find the slot that matches the pointer
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (ServerPlayers[i].Active && ServerPlayers[i].Peer == peer)
			return i;
	}
	return -1;
}

// sends a packet over the network to every active player, except the one specified (usually the sender)
// senders know what they sent so you can choose to not send them data they already know.
// in a truly authoritative server you'd send back an acceptance message to all client input so they know it wasn't rejected.
void SendToAllBut(ENetPacket* packet, int exceptPlayerId)
{
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (!ServerPlayers[i].Active || i == exceptPlayerId)
			continue;

		enet_peer_send(ServerPlayers[i].Peer, 0, packet);
	}
}

// the main server loop
void RunServer()
{
	printf("Startup\n");

	// set up networking
	if (enet_initialize() != 0)
		return;

	printf("Initialized\n");

	// network servers must 'listen' on an interface and a port
	// this code sets up enet to listen on any available interface and using our port
	// the client must use the same port as the server and know the address of the server
	ENetAddress address = { 0 };
	address.host = ENET_HOST_ANY;
	address.port = 4545;

	// create the server host
	ENetHost* server = enet_host_create(&address, MAX_PLAYERS, 1, 0, 0);

	if (server == NULL)
		return;

	printf("Created\n");

	// the server will run forever. If we wanted a way to stop it, we'd set run to false using some code
	serverRunning = true;

	enet_uint32 lastBulletUpdate = enet_time_get();

	while (serverRunning)
	{
		ENetEvent event = { 0 };

		// see if there are any inbound network events, wait up to 1000ms before returning.
		// if the server also did game logic, this timeout should be lowered
		if (enet_host_service(server, &event, 0) > 0)
		{
			// see what kind of event we have
			switch (event.type)
			{

				// a new client is trying to connect
			case ENET_EVENT_TYPE_CONNECT:
			{
				printf("Player Connected\n");

				// find an empty slot, or disconnect them if we are full
				int playerId = 0;
				for (; playerId < MAX_PLAYERS; playerId++)
				{
					if (!ServerPlayers[playerId].Active)
						break;
				}

				// we are full
				if (playerId == MAX_PLAYERS)
				{
					// I said good day SIR!
					enet_peer_disconnect(event.peer, 0);
					break;
				}

				// player is good, don't give away the slot
				ServerPlayers[playerId].Active = true;

				// but don't send out an update to everyone until they give us a good position
				ServerPlayers[playerId].ValidPosition = false;
				ServerPlayers[playerId].Peer = event.peer;

				ServerPlayers[playerId].Health = 3;

				// pack up a message to send back to the client to tell them they have been accepted as a player
				uint8_t buffer[2] = { 0 };
				buffer[0] = (uint8_t)AcceptPlayer;  // command for the client
				buffer[1] = (uint8_t)playerId;      // the player ID so they know who they are

				// copy the buffer into an enet packet (TODO : add write functions to go directly to a packet)
				ENetPacket* packet = enet_packet_create(buffer, 2, ENET_PACKET_FLAG_RELIABLE);
				// send the data to the user
				enet_peer_send(event.peer, 0, packet);

				// We have to tell the new client about all the other players that are already on the server
				// so send them an add message for all existing active players.
				for (int i = 0; i < MAX_PLAYERS; i++)
				{
					// only people who are valid and not the new player
					if (i == playerId || !ServerPlayers[i].ValidPosition)
						continue;

					// pack up an add player message with the ID and the last known position
					uint8_t addBuffer[10] = { 0 };
					addBuffer[0] = (uint8_t)AddPlayer;
					addBuffer[1] = (uint8_t)i;
					*(int16_t*)(addBuffer + 2) = (int16_t)ServerPlayers[i].X;
					*(int16_t*)(addBuffer + 4) = (int16_t)ServerPlayers[i].Y;
					*(int16_t*)(addBuffer + 6) = (int16_t)ServerPlayers[i].DX;
					*(int16_t*)(addBuffer + 8) = (int16_t)ServerPlayers[i].DY;

					// Optimally we'd also send other info like name, color, and other static player info.

					// copy and send the message
					packet = enet_packet_create(addBuffer, 10, ENET_PACKET_FLAG_RELIABLE);
					enet_peer_send(event.peer, 0, packet);

					// NOTE enet_host_service will handle releasing send packets when the network system has finally sent them,
					// you don't have to destroy them
				}

				if (ServerMapValid) {
					ENetPacket* packet = enet_packet_create(
						ServerMapData,
						1 + MAP_ROWS * MAP_COLUMNS,
						ENET_PACKET_FLAG_RELIABLE
					);
					enet_peer_send(event.peer, 0, packet);
				}

				break;
			}

			// someone sent us data
			case ENET_EVENT_TYPE_RECEIVE:
			{
				// find the player who sent the data
				// we don't need them to send us what ID they are, we know who they are by the peer
				// we want to trust the client as little as possible so that people can't cheat/hack
				// if we blindly accepted a player ID, a client could send you updates for someone else :(

				int playerId = GetPlayerId(event.peer);
				if (playerId == -1)
				{
					// they are not one of our peeple, boot them
					enet_peer_disconnect(event.peer, 0);
					break;
				}

				// keep track of how far into the message we are
				size_t offset = 0;

				// read off the command the client wants us to process
				NetworkCommands command = ReadByte(event.packet, &offset);

				// we only accept one message from clients for now, so make sure this is what it is
				if (command == UpdateInput)
				{
					// update the location data with the new info
					ServerPlayers[playerId].X = ReadShort(event.packet, &offset);
					ServerPlayers[playerId].Y = ReadShort(event.packet, &offset);
					ServerPlayers[playerId].DX = ReadShort(event.packet, &offset);
					ServerPlayers[playerId].DY = ReadShort(event.packet, &offset);

					// lets tell everyone about this new location
					NetworkCommands outboundCommand = UpdatePlayer;

					// if they are new, send this update as an add player instead of an update
					if (!ServerPlayers[playerId].ValidPosition)
						outboundCommand = AddPlayer;

					// the player has sent us a position, they can be part of future regular updates
					ServerPlayers[playerId].ValidPosition = true;

					// pack up the update message with command, player and position
					uint8_t buffer[10] = { 0 };
					buffer[0] = (uint8_t)outboundCommand;
					buffer[1] = (uint8_t)playerId;
					*(int16_t*)(buffer + 2) = (int16_t)ServerPlayers[playerId].X;
					*(int16_t*)(buffer + 4) = (int16_t)ServerPlayers[playerId].Y;
					*(int16_t*)(buffer + 6) = (int16_t)ServerPlayers[playerId].DX;
					*(int16_t*)(buffer + 8) = (int16_t)ServerPlayers[playerId].DY;


					// Copy and send the data to everyone but the player who sent it  (TODO : add write functions to go directly to a packet)
					ENetPacket* packet = enet_packet_create(buffer, 10, ENET_PACKET_FLAG_RELIABLE);
					SendToAllBut(packet, playerId);

					// NOTE enet_host_service will handle releasing send packets when the network system has finally sent them,
					// you don't have to destroy them
				}
				else if (command == SpawnBullet) {
					// server assigns the ID, don't trust the client to pick one
					int bulletId = -1;
					for (int i = 0; i < MAX_BULLETS; i++) {
						if (!ServerBullets[i].active) { bulletId = i; break; }
					}
					if (bulletId == -1) break;  // no free slots, ignore

					ServerBullets[bulletId].ownerID = playerId;
					ServerBullets[bulletId].x = ReadShort(event.packet, &offset);
					ServerBullets[bulletId].y = ReadShort(event.packet, &offset);
					ServerBullets[bulletId].velX = ReadShort(event.packet, &offset);
					ServerBullets[bulletId].velY = ReadShort(event.packet, &offset);
					ServerBullets[bulletId].active = true;

					uint8_t buffer[11] = { 0 };
					buffer[0] = (uint8_t)SpawnBullet;
					buffer[1] = (uint8_t)bulletId;
					buffer[2] = (uint8_t)ServerBullets[bulletId].ownerID;
					*(int16_t*)(buffer + 3) = (int16_t)ServerBullets[bulletId].x;
					*(int16_t*)(buffer + 5) = (int16_t)ServerBullets[bulletId].y;
					*(int16_t*)(buffer + 7) = (int16_t)ServerBullets[bulletId].velX;
					*(int16_t*)(buffer + 9) = (int16_t)ServerBullets[bulletId].velY;

					ENetPacket* packet = enet_packet_create(buffer, 11, ENET_PACKET_FLAG_RELIABLE);
					enet_host_broadcast(server, 0, packet);
					
				}
				else if (command == SyncMap) {
					memcpy(ServerMapData, event.packet->data, event.packet->dataLength);
					ServerMapValid = true;

					ENetPacket* packet = enet_packet_create(
						event.packet->data,
						event.packet->dataLength,
						ENET_PACKET_FLAG_RELIABLE
					);
					SendToAllBut(packet, playerId);

				}

				// tell enet that it can recycle the inbound packet
				enet_packet_destroy(event.packet);
				break;
			}
			case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
			case ENET_EVENT_TYPE_DISCONNECT:
			{
				// a player was disconnected
				printf("Player Disconnected\n");

				// find them if they are a real player
				int playerId = GetPlayerId(event.peer);
				if (playerId == -1)
					break;

				// mark them as inactive and clear the peer pointer
				ServerPlayers[playerId].Active = false;
				ServerPlayers[playerId].Peer = NULL;

				// Tell everyone that someone left
				uint8_t buffer[2] = { 0 };
				buffer[0] = (uint8_t)RemovePlayer;
				buffer[1] = (uint8_t)playerId;

				// Copy and send the data to everyone but the player who sent it  (TODO : add write functions to go directly to a packet)
				ENetPacket* packet = enet_packet_create(buffer, 2, ENET_PACKET_FLAG_RELIABLE);
				SendToAllBut(packet, -1);

				// NOTE enet_host_service will handle releasing send packets when the network system has finally sent them,
				// you don't have to destroy them

				break;
			}

			case ENET_EVENT_TYPE_NONE:
				break;
			}
		}

		enet_uint32 now = enet_time_get();
		float deltaTime = (now - lastBulletUpdate) / 1000.0f;
		lastBulletUpdate = now;

		for (int i = 0; i < MAX_BULLETS; i++) {
			if (!ServerBullets[i].active) continue;

			ServerBullets[i].x += ServerBullets[i].velX * deltaTime;
			ServerBullets[i].y += ServerBullets[i].velY * deltaTime;

			bool destroy = false;

			// check for oob
			if (ServerBullets[i].x < 0 || ServerBullets[i].x > FieldSizeWidth ||
				ServerBullets[i].y < 0 || ServerBullets[i].y > FieldSizeHeight)
			{
				destroy = true;
			}

			if (!destroy && ServerMapValid) {
				int col = (int)(ServerBullets[i].x / TILE_SIZE);
				int row = (int)(ServerBullets[i].y / TILE_SIZE);

				if (col >= 0 && col < MAP_COLUMNS && row >= 0 && row < MAP_ROWS) {
					int tileIndex = 1 + row * MAP_COLUMNS + col;
					if (ServerMapData[tileIndex] == TILE_TYPE_ROCK) {
						destroy = true;
					}
				}
			}

			if (!destroy) {
				for (int p = 0; p < MAX_PLAYERS; p++) {
					if (!ServerPlayers[p].Active || p == ServerBullets[i].ownerID) continue;

					float bx = ServerBullets[i].x;
					float by = ServerBullets[i].y;
					float px = (float)ServerPlayers[p].X;
					float py = (float)ServerPlayers[p].Y;

					if (bx >= px && bx <= px + PlayerSize &&
						by >= py && by <= py + PlayerSize)
					{
						destroy = true;

						ServerPlayers[p].Health -= 1;

						printf("[HIT] Bullet %d hit player %d, health now %d\n",
							i, p, ServerPlayers[p].Health);

						uint8_t buffer[4] = { 0 };
						buffer[0] = (uint8_t)DamagePlayer;
						buffer[1] = (uint8_t)p;
						*(uint16_t*)(buffer + 2) = ServerPlayers[p].Health;

						ENetPacket* packet = enet_packet_create(buffer, 4, ENET_PACKET_FLAG_RELIABLE);
						enet_host_broadcast(server, 0, packet);

						if (ServerPlayers[p].Health <= 0) {
							ServerPlayers[p].Active = false;
							enet_peer_disconnect(ServerPlayers[p].Peer, 0);
						}
						break;
					}
				}
			}

			if (destroy) {
				ServerBullets[i].active = false;

				uint8_t buffer[2] = { 0 };
				buffer[0] = (uint8_t)DestroyBullet;
				buffer[1] = (uint8_t)i;

				ENetPacket* packet = enet_packet_create(buffer, 2, ENET_PACKET_FLAG_RELIABLE);
				enet_host_broadcast(server, 0, packet);
			}
		}
	}

	// cleanup
	enet_host_destroy(server);
	enet_deinitialize();

	return;
}
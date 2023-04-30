# Raylib Networking Example
A simple example of how to do networking using raylib and enet.

* Raylib can be found at https://github.com/raysan5/raylib, but is a subnet module for this repository
* Enet can be found at https://github.com/zpl-c/enet but is also included in this repository

## About
This is a simple client/server networking demo that allows up to 8 players to connect to a server and move boxes around a fixed size area. It is written in Pure C using Raylib for graphics and window setup and the ZPL-C version of enet for networking.

When a client is started it will attempt to connect to the server (on localhost by default). Once conncected it will spawn a player with a peset color that the client can move around with the arrow keys. Different colored player objects for other clients will be shown in the window, updating with the respective client. Each client maintains a local simulation state that represents the gameplay state that it is aware of. The server also maintains a state of the last known positon of each connected player.

### Notes
This example is not a robust networking system and is only intended as a simple example of how to setup a basic client/server system. Game networking is a very complex subject with many subtle nuances. Networking is not something that can just be attached to a single player game. A game needs to be built with networking in mind from the ground up. Even in this simple example, the core gameplay system is built into the networking layer, not the main application. Network games must be designed to take latency and data loss into account. If your game relies on every player having the exact same gamestate at the same time, then it will not work well in a network environment.

Different network setups are required for different kinds of games. For good networking you will need to research the best solution for game type. For action games a good place to start the articiles below.
* https://gafferongames.com/categories/game-networking/
* https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking
* https://developer.valvesoftware.com/wiki/Lag_compensation
* https://developer.valvesoftware.com/wiki/Latency_Compensating_Methods_in_Client/Server_In-game_Protocol_Design_and_Optimization


## Building
Uses game-premake https://github.com/raylib-extras/game-premake

### Windows MinGW
Run the premake-mingw.bat and then run make in the folder

### Windows Visual Studio (not VSC)
Run premake-VisualStudio.bat and then open the fasteroids.sln that is generated

### Linux
CD into the directory, run ./premake5 gmake2 and then run make

#### MacOS
CD into the directory, run ./premake5.osx gmake2 and then run make

## Code Overview

### NetCommon
A libary containing common networking functions and constants used by both client and server

### Server
The server is entirely contained within the server.c file. It is very simple and just runs a loop looking for network events. When a player connects, disconnects or sends data, the server responds to the event, updates an internal player list, and sends out required updates to other players.

### Client
The client is broken up into 3 files
* client.c
* net_client.h
* net_client.c

#### client.c
The main file is where the normal raylib window is setup, input is checked and the game is drawn. Every frame the input is checked, the player is updated and the field is drawn with all players on it.

Due to conflicts between raylib and windows.h, it is not possible to include networking in the same source files as raylib. For this reason the gameplay and networking systems are put into a seperate file and accessed via an interface header.

#### net_client.h
This is the interface between the network gameplay system and the main game application. It exists to keep raylib and windows files seperate. It contains defintions of all the functions and constants that are needed by the main game to run the game. Because raymath.h does not conflict with windows.h, the networking.h file includes raymath in order to use raylib structures, such as Vector2

#### net_client.c
This is the implementation file for the network gameplay system. It uses enet to create a client connection to the server and keep the local simulation up to date. It sends out the local player's position 20 times a second using a server tick clock. This prevents the network from being overloaded with updates with every drawn frame and different update rates for players with different frame rates.

## Network Commands
All network iformation is sent as commands. Commands are encoded into the network packet as a single byte, allowing up to 255 different commands. The command tells the receiving system what kind of data will be in the packet and what the requested action is.

## Packet Data
In this example network data is packaged up in the native format for the sending computer. This means that computers with different byte ordering (https://en.wikipedia.org/wiki/Endianness) can not communicate with each other. A real game would encode all data into Network Byte Order on send and decode on receive.

## Example Data Flow

Client -> Server
Client startup and connects to server.

Server receives connection request and allocates a player ID for the new user
	If the server is full the new player is rejected.
	
Server -> Client
Server sends Acccept messaage back to player with player ID
Server sends Add Player message for all existing players to new player

Client receives accept message
Client adds self to player list and marks connection as active
Client gameplay loop starts polling for local player input

Client receives Add Player messages and updates local simulation state

Every frame on the client, input is polled and a new local player position is updated in the local simulation.

Client -> Server
Every network tick (1/20th of a second), the local player's location is sent as an input update to the server.

Server -> Client
When the server receiives an input update, it updates the server game state with the new position and sends an Update Player message to all players.

As clients receive update messages they set the local simulation to match the last known location of each remote player.



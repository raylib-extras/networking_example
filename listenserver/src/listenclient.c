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

//This is the client main for a simple networking game (max 8 players)
// it starts up a graphical client, connects to a server and runs the game, showing all players

#define _CRT_SECURE_NO_WARNINGS

// include raylib
#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// include the networking interface
// we can't directly include networking in any file that uses raylib.h, so we abstract out the network gameplay to it's own file
#include "net_listenclient.h"
#include "net_constants.h"
#include "PCG.h"

char defaultIP[256] = "Enter IP Address";
char portStr[16] = "4545";
uint16_t defaultPort = 4545;
bool ipEditMode = false;
bool portEditMode = false;

// a list of predefined colors based on the player lost
static Color PlayerColors[MAX_PLAYERS] = { 0 };

TileType tileArray[MAP_ROWS][MAP_COLUMNS] = { 0 };

void SetColors()
{
	PlayerColors[0] = WHITE;
	PlayerColors[1] = RED;
	PlayerColors[2] = GREEN;
	PlayerColors[3] = BLUE;
	PlayerColors[4] = PURPLE;
	PlayerColors[5] = GRAY;
	PlayerColors[6] = YELLOW;
	PlayerColors[7] = ORANGE;
}

typedef enum GameState
{
	Connecting,
	Playing,
	Disconnecting,
	Disconnected,
}GameState;

static GameState State = Disconnected;

static bool RunGame = true;

static RenderTexture2D screenTarget = { 0 };
static Shader           shakeShader = { 0 };
static int              shakeLoc = 0;   // uniform: shakeStrength
static int              timeLoc = 0;   // uniform: time
static float            shakeTimer = 0.0f;
static float            shakeDuration = 0.3f;   // seconds the shake lasts
static float            shakePeak = 0.012f;  // max UV offset strength
static int              lastLocalHP = 3;

static Sound bulletSFX = { 0 };
static Sound hitSFX = { 0 };
static Sound deathSFX = { 0 };

static void Quit()
{
	RunGame = false;
}

// how fast in pixels per second we can move
// NOTE : the server should send us all this data in a real game
static float MoveSpeed = 100;

void UpdateGame()
{
	// let the network game system update
	// this will process any inbound events and update the local simulation
	Update(GetTime(), GetFrameTime(), tileArray);

	switch (State)
	{
	case Disconnected:
		Quit();
	default:
		break;

	case Connecting:
		if (Connected() && GetLocalPlayerId() >= 0) {
			State = Playing;
			if (IsHost())
				SendMapSync(tileArray);
		}
		break;

	case Disconnecting:
		if (!Connected())
		{
			State = Disconnected;
		}
		break;

	case Playing:
		if (WindowShouldClose())
		{
			Disconnect();
			State = Disconnecting;
		}
		else if (!Connected())
		{
			// we got booted, reconnect
			Connect(defaultIP, defaultPort);
			State = Connecting;
		}
		else
		{
			// cache of the incremental amount we are going to move this frame
			Vector2 movement = { 0 };
			float speed = MoveSpeed;

			// see what axes we move in
			if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
				movement.y -= speed;
			if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))
				movement.y += speed;

			if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
				movement.x -= speed;
			if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
				movement.x += speed;

			// Detect damage taken this frame
			int currentHP = GetPlayerHealth(GetLocalPlayerId());
			if (currentHP < lastLocalHP) {
				shakeTimer = shakeDuration;
				PlaySound(hitSFX);
			}
			else if (currentHP <= 0) {
				PlaySound(deathSFX);
			}
			lastLocalHP = currentHP;

			// Decay the shake timer
			shakeTimer -= GetFrameTime();
			if (shakeTimer < 0.0f) shakeTimer = 0.0f;

			// tell the network game play client that we moved
			// it will update the local simulation and cache the data until the next network tick time
			UpdateLocalPlayer(&movement, GetFrameTime(), tileArray);

			if (IsMouseButtonPressed(0)) {
				SpawnLocalBullet(GetMousePosition());
				PlaySound(bulletSFX);
			}
		}
		break;
	}
}

void DrawGame()
{

	switch (State)
	{
	case Disconnected:
	default:
		DrawText("Disconnected", 0, 20, 20, RED);
		break;

	case Connecting:
		DrawText("Connecting...", 0, 20, 20, DARKGREEN);
		DrawText("TANK2600", ((FieldSizeWidth / 2) - MeasureText("TANK2600", 50) / 2), FieldSizeHeight / 2, 50, BLACK);
		DrawText("Map Select", ((FieldSizeWidth / 2) - MeasureText("Map Select", 20) / 2), (FieldSizeHeight / 2 + 50), 20, BLACK);
		if (GuiButton((Rectangle) { FieldSizeWidth / 2 - 40, FieldSizeHeight / 2 + 70, 80, 20 }, "Load Map")) {
			PCG_LoadMapData(tileArray, "pcg_map_data.fyl");
		}
		DrawText("Host Game", ((FieldSizeWidth / 2) - MeasureText("Host Game", 20) / 2), (FieldSizeHeight / 2 + 90), 20, BLACK);
		if (GuiButton((Rectangle) { FieldSizeWidth/2 - 40, FieldSizeHeight/2 + 110, 80, 20 }, "Host")) {
			StartListenServer();
			WaitTime(0.1);
			ConnectHost("127.0.0.1");
		}
		DrawText("Join Game", ((FieldSizeWidth / 2) - MeasureText("Join Game", 20) / 2), (FieldSizeHeight / 2 + 130), 20, BLACK);
		if (GuiTextBox((Rectangle) { FieldSizeWidth / 2 - 100, FieldSizeHeight / 2 + 150, 200, 20 }, defaultIP, 20, ipEditMode)) {
			ipEditMode = !ipEditMode;
		}
		if (GuiButton((Rectangle) { FieldSizeWidth / 2 - 40, FieldSizeHeight / 2 + 170, 80, 20 }, "Join") && strlen(defaultIP) > 0) {
				Connect(defaultIP);
		}
		break;

	case Disconnecting:
		DrawText("Disconnecting from server...", 0, 20, 20, MAROON);
		break;

	case Playing:
		PCG_DrawMap(tileArray, false);

		// we are connected, and know what our player ID is, so show that to the player in our color
		DrawText(TextFormat("Player %d", GetLocalPlayerId()), 0, 20, 20, PlayerColors[GetLocalPlayerId()]);

		// draw all active players, this includes our local player since the game system is maintaining the local simulation
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			Vector2 pos = { 0 };
			if (GetPlayerPos(i, &pos))
			{
				DrawRectangle((int)pos.x, (int)pos.y, PlayerSize, PlayerSize, PlayerColors[i]);
				DrawText(TextFormat("%d", GetPlayerHealth(i)), (int)pos.x, (int)pos.y - 15, 12, PlayerColors[i]);
			}
		}

		for (int i = 0; i < MAX_BULLETS; i++)
		{
			Vector2 pos = { 0 };
			int ownerId = 0;
			if (GetBulletPos(i, &pos, &ownerId))
				DrawCircle((int)pos.x, (int)pos.y, 5, PlayerColors[ownerId]);
		}

		break;
	}
}

// main game client
int main()
{
	SetColors();

	// set up raylib
	InitWindow(FieldSizeWidth, FieldSizeHeight, "ListenClient");
	InitAudioDevice();
	SetTargetFPS(60);

	screenTarget = LoadRenderTexture(FieldSizeWidth, FieldSizeHeight);
	shakeShader = LoadShader(NULL, "resources/screenshake.fs");
	shakeLoc = GetShaderLocation(shakeShader, "shakeStrength");
	timeLoc = GetShaderLocation(shakeShader, "time");

	bulletSFX = LoadSound("resources/bullet.wav");
	hitSFX = LoadSound("resources/hit.wav");
	deathSFX = LoadSound("resources/death.wav");;

	// start listen server on separate thread
	//StartListenServer();
	//WaitTime(0.1);

	// if you want to connect to a server on another machine, change this, or ask the user for the server address
	//Connect(defaultIP);
	State = Connecting;

	while (RunGame)
	{
		UpdateGame();

		BeginTextureMode(screenTarget);
			ClearBackground(GRASS_COLOR);
			DrawGame();
			DrawFPS(0, 0);
		EndTextureMode();

		float strength = (shakeTimer / shakeDuration) * shakePeak;
		SetShaderValue(shakeShader, shakeLoc, &strength, SHADER_UNIFORM_FLOAT);
		float t = (float)GetTime();
		SetShaderValue(shakeShader, timeLoc, &t, SHADER_UNIFORM_FLOAT);

		// draw our game screen
		BeginDrawing();
			ClearBackground(GRASS_COLOR);
			BeginShaderMode(shakeShader);
				DrawTextureRec(screenTarget.texture, (Rectangle) { 0, 0, (float)FieldSizeWidth, -(float)FieldSizeHeight }, (Vector2) { 0, 0 }, WHITE);
				EndShaderMode();
		EndDrawing();
	}

	UnloadRenderTexture(screenTarget);
	UnloadShader(shakeShader);
	UnloadSound(bulletSFX);
	UnloadSound(hitSFX);
	UnloadSound(deathSFX);

	StopListenServer();

	CloseAudioDevice();
	CloseWindow();

	return 0;
}
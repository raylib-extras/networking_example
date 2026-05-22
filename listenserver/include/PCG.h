#ifndef PCG_H
#define PCG_H

#include <raylib.h>
#include "PCG_TYPES.h"

// Visuals & Characters
#define GRASS_CHAR '.'
#define ROCK_CHAR '#'
#define GRASS_COLOR (Color){69, 182, 156, 255}
#define ROCK_COLOR (Color){114, 147, 160, 255}
#define UNKNOWN_COLOR WHITE

// File Names
#define MAP_FILE_NAME "pcg_map_data.fyl"

// Helpers
char GetTileChar(TileType tileType);

// UI Constants
#define BUTTON_WIDTH 200
#define BUTTON_HEIGHT 50
#define BUTTON_X (SCREEN_WIDTH - BUTTON_WIDTH - 20)
#define BUTTON_Y (SCREEN_HEIGHT - BUTTON_HEIGHT - 20)
#define RESET_BUTTON_BOUNDS (Rectangle){BUTTON_X, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT}

// =============================================
// Function Declarations
// =============================================

#ifdef __cplusplus
extern "C" {
#endif

	// PCG Functions
	void PCG_CreateMap(TileType _tileArray[MAP_ROWS][MAP_COLUMNS]);
	void PCG_DrawMap(TileType _tileArray[MAP_ROWS][MAP_COLUMNS], bool isEngine);
	void PCG_PrintMap(TileType _tileArray[MAP_ROWS][MAP_COLUMNS]);
	Color PCG_GetTileColor(TileType tileType);

	// File I/O Functions
	void PCG_SaveMapData(TileType _tileArray[MAP_ROWS][MAP_COLUMNS], const char* fileName);
	void PCG_LoadMapData(TileType _tileArray[MAP_ROWS][MAP_COLUMNS], const char* fileName);

	// UI Functions
	void PCG_DrawGUI(TileType _tileArray[MAP_ROWS][MAP_COLUMNS]);

#ifdef __cplusplus
}
#endif

#endif
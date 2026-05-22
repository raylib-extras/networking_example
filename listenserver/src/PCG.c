#define _CRT_SECURE_NO_WARNINGS

#include "PCG.h"
#include "perlin.h"
//#include "save_dialog.h"
#include <stdio.h>

//#define RAYGUI_IMPLEMENTATION
//#include "raygui.h"

static float s_offsetX = 0.0f;
static float s_offsetY = 0.0f;

// ============================================= 
// PCG_CreateMap
// ============================================= 
void PCG_CreateMap(TileType _tileArray[MAP_ROWS][MAP_COLUMNS]) {
    s_offsetX = (float)GetRandomValue(0, 10000);
    s_offsetY = (float)GetRandomValue(0, 10000);

    for (int y = 0; y < MAP_ROWS; y++) {
        for (int x = 0; x < MAP_COLUMNS; x++) {
            //_tileArray[y][x] = (TileType)GetRandomValue(0, TILE_COUNT - 1);
            float noise = perlinNoise(s_offsetX + (float)x / MAP_COLUMNS * NOISE_SCALE, s_offsetY + (float)y / MAP_ROWS * NOISE_SCALE);
            _tileArray[y][x] = (noise > 0.0f) ? TILE_TYPE_GRASS : TILE_TYPE_ROCK;
            printf("%f ", noise);
        }
        printf("\n");
    }
}

// ============================================= 
// PCG_GetTileColor
// ============================================= 
Color PCG_GetTileColor(TileType tileType) {
    switch (tileType) {
    case TILE_TYPE_GRASS: return GRASS_COLOR;
    case TILE_TYPE_ROCK: return ROCK_COLOR;
    default: return UNKNOWN_COLOR;
    }
}

// ============================================= 
// PCG_DrawMap
// ============================================= 
void PCG_DrawMap(TileType _tileArray[MAP_ROWS][MAP_COLUMNS], bool isEngine) {
    for (int y = 0; y < MAP_ROWS; y++) {
        for (int x = 0; x < MAP_COLUMNS; x++) {
            if (isEngine) {
                Vector3 position = { x, 0.0f, y };
                DrawCube(position, 1.0f, 1.0f, 1.0f, PCG_GetTileColor(_tileArray[y][x]));
            }
            else {
                DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, PCG_GetTileColor(_tileArray[y][x]));
            }
        }
    }
}

// ============================================= 
// PCG_PrintMap
// ============================================= 
void PCG_PrintMap(TileType _tileArray[MAP_ROWS][MAP_COLUMNS]) {
    printf("\n-------Map Layout:--------\n");
    for (int y = 0; y < MAP_ROWS; y++) {
        for (int x = 0; x < MAP_COLUMNS; x++) {
            if (_tileArray[y][x] == TILE_TYPE_GRASS) {
                printf("%c", GRASS_CHAR);
            }
            else {
                printf("%c", ROCK_CHAR);
            }
        }
        printf("\n");
    }
    printf("--------------------------\n");
}

// =============================================
// GetTileChar
// =============================================
char GetTileChar(TileType tileType) {
    switch (tileType) {
    case TILE_TYPE_GRASS: return GRASS_CHAR;
    case TILE_TYPE_ROCK: return ROCK_CHAR;
    default: return '?';
    }
}

// =============================================
// PCG_SaveMapData
// =============================================
/*void PCG_SaveMapData(TileType _tileArray[MAP_ROWS][MAP_COLUMNS], const char* fileName) {
    char filePath[260] = "";

    if (!ShowSaveDialog(filePath, sizeof(filePath), fileName)) {
        printf("Save cancelled.\n");
        return;
    }

    FILE* file = fopen(filePath, "w");
    if (file == NULL) {
        printf("Error opening file for writing: %s\n", filePath);
        return;
    }

    // Save offsets on the first line
    fprintf(file, "%f %f\n", s_offsetX, s_offsetY);

    for (int y = 0; y < MAP_ROWS; y++) {
        for (int x = 0; x < MAP_COLUMNS; x++) {
            fputc(GetTileChar(_tileArray[y][x]), file);
        }
        fputc('\n', file);
    }
    fclose(file);
    printf("Map saved successfully to: %s\n", filePath);
}

// =============================================
// PCG_LoadMapData
// =============================================
void PCG_LoadMapData(TileType _tileArray[MAP_ROWS][MAP_COLUMNS], const char* fileName) {
    FILE* file = fopen(fileName, "r");
    if (file == NULL) {
        printf("Error opening file for reading: %s\n", fileName);
        return;
    }

    // Read offsets from the first line
    fscanf(file, "%f %f\n", &s_offsetX, &s_offsetY);

    for (int y = 0; y < MAP_ROWS; y++) {
        for (int x = 0; x < MAP_COLUMNS; x++) {
            int ch = fgetc(file);
            while (ch == '\n' || ch == '\r') {
                ch = fgetc(file);
            }

            if (ch == GRASS_CHAR) {
                _tileArray[y][x] = TILE_TYPE_GRASS;
            }
            else if (ch == ROCK_CHAR) {
                _tileArray[y][x] = TILE_TYPE_ROCK;
            }
        }
    }
    fclose(file);
}

// =============================================
// PCG_DrawGUI
// =============================================
void PCG_DrawGUI(TileType _tileArray[MAP_ROWS][MAP_COLUMNS]) {
    //Reset Button
    if (GuiButton(RESET_BUTTON_BOUNDS, "Reset Map")) {
        PCG_CreateMap(_tileArray);
    }

    // Save Button
    Rectangle saveRect = { BUTTON_X, BUTTON_Y - 140, BUTTON_WIDTH, BUTTON_HEIGHT };
    if (GuiButton(saveRect, "Save Map")) {
        PCG_SaveMapData(_tileArray, MAP_FILE_NAME);
    }

    //Load Button
    Rectangle loadRect = { BUTTON_X, BUTTON_Y - 70, BUTTON_WIDTH, BUTTON_HEIGHT };
    if (GuiButton(loadRect, "Load Map")) {
        PCG_LoadMapData(_tileArray, MAP_FILE_NAME);
    }
}*/
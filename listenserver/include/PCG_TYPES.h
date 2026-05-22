#pragma once

// some network functions require these vars, but enet/winsock don't like raylib, so we can't include PCG.h

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 800
#define TILE_SIZE     64
#define NOISE_SCALE   3.0f

#define MAP_COLUMNS (SCREEN_WIDTH / TILE_SIZE)
#define MAP_ROWS    (SCREEN_HEIGHT / TILE_SIZE)

typedef enum {
    TILE_TYPE_GRASS,
    TILE_TYPE_ROCK,
    TILE_COUNT
} TileType;
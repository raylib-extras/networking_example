#ifndef perlin_H
#define perlin_H
#include <raylib.h>

float perlinNoise(float x, float y);
float interpolate(float a0, float a1, float w);
float dotGridGradient(int ix, int iy, float x, float y);
Vector2 randomGradient(int ix, int iy);

#endif
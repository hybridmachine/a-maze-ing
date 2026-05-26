#pragma once

#include "camera.h"

#ifndef AMAZEING_HEADLESS
#include <raylib.h>
#else
typedef struct Texture2D {
    unsigned int id;
    int width, height, mipmaps, format;
} Texture2D;
#endif

typedef struct ParallaxLayer {
    Texture2D *tex;
    float factor;
} ParallaxLayer;

void parallax_draw(ParallaxLayer *layers, int count, GameCamera *cam);

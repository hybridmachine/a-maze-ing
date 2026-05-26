#pragma once

#include "util_math.h"

#include <stdbool.h>

typedef enum { LAYER_OBJECT, LAYER_DECOR, LAYER_CHARACTER, LAYER_HUD } SpriteLayer;

typedef struct SpriteDef {
    int id;
    char name[64];
    float origin_x, origin_y;
    float foot_x, foot_y;
    Shape collide;
    Shape proximity;
    float shadow_h;
    SpriteLayer layer;
    float interact_x, interact_y;
} SpriteDef;

bool sprites_load(const char *path);
const SpriteDef *sprites_find(const char *name);
const SpriteDef *sprites_at(int id);
void sprites_shutdown(void);

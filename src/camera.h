#pragma once

#include "util_math.h"

typedef struct GameCamera {
    Vec2 pos;
    Vec2 deadzone_half;
    float smoothing;
} GameCamera;

void camera_init(GameCamera *c, Vec2 start);
void camera_follow(GameCamera *c, Vec2 target, float dt);

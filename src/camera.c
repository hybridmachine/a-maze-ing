#include "camera.h"

#include <math.h>

void camera_init(GameCamera *c, Vec2 start) {
    c->pos = start;
    c->deadzone_half = (Vec2){2.0f, 1.5f};
    c->smoothing = 6.0f;
}

void camera_follow(GameCamera *c, Vec2 target, float dt) {
    Vec2 desired = c->pos;
    float dx = target.x - c->pos.x;
    float dy = target.y - c->pos.y;
    if (fabsf(dx) > c->deadzone_half.x) {
        desired.x = target.x - (dx > 0 ? c->deadzone_half.x : -c->deadzone_half.x);
    }
    if (fabsf(dy) > c->deadzone_half.y) {
        desired.y = target.y - (dy > 0 ? c->deadzone_half.y : -c->deadzone_half.y);
    }
    float t = 1.0f - expf(-c->smoothing * dt);
    c->pos.x = lerpf(c->pos.x, desired.x, t);
    c->pos.y = lerpf(c->pos.y, desired.y, t);
}

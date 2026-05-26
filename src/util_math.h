#pragma once

#include <stdbool.h>

typedef struct {
    float x, y;
} Vec2;

typedef enum { SHAPE_RECT, SHAPE_CAPSULE } ShapeKind;

typedef struct {
    ShapeKind kind;
    float ox, oy;
    float w, h;
} Shape;

Vec2 v2_add(Vec2 a, Vec2 b);
Vec2 v2_sub(Vec2 a, Vec2 b);
Vec2 v2_scale(Vec2 a, float s);
float v2_len(Vec2 a);
float v2_dot(Vec2 a, Vec2 b);
Vec2 v2_norm(Vec2 a);
float clampf(float v, float lo, float hi);
float lerpf(float a, float b, float t);
bool shape_overlap(Shape a, Vec2 pos_a, Shape b, Vec2 pos_b);
Vec2 shape_slide(Vec2 desired, Shape foot, Shape block, Vec2 block_pos);

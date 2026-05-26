#include "util_math.h"

#include <math.h>

Vec2 v2_add(Vec2 a, Vec2 b) {
    return (Vec2){a.x + b.x, a.y + b.y};
}

Vec2 v2_sub(Vec2 a, Vec2 b) {
    return (Vec2){a.x - b.x, a.y - b.y};
}

Vec2 v2_scale(Vec2 a, float s) {
    return (Vec2){a.x * s, a.y * s};
}

float v2_dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

float v2_len(Vec2 a) {
    return sqrtf(v2_dot(a, a));
}

Vec2 v2_norm(Vec2 a) {
    float len = v2_len(a);
    return len > 0.00001f ? v2_scale(a, 1.0f / len) : (Vec2){0, 0};
}

float clampf(float v, float lo, float hi) {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

static void rect_bounds(Shape s, Vec2 p, float *l, float *t, float *r, float *b) {
    *l = p.x + s.ox;
    *t = p.y + s.oy;
    *r = *l + s.w;
    *b = *t + s.h;
}

static bool rect_rect(Shape a, Vec2 pa, Shape b, Vec2 pb) {
    float al, at, ar, ab, bl, bt, br, bb;
    rect_bounds(a, pa, &al, &at, &ar, &ab);
    rect_bounds(b, pb, &bl, &bt, &br, &bb);
    return al < br && ar > bl && at < bb && ab > bt;
}

static bool rect_capsule(Shape rect, Vec2 pr, Shape cap, Vec2 pc) {
    float rl, rt, rr, rb;
    rect_bounds(rect, pr, &rl, &rt, &rr, &rb);
    float radius = cap.w * 0.5f;
    float cx = pc.x + cap.ox + radius;
    float y0 = pc.y + cap.oy + radius;
    float y1 = pc.y + cap.oy + cap.h - radius;
    if (y1 < y0) {
        y1 = y0;
    }
    float closest_y = clampf(clampf(cx, rl, rr), rl, rr);
    (void)closest_y;
    float seg_y = clampf(clampf((rt + rb) * 0.5f, y0, y1), y0, y1);
    float qx = clampf(cx, rl, rr);
    float qy = clampf(seg_y, rt, rb);
    float dx = cx - qx;
    float dy = seg_y - qy;
    return dx * dx + dy * dy <= radius * radius;
}

static bool capsule_capsule(Shape a, Vec2 pa, Shape b, Vec2 pb) {
    float ar = a.w * 0.5f;
    float br = b.w * 0.5f;
    Vec2 ac = {pa.x + a.ox + ar, pa.y + a.oy + a.h * 0.5f};
    Vec2 bc = {pb.x + b.ox + br, pb.y + b.oy + b.h * 0.5f};
    float reach = ar + br + fmaxf(0.0f, (a.h - a.w) * 0.5f) + fmaxf(0.0f, (b.h - b.w) * 0.5f);
    return v2_len(v2_sub(ac, bc)) <= reach;
}

bool shape_overlap(Shape a, Vec2 pos_a, Shape b, Vec2 pos_b) {
    if (a.w <= 0 || a.h <= 0 || b.w <= 0 || b.h <= 0) {
        return false;
    }
    if (a.kind == SHAPE_RECT && b.kind == SHAPE_RECT) {
        return rect_rect(a, pos_a, b, pos_b);
    }
    if (a.kind == SHAPE_RECT && b.kind == SHAPE_CAPSULE) {
        return rect_capsule(a, pos_a, b, pos_b);
    }
    if (a.kind == SHAPE_CAPSULE && b.kind == SHAPE_RECT) {
        return rect_capsule(b, pos_b, a, pos_a);
    }
    return capsule_capsule(a, pos_a, b, pos_b);
}

Vec2 shape_slide(Vec2 desired, Shape foot, Shape block, Vec2 block_pos) {
    if (!shape_overlap(foot, desired, block, block_pos)) {
        return desired;
    }
    float fl, ft, fr, fb, bl, bt, br, bb;
    rect_bounds(foot, desired, &fl, &ft, &fr, &fb);
    rect_bounds(block, block_pos, &bl, &bt, &br, &bb);
    float push_left = br - fl;
    float push_right = fr - bl;
    float push_up = bb - ft;
    float push_down = fb - bt;
    float min_x = push_left < push_right ? -push_left : push_right;
    float min_y = push_up < push_down ? -push_up : push_down;
    if (fabsf(min_x) < fabsf(min_y)) {
        desired.x += min_x;
    } else {
        desired.y += min_y;
    }
    return desired;
}

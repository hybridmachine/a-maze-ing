#include "collision.h"

#include <math.h>

#define TILE_OVERLAP_EPS 0.0001f

static bool blocks_player(const Entity *e) {
    return e->alive && e->collide.w > 0 && e->kind != ENT_HAZARD &&
           !(e->kind == ENT_INTERACT && e->interact.active);
}

static bool tile_blocked(const World *w, Shape foot, Vec2 pos) {
    if (!w->maze.tiles) {
        return false;
    }
    float l = pos.x + foot.ox;
    float t = pos.y + foot.oy;
    float r = l + foot.w;
    float b = t + foot.h;
    int x0 = (int)floorf(l + 0.5f);
    int y0 = (int)floorf(t + 0.5f);
    int x1 = (int)floorf(r + 0.5f - TILE_OVERLAP_EPS);
    int y1 = (int)floorf(b + 0.5f - TILE_OVERLAP_EPS);
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            if (!world_tile_walkable(w, x, y)) {
                return true;
            }
        }
    }
    return false;
}

bool collision_blocked(const World *w, Shape foot, Vec2 pos) {
    if (tile_blocked(w, foot, pos)) {
        return true;
    }
    for (int i = 0; i < w->entity_count; i++) {
        const Entity *e = &w->entities[i];
        if (blocks_player(e) && shape_overlap(foot, pos, e->collide, e->pos)) {
            return true;
        }
    }
    return false;
}

int collision_overlap_query(const World *w, Shape s, Vec2 pos, EntityFilter f, Entity **out,
                            int max) {
    int n = 0;
    for (int i = 0; i < w->entity_count; i++) {
        Entity *e = (Entity *)&w->entities[i];
        if (!e->alive || (f && !f(e))) {
            continue;
        }
        Shape target = e->proximity.w > 0 ? e->proximity : e->collide;
        if (target.w > 0 && shape_overlap(s, pos, target, e->pos)) {
            if (n < max) {
                out[n] = e;
            }
            n++;
        }
    }
    return n;
}

Vec2 collision_slide_player(const World *w, Shape foot, Vec2 from, Vec2 to) {
    if (!collision_blocked(w, foot, to)) {
        return to;
    }
    Vec2 x_only = {to.x, from.y};
    if (!collision_blocked(w, foot, x_only)) {
        return x_only;
    }
    Vec2 y_only = {from.x, to.y};
    if (!collision_blocked(w, foot, y_only)) {
        return y_only;
    }
    return from;
}

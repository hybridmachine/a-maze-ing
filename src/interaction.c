#include "interaction.h"

#include "game.h"

#include <float.h>
#include <math.h>
#include <string.h>

static Vec2 facing_vec(int facing) {
    switch (facing) {
    case 0:
        return (Vec2){0, -1};
    case 1:
        return v2_norm((Vec2){1, -1});
    case 2:
        return (Vec2){1, 0};
    case 3:
        return v2_norm((Vec2){1, 1});
    case 4:
        return (Vec2){0, 1};
    case 5:
        return v2_norm((Vec2){-1, 1});
    case 6:
        return (Vec2){-1, 0};
    default:
        return v2_norm((Vec2){-1, -1});
    }
}

bool interaction_find_target(Game *g, InteractionTarget *out) {
    Shape foot = player_foot_shape();
    Vec2 fwd = facing_vec(g->player.facing);
    Entity *best = NULL;
    float best_score = -FLT_MAX;
    for (int i = 0; i < g->world.entity_count; i++) {
        Entity *e = &g->world.entities[i];
        if (!e->alive || !e->cb || !e->cb->on_interact) {
            continue;
        }
        bool near = e->proximity.w > 0 && shape_overlap(foot, g->player.pos, e->proximity, e->pos);
        Vec2 to = v2_sub(e->pos, g->player.pos);
        float dist = v2_len(to);
        float dot = dist > 0.001f ? v2_dot(v2_scale(to, 1.0f / dist), fwd) : 1.0f;
        bool in_cone = dist <= 1.5f && dot >= 0.5f;
        if (!near && !in_cone) {
            continue;
        }
        float score = dot * 10.0f - dist - (float)e->id * 0.0001f;
        if (!best || score > best_score) {
            best = e;
            best_score = score;
        }
    }
    if (!best) {
        return false;
    }
    *out = (InteractionTarget){.e = best};
    return true;
}

void interaction_dispatch(Game *g, InteractionTarget *target, const char *item_id_or_null) {
    if (!target || !target->e || !target->e->cb || !target->e->cb->on_interact) {
        return;
    }
    target->e->cb->on_interact(g, target->e, item_id_or_null);
}

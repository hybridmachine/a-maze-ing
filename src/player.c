#include "player.h"

#include "collision.h"
#include "entity_hazard.h"
#include "game.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

Shape player_foot_shape(void) {
    return (Shape){SHAPE_RECT, -0.25f, -0.25f, 0.5f, 0.5f};
}

void player_init(Player *p, Vec2 start) {
    memset(p, 0, sizeof *p);
    p->pos = start;
    p->prev_pos = start;
    p->speed = 4.0f;
    p->speed_mul = 1.0f;
    p->facing = 4;
    snprintf(p->outfit, sizeof p->outfit, "default");
    snprintf(p->face, sizeof p->face, "neutral");
}

static int facing_from_dir(Vec2 d) {
    if (fabsf(d.x) > fabsf(d.y)) {
        return d.x > 0 ? 2 : 6;
    }
    if (fabsf(d.y) > 0.001f) {
        return d.y > 0 ? 4 : 0;
    }
    return 4;
}

void player_tick(struct Game *g, Player *p, const InputFrame *in, float dt) {
    p->prev_pos = p->pos;
    p->speed_mul = 1.0f;
    Vec2 d = {0, 0};
    if (in->held[ACT_MOVE_LEFT]) {
        d.x -= 1.0f;
    }
    if (in->held[ACT_MOVE_RIGHT]) {
        d.x += 1.0f;
    }
    if (in->held[ACT_MOVE_UP]) {
        d.y -= 1.0f;
    }
    if (in->held[ACT_MOVE_DOWN]) {
        d.y += 1.0f;
    }
    d = v2_norm(d);
    if (d.x != 0.0f || d.y != 0.0f) {
        p->facing = facing_from_dir(d);
        p->velocity = d;
    } else if (!p->sliding) {
        p->velocity = (Vec2){0, 0};
    }
    Vec2 desired = v2_add(p->pos, v2_scale(p->velocity, p->speed * p->speed_mul * dt));
    p->pos = collision_slide_player(&g->world, player_foot_shape(), p->pos, desired);
    hazard_apply_post_move(g);
    if (d.x != 0.0f || d.y != 0.0f) {
        p->anim_time += dt;
        if (p->anim_time >= 0.15f) {
            p->anim_time = 0.0f;
            p->anim_frame = (p->anim_frame + 1) % 4;
        }
    }
}

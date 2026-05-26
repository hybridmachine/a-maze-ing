#pragma once

#include "input.h"
#include "util_math.h"

typedef struct Player {
    Vec2 pos, prev_pos;
    Vec2 velocity;
    int facing;
    float speed;
    float speed_mul;
    int anim_frame;
    float anim_time;
    char outfit[48];
    char face[32];
    bool sliding;
} Player;

struct Game;
void player_init(Player *p, Vec2 start);
void player_tick(struct Game *g, Player *p, const InputFrame *in, float dt);
Shape player_foot_shape(void);

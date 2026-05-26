#pragma once

#include "world.h"

typedef bool (*EntityFilter)(const Entity *e);

bool collision_blocked(const World *w, Shape foot, Vec2 pos);
int collision_overlap_query(const World *w, Shape s, Vec2 pos, EntityFilter f, Entity **out,
                            int max);
Vec2 collision_slide_player(const World *w, Shape foot, Vec2 from, Vec2 to);

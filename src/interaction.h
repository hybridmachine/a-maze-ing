#pragma once

#include "player.h"
#include "world.h"

struct Game;

typedef struct InteractionTarget {
    Entity *e;
    int applicable_items[16];
    int applicable_count;
} InteractionTarget;

bool interaction_find_target(struct Game *g, InteractionTarget *out);
void interaction_dispatch(struct Game *g, InteractionTarget *target, const char *item_id_or_null);

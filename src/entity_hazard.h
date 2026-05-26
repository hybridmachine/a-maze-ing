#pragma once

#include "entity.h"

struct Game;
void entity_hazard_register(void);
void hazard_apply_post_move(struct Game *g);

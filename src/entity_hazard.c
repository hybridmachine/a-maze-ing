#include "entity_hazard.h"

#include "game.h"
#include "player.h"

#include <string.h>

static void hazard_loaded(struct Game *g, Entity *e) {
    (void)g;
    (void)e;
}

static const EntityCallbacks HAZARD_CB = {.on_loaded = hazard_loaded};

void entity_hazard_register(void) {
    entity_registry_register("hazard", &HAZARD_CB);
    entity_registry_register("water", &HAZARD_CB);
    entity_registry_register("mud", &HAZARD_CB);
    entity_registry_register("ice", &HAZARD_CB);
    entity_registry_register("thorns", &HAZARD_CB);
}

static bool outfit_satisfies(const Player *p, const Entity *e) {
    return e->needs_item[0] && strcmp(p->outfit, e->needs_item) == 0;
}

void hazard_apply_post_move(struct Game *g) {
    Shape foot = player_foot_shape();
    g->player.sliding = false;
    for (int i = 0; i < g->world.entity_count; i++) {
        Entity *e = &g->world.entities[i];
        if (!e->alive || e->kind != ENT_HAZARD) {
            continue;
        }
        Shape s = e->collide.w > 0 ? e->collide : e->proximity;
        if (!shape_overlap(foot, g->player.pos, s, e->pos)) {
            continue;
        }
        if (e->hazard.type == HAZ_MUD) {
            if (e->needs_item[0] && !outfit_satisfies(&g->player, e)) {
                g->player.pos = g->player.prev_pos;
            } else {
                g->player.speed_mul *= 0.5f;
            }
        } else if (e->hazard.type == HAZ_ICE) {
            g->player.sliding = true;
        } else if ((e->hazard.type == HAZ_WATER || e->hazard.type == HAZ_THORNS) &&
                   !outfit_satisfies(&g->player, e)) {
            g->player.pos = g->player.prev_pos;
        }
    }
}

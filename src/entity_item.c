#include "entity_item.h"

#include "entity.h"
#include "game.h"

static void item_on_interact(Game *g, Entity *e, const char *item_id) {
    (void)item_id;
    if (e->item.taken) {
        return;
    }
    const ItemDef *d = items_find(e->item.item_id);
    if (!d) {
        return;
    }
    if (d->scope == SCOPE_PROFILE) {
        game_dirty_profile_item_add(g, d->id);
        if (d->kind == KIND_OUTFIT && g->auto_wear_first) {
            game_dirty_outfit_set(g, d->id);
        }
    } else {
        game_dirty_inventory_add(g, d->id, 1);
    }
    game_dirty_entity_override(g, e->stable_id, 1, 0, -1);
    e->item.taken = true;
    e->alive = false;
    game_request_checkpoint(g, CKPT_ITEM_PICKED);
}

static const EntityCallbacks ITEM_CB = {.on_interact = item_on_interact};

void entity_item_register(void) {
    entity_registry_register("pickup", &ITEM_CB);
}

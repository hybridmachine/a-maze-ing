#include "entity_interact.h"

#include "game.h"
#include "items.h"
#include "sprite_manifest.h"
#include "time_of_day.h"
#include "ui.h"

#include <string.h>

static void door_on_interact(Game *g, Entity *e, const char *item_id) {
    if (e->interact.active) {
        return;
    }
    if (!item_id && game_player_has_item(g, e->needs_item)) {
        item_id = e->needs_item;
    }
    if (!item_id || strcmp(item_id, e->needs_item) != 0) {
        ui_dialog_show_key(g, "hint.door.locked");
        return;
    }
    e->interact.active = true;
    e->collide.w = 0;
    game_dirty_entity_override(g, e->stable_id, 0, 1, -1);
    game_request_checkpoint(g, CKPT_DIALOG_DONE);
}

static void sign_on_interact(Game *g, Entity *e, const char *item_id) {
    (void)item_id;
    ui_dialog_show_key(g, e->hint_key[0] ? e->hint_key : "hint.welcome");
}

static void sundial_on_interact(Game *g, Entity *e, const char *item_id) {
    (void)e;
    (void)item_id;
    int target = (clock_hour(&g->clock) < 12) ? 12 * 60 : 18 * 60;
    ui_fade_then_advance_clock(g, target);
    game_request_checkpoint(g, CKPT_DAY_ROLLED);
}

static void withered_on_interact(Game *g, Entity *e, const char *item_id) {
    if (!item_id && game_player_has_item(g, "seed")) {
        item_id = "seed";
    }
    if (!item_id) {
        ui_dialog_show_key(g, "hint.withered_plant");
        return;
    }
    const ItemDef *d = items_find(item_id);
    if (!d || strcmp(d->use_tag, e->use_tag[0] ? e->use_tag : "water_plant") != 0) {
        ui_dialog_show_key(g, "hint.withered_plant.2");
        return;
    }
    game_dirty_inventory_add(g, item_id, -1);
    e->interact.active = true;
    const SpriteDef *flower = sprites_find("flower.png");
    if (flower) {
        e->sprite_id = flower->id;
    }
    game_dirty_entity_override(g, e->stable_id, 0, 1, -1);
    game_request_checkpoint(g, CKPT_DIALOG_DONE);
}

static const EntityCallbacks DOOR_CB = {.on_interact = door_on_interact};
static const EntityCallbacks SIGN_CB = {.on_interact = sign_on_interact};
static const EntityCallbacks SUNDIAL_CB = {.on_interact = sundial_on_interact};
static const EntityCallbacks WITHERED_CB = {.on_interact = withered_on_interact};

void entity_interact_register(void) {
    entity_registry_register("door", &DOOR_CB);
    entity_registry_register("sign", &SIGN_CB);
    entity_registry_register("sundial", &SUNDIAL_CB);
    entity_registry_register("withered_plant", &WITHERED_CB);
}

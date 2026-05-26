#include "entity_npc.h"

#include "dialogs.h"
#include "game.h"
#include "items.h"
#include "ui.h"

#include <string.h>

static void apply_post_state(Game *g, const char *state) {
    const char *prefix = "give_item:";
    size_t n = strlen(prefix);
    if (strncmp(state, prefix, n) != 0) {
        return;
    }
    const char *item = state + n;
    const ItemDef *d = items_find(item);
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
}

static void npc_on_interact(Game *g, Entity *e, const char *item_id) {
    (void)item_id;
    const DialogDef *d = dialogs_find(e->npc.dialog_id);
    if (!d) {
        return;
    }
    int today = g->clock.day_index;
    int last = game_entity_last_dialog_day(g, e->stable_id);
    if ((d->repeat == REPEAT_ONCE && last >= 0) ||
        (d->repeat == REPEAT_ONCE_PER_DAY && last == today)) {
        return;
    }
    ui_dialog_play(g, d, e);
    game_dirty_entity_override_dialog_day(g, e->stable_id, today);
    if (d->post_state[0]) {
        apply_post_state(g, d->post_state);
    }
    game_request_checkpoint(g, CKPT_DIALOG_DONE);
}

static const EntityCallbacks NPC_CB = {.on_interact = npc_on_interact};

void entity_npc_register(void) {
    entity_registry_register("npc", &NPC_CB);
}

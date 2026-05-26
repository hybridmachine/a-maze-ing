#include "game.h"

#include "entity_hazard.h"
#include "entity_interact.h"
#include "entity_item.h"
#include "entity_npc.h"
#include "profile.h"

#ifndef AMAZEING_HEADLESS
#include "render.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Game *game_create(void) {
    Game *g = calloc(1, sizeof *g);
    if (!g) {
        return NULL;
    }
    g->state = GS_TITLE;
    g->auto_wear_first = true;
    g->dialog_chars_per_second = 40.0f;
    entity_registry_init();
    entity_item_register();
    entity_hazard_register();
    entity_interact_register();
    entity_npc_register();
    player_init(&g->player, (Vec2){0, 0});
    clock_init(&g->clock, 8 * 60);
    camera_init(&g->camera, g->player.pos);
    return g;
}

void game_destroy(Game *g) {
    if (!g) {
        return;
    }
    world_unload(&g->world);
    free(g);
}

void game_set_state(Game *g, GameState s) {
    g->state = s;
}

GameState game_state(const Game *g) {
    return g->state;
}

static void ensure_checkpoint_context(Game *g) {
    g->pending_checkpoint.profile_id = g->active_profile_id;
    if (!g->pending_checkpoint.maze_id[0] && g->world.maze.maze_id[0]) {
        snprintf(g->pending_checkpoint.maze_id, sizeof g->pending_checkpoint.maze_id, "%s",
                 g->world.maze.maze_id);
    }
}

void game_dirty_inventory_add(Game *g, const char *item_id, int delta) {
    ensure_checkpoint_context(g);
    int i = g->pending_checkpoint.inv_add_count;
    if (i < 16) {
        snprintf(g->pending_checkpoint.inv_adds[i].item_id,
                 sizeof g->pending_checkpoint.inv_adds[i].item_id, "%s", item_id);
        g->pending_checkpoint.inv_adds[i].delta = delta;
        g->pending_checkpoint.inv_add_count++;
    }
}

void game_dirty_profile_item_add(Game *g, const char *item_id) {
    ensure_checkpoint_context(g);
    int i = g->pending_checkpoint.profile_item_count;
    if (i < 8) {
        snprintf(g->pending_checkpoint.profile_items[i],
                 sizeof g->pending_checkpoint.profile_items[i], "%s", item_id);
        g->pending_checkpoint.profile_item_count++;
    }
}

void game_dirty_outfit_set(Game *g, const char *outfit_id) {
    ensure_checkpoint_context(g);
    g->pending_checkpoint.outfit_dirty = true;
    snprintf(g->pending_checkpoint.outfit_id, sizeof g->pending_checkpoint.outfit_id, "%s",
             outfit_id);
    snprintf(g->player.outfit, sizeof g->player.outfit, "%s", outfit_id);
}

void game_dirty_entity_override(Game *g, const char *sid, int taken, int active,
                                int last_dialog_day) {
    ensure_checkpoint_context(g);
    int i = g->pending_checkpoint.ovr_count;
    if (i < 32) {
        snprintf(g->pending_checkpoint.ovrs[i].stable_id,
                 sizeof g->pending_checkpoint.ovrs[i].stable_id, "%s", sid);
        g->pending_checkpoint.ovrs[i].taken = taken;
        g->pending_checkpoint.ovrs[i].active = active;
        g->pending_checkpoint.ovrs[i].last_dialog_day = last_dialog_day;
        g->pending_checkpoint.ovr_count++;
    }
}

void game_dirty_entity_override_dialog_day(Game *g, const char *sid, int day) {
    game_dirty_entity_override(g, sid, 0, 0, day);
}

void game_request_checkpoint(Game *g, CheckpointReason reason) {
    ensure_checkpoint_context(g);
    if (g->active_profile_id > 0 && g->pending_checkpoint.maze_id[0]) {
        save_checkpoint(&g->pending_checkpoint, reason);
    }
    memset(&g->pending_checkpoint, 0, sizeof g->pending_checkpoint);
}

bool game_player_has_item(Game *g, const char *item_id) {
    if (!item_id || !*item_id) {
        return true;
    }
    if (strcmp(g->player.outfit, item_id) == 0) {
        return true;
    }
    if (g->active_profile_id <= 0) {
        return false;
    }
    char ids[64][48];
    int n = save_load_profile_items(g->active_profile_id, ids, 64);
    for (int i = 0; i < n; i++) {
        if (strcmp(ids[i], item_id) == 0) {
            return true;
        }
    }
    int count = 0;
    return save_load_maze_inventory_count(g->active_profile_id, g->world.maze.maze_id, item_id,
                                          &count) &&
           count > 0;
}

int game_entity_last_dialog_day(Game *g, const char *stable_id) {
    int day = -1;
    if (g->active_profile_id > 0) {
        save_load_entity_override(g->active_profile_id, g->world.maze.maze_id, stable_id, NULL,
                                  NULL, &day);
    }
    return day;
}

void game_tick(Game *g, const InputFrame *in, float dt) {
    if (g->quit_requested) {
        return;
    }
    if (g->state == GS_TITLE && in->edge_count[ACT_CONFIRM]) {
        game_set_state(g, GS_PROFILE);
    } else if (g->state == GS_PROFILE && in->edge_count[ACT_CONFIRM]) {
        game_set_state(g, GS_MAZE_SELECT);
    } else if (g->state == GS_IN_MAZE) {
        if (in->edge_count[ACT_PAUSE]) {
            game_set_state(g, GS_PAUSE);
            return;
        }
        if (in->edge_count[ACT_INVENTORY]) {
            game_set_state(g, GS_INVENTORY);
            return;
        }
        if (in->edge_count[ACT_INTERACT]) {
            InteractionTarget target;
            if (interaction_find_target(g, &target)) {
                interaction_dispatch(g, &target, NULL);
            }
        }
        int old_hour = clock_hour(&g->clock);
        player_tick(g, &g->player, in, dt);
        clock_tick(&g->clock, dt, g->paused_for_dialog);
        if (clock_hour(&g->clock) != old_hour) {
            int hour = clock_hour(&g->clock);
            for (int i = 0; i < g->world.entity_count; i++) {
                Entity *e = &g->world.entities[i];
                if (e->alive && e->cb && e->cb->on_time_changed) {
                    e->cb->on_time_changed(g, e, hour);
                }
            }
        }
        camera_follow(&g->camera, g->player.pos, dt);
    } else if ((g->state == GS_PAUSE || g->state == GS_INVENTORY || g->state == GS_DIALOG) &&
               in->edge_count[ACT_CANCEL]) {
        game_set_state(g, GS_IN_MAZE);
    }
}

void game_render(Game *g, float alpha) {
#ifndef AMAZEING_HEADLESS
    render_frame(g, alpha);
#else
    (void)g;
    (void)alpha;
#endif
}

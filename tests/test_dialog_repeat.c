#include "test_harness.h"
#include "dialogs.h"
#include "game.h"
#include "interaction.h"
#include "items.h"
#include "profile.h"
#include "save.h"

#include <stdio.h>
#include <string.h>

static Game *dialog_game(const char *db_path) {
    remove(db_path);
    if (!save_open(db_path)) {
        return NULL;
    }
    items_shutdown();
    dialogs_shutdown();
    if (!items_load("tests/fixtures/items.txt") || !dialogs_load("tests/fixtures/dialogs.txt")) {
        return NULL;
    }
    Game *g = game_create();
    g->active_profile_id = profile_create("p");
    snprintf(g->world.maze.maze_id, sizeof g->world.maze.maze_id, "nature");
    clock_init(&g->clock, 8 * 60);
    return g;
}

void test_repeat_once(void) {
    Game *g = dialog_game("/tmp/amz_dialog_once.db");
    ASSERT(g != NULL);
    Entity e = {.kind = ENT_NPC, .alive = true};
    snprintf(e.stable_id, sizeof e.stable_id, "nature.npc");
    snprintf(e.npc.dialog_id, sizeof e.npc.dialog_id, "npc.gardener.greet");
    e.cb = entity_registry_find("npc");
    InteractionTarget target = {.e = &e};
    interaction_dispatch(g, &target, NULL);
    ASSERT_EQ_INT(game_entity_last_dialog_day(g, e.stable_id), 0);
    g->state = GS_IN_MAZE;
    interaction_dispatch(g, &target, NULL);
    ASSERT_EQ_INT(game_entity_last_dialog_day(g, e.stable_id), 0);
    game_destroy(g);
    save_close();
}

void test_repeat_once_per_day(void) {
    Game *g = dialog_game("/tmp/amz_dialog_day.db");
    ASSERT(g != NULL);
    Entity e = {.kind = ENT_NPC, .alive = true};
    snprintf(e.stable_id, sizeof e.stable_id, "nature.beaver");
    snprintf(e.npc.dialog_id, sizeof e.npc.dialog_id, "npc.beaver.idle");
    e.cb = entity_registry_find("npc");
    InteractionTarget target = {.e = &e};
    interaction_dispatch(g, &target, NULL);
    ASSERT_EQ_INT(game_entity_last_dialog_day(g, e.stable_id), 0);
    g->clock.day_index = 1;
    interaction_dispatch(g, &target, NULL);
    ASSERT_EQ_INT(game_entity_last_dialog_day(g, e.stable_id), 1);
    game_destroy(g);
    save_close();
}

void test_post_state_give_item(void) {
    Game *g = dialog_game("/tmp/amz_dialog_item.db");
    ASSERT(g != NULL);
    Entity e = {.kind = ENT_NPC, .alive = true};
    snprintf(e.stable_id, sizeof e.stable_id, "nature.npc");
    snprintf(e.npc.dialog_id, sizeof e.npc.dialog_id, "npc.gardener.greet");
    e.cb = entity_registry_find("npc");
    InteractionTarget target = {.e = &e};
    interaction_dispatch(g, &target, NULL);
    char ids[8][48];
    ASSERT_EQ_INT(save_load_profile_items(g->active_profile_id, ids, 8), 1);
    ASSERT_EQ_INT(strcmp(ids[0], "rain_boots"), 0);
    game_destroy(g);
    save_close();
}

void register_dialog_repeat_tests(void) {
    RUN_TEST(test_repeat_once);
    RUN_TEST(test_repeat_once_per_day);
    RUN_TEST(test_post_state_give_item);
}

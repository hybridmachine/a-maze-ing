#include "test_harness.h"
#include "dialogs.h"
#include "entity_hazard.h"
#include "game.h"
#include "interaction.h"
#include "items.h"
#include "profile.h"
#include "save.h"

#include <stdio.h>
#include <string.h>

static Game *new_saved_game(const char *db_path) {
    remove(db_path);
    if (!save_open(db_path)) {
        return NULL;
    }
    items_shutdown();
    if (!items_load("tests/fixtures/items.txt")) {
        return NULL;
    }
    Game *g = game_create();
    g->active_profile_id = profile_create("p");
    snprintf(g->world.maze.maze_id, sizeof g->world.maze.maze_id, "nature");
    return g;
}

void test_water_blocks_without_waders(void) {
    Game g = {0};
    g.world.entity_count = 1;
    g.world.entities[0] = (Entity){.kind = ENT_HAZARD,
                                   .alive = true,
                                   .pos = {10, 10},
                                   .collide = {SHAPE_RECT, -4, -4, 8, 8}};
    g.world.entities[0].hazard.type = HAZ_WATER;
    g.player.pos = (Vec2){4, 10};
    g.player.prev_pos = (Vec2){4, 10};
    snprintf(g.player.outfit, sizeof g.player.outfit, "default");
    g.player.pos = (Vec2){10, 10};
    hazard_apply_post_move(&g);
    ASSERT_NEAR(g.player.pos.x, 4.0f, 0.01f);
}

void test_deep_mud_blocks_without_rain_boots(void) {
    Game g = {0};
    g.world.entity_count = 1;
    g.world.entities[0] = (Entity){.kind = ENT_HAZARD,
                                   .alive = true,
                                   .pos = {10, 10},
                                   .collide = {SHAPE_RECT, -4, -4, 8, 8}};
    g.world.entities[0].hazard.type = HAZ_MUD;
    snprintf(g.world.entities[0].needs_item, sizeof g.world.entities[0].needs_item, "rain_boots");
    g.player.prev_pos = (Vec2){4, 10};
    g.player.pos = (Vec2){10, 10};
    snprintf(g.player.outfit, sizeof g.player.outfit, "default");
    hazard_apply_post_move(&g);
    ASSERT_NEAR(g.player.pos.x, 4.0f, 0.01f);
    g.player.prev_pos = (Vec2){4, 10};
    g.player.pos = (Vec2){10, 10};
    snprintf(g.player.outfit, sizeof g.player.outfit, "rain_boots");
    hazard_apply_post_move(&g);
    ASSERT_NEAR(g.player.pos.x, 10.0f, 0.01f);
}

void test_pickup_profile_item(void) {
    Game *g = new_saved_game("/tmp/amz_ent_pickup.db");
    ASSERT(g != NULL);
    Entity e = {.kind = ENT_ITEM, .alive = true};
    snprintf(e.stable_id, sizeof e.stable_id, "nature.rb.a");
    snprintf(e.item.item_id, sizeof e.item.item_id, "rain_boots");
    e.item.scope = SCOPE_PROFILE;
    e.cb = entity_registry_find("pickup");
    InteractionTarget target = {.e = &e};
    interaction_dispatch(g, &target, NULL);
    char ids[8][48];
    ASSERT_EQ_INT(save_load_profile_items(g->active_profile_id, ids, 8), 1);
    ASSERT_EQ_INT(strcmp(ids[0], "rain_boots"), 0);
    ASSERT(!e.alive);
    game_destroy(g);
    save_close();
}

void test_door_rejects_wrong_item(void) {
    Game *g = new_saved_game("/tmp/amz_ent_door_bad.db");
    ASSERT(g != NULL);
    ASSERT(save_set_inventory_add(g->active_profile_id, "nature", "seed", 1));
    Entity e = {.kind = ENT_INTERACT, .alive = true};
    snprintf(e.stable_id, sizeof e.stable_id, "nature.gate");
    snprintf(e.needs_item, sizeof e.needs_item, "rain_boots");
    e.collide = (Shape){SHAPE_RECT, -1, -1, 2, 2};
    e.cb = entity_registry_find("door");
    InteractionTarget target = {.e = &e};
    interaction_dispatch(g, &target, "seed");
    ASSERT(!e.interact.active);
    ASSERT_NEAR(e.collide.w, 2.0f, 0.001f);
    int count = 0;
    ASSERT(save_load_maze_inventory_count(g->active_profile_id, "nature", "seed", &count));
    ASSERT_EQ_INT(count, 1);
    game_destroy(g);
    save_close();
}

void test_door_accepts_correct_item(void) {
    Game *g = new_saved_game("/tmp/amz_ent_door_good.db");
    ASSERT(g != NULL);
    ASSERT(save_set_profile_item(g->active_profile_id, "rain_boots"));
    Entity e = {.kind = ENT_INTERACT, .alive = true};
    snprintf(e.stable_id, sizeof e.stable_id, "nature.gate");
    snprintf(e.needs_item, sizeof e.needs_item, "rain_boots");
    e.collide = (Shape){SHAPE_RECT, -1, -1, 2, 2};
    e.cb = entity_registry_find("door");
    InteractionTarget target = {.e = &e};
    interaction_dispatch(g, &target, NULL);
    ASSERT(e.interact.active);
    ASSERT_NEAR(e.collide.w, 0.0f, 0.001f);
    game_destroy(g);
    save_close();
}

void register_entity_behavior_tests(void) {
    RUN_TEST(test_water_blocks_without_waders);
    RUN_TEST(test_deep_mud_blocks_without_rain_boots);
    RUN_TEST(test_pickup_profile_item);
    RUN_TEST(test_door_rejects_wrong_item);
    RUN_TEST(test_door_accepts_correct_item);
}

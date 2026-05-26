#include "test_harness.h"
#include "game.h"
#include "interaction.h"

static void noop_interact(struct Game *g, Entity *e, const char *item) {
    (void)g;
    (void)e;
    (void)item;
}

void test_facing_picks_front_entity(void) {
    Game g = {0};
    g.world.entity_count = 2;
    g.world.entities[0] =
        (Entity){.alive = true, .id = 1, .pos = {10, 10}, .proximity = {SHAPE_RECT, -8, -8, 16, 16}};
    g.world.entities[1] =
        (Entity){.alive = true, .id = 2, .pos = {8, 10}, .proximity = {SHAPE_RECT, -8, -8, 16, 16}};
    static EntityCallbacks cb = {.on_interact = noop_interact};
    g.world.entities[0].cb = &cb;
    g.world.entities[1].cb = &cb;
    g.player.pos = (Vec2){9, 10};
    g.player.facing = 2;
    InteractionTarget target;
    ASSERT(interaction_find_target(&g, &target));
    ASSERT_EQ_INT(target.e->id, 1);
}

void test_id_tie_break(void) {
    Game g = {0};
    g.world.entity_count = 2;
    g.world.entities[0] =
        (Entity){.alive = true, .id = 2, .pos = {10, 10}, .proximity = {SHAPE_RECT, -8, -8, 16, 16}};
    g.world.entities[1] =
        (Entity){.alive = true, .id = 1, .pos = {10, 10}, .proximity = {SHAPE_RECT, -8, -8, 16, 16}};
    static EntityCallbacks cb = {.on_interact = noop_interact};
    g.world.entities[0].cb = &cb;
    g.world.entities[1].cb = &cb;
    g.player.pos = (Vec2){9, 10};
    g.player.facing = 2;
    InteractionTarget target;
    ASSERT(interaction_find_target(&g, &target));
    ASSERT_EQ_INT(target.e->id, 1);
}

void register_interaction_tests(void) {
    RUN_TEST(test_facing_picks_front_entity);
    RUN_TEST(test_id_tie_break);
}

#include "test_harness.h"
#include "collision.h"
#include "game.h"
#include "items.h"
#include "player.h"

static Game *new_nature_game(void) {
    items_shutdown();
    if (!items_load("data/items.txt")) {
        return NULL;
    }
    Game *g = game_create();
    if (!g) {
        return NULL;
    }
    if (!world_load_maze(&g->world, "data/mazes/nature.maze", "assets/themes/nature")) {
        game_destroy(g);
        return NULL;
    }
    player_init(&g->player,
                (Vec2){(float)g->world.maze.start_x, (float)g->world.maze.start_y});
    clock_init(&g->clock, g->world.maze.time_start_min);
    g->state = GS_IN_MAZE;
    return g;
}

void test_nature_spawn_is_walkable(void) {
    Game *g = new_nature_game();
    ASSERT(g != NULL);
    ASSERT(world_tile_walkable(&g->world, g->world.maze.start_x, g->world.maze.start_y));
    ASSERT(!collision_blocked(&g->world, player_foot_shape(), g->player.pos));
    game_destroy(g);
}

void test_nature_player_moves_from_start(void) {
    Game *g = new_nature_game();
    ASSERT(g != NULL);
    Vec2 start = g->player.pos;
    InputFrame in = {0};
    in.held[ACT_MOVE_DOWN] = true;
    for (int i = 0; i < 20; i++) {
        game_tick(g, &in, 1.0f / 60.0f);
    }
    ASSERT(g->player.pos.y > start.y + 0.75f);
    game_destroy(g);
}

void test_nature_has_blocking_maze_walls(void) {
    Game *g = new_nature_game();
    ASSERT(g != NULL);
    ASSERT(!world_tile_walkable(&g->world, 0, 0));
    ASSERT(!world_tile_walkable(&g->world, 3, 2));
    ASSERT(world_tile_walkable(&g->world, 1, 1));
    game_destroy(g);
}

void register_game_fixture_tests(void) {
    RUN_TEST(test_nature_spawn_is_walkable);
    RUN_TEST(test_nature_player_moves_from_start);
    RUN_TEST(test_nature_has_blocking_maze_walls);
}

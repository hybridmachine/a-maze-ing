#include "test_harness.h"
#include "maze_data.h"
#include "world.h"

#include <string.h>

void test_maze_header(void) {
    MazeFile m = {0};
    ASSERT(maze_data_load("tests/fixtures/mini.maze", &m));
    ASSERT_EQ_INT(m.width, 8);
    ASSERT_EQ_INT(m.height, 4);
    ASSERT_EQ_INT(m.start_x, 1);
    ASSERT_EQ_INT(m.start_y, 1);
    ASSERT_EQ_INT(m.time_start_min, 8 * 60);
    ASSERT_EQ_INT(m.ambient_count, 2);
    maze_data_free(&m);
}

void test_maze_entities(void) {
    MazeFile m = {0};
    ASSERT(maze_data_load("tests/fixtures/mini.maze", &m));
    ASSERT_EQ_INT(m.entity_count, 2);
    ASSERT_EQ_INT(strcmp(m.entities[0].behavior, "pickup"), 0);
    ASSERT_EQ_INT(strcmp(m.entities[0].stable_id, "test.seed.a"), 0);
    ASSERT_EQ_INT(m.entities[0].tile_x, 2);
    ASSERT_EQ_INT(strcmp(m.entities[1].args[0], "needs:rain_boots"), 0);
    ASSERT(m.entities[1].has_shape_override);
    ASSERT(m.entities[1].has_proximity_override);
    ASSERT(!m.entities[1].has_collide_override);
    maze_data_free(&m);
}

void test_maze_tiles(void) {
    MazeFile m = {0};
    ASSERT(maze_data_load("tests/fixtures/mini.maze", &m));
    ASSERT_EQ_INT(m.tiles[1 * 8 + 1], 'G');
    ASSERT_EQ_INT(m.tiles[0], '.');
    maze_data_free(&m);
}

void test_maze_zones(void) {
    MazeFile m = {0};
    ASSERT(maze_data_load("tests/fixtures/mini.maze", &m));
    ASSERT_EQ_INT(m.zone_count, 1);
    ASSERT_EQ_INT(m.zones[0].x0, 1);
    ASSERT_EQ_INT(strcmp(m.zones[0].clip, "water_creek.ogg"), 0);
    maze_data_free(&m);
}

void test_world_walkable(void) {
    entity_registry_init();
    entity_register_builtin_stubs();
    World w = {0};
    ASSERT(world_load_maze(&w, "tests/fixtures/mini.maze", "tests/fixtures/themes/test"));
    ASSERT(world_tile_walkable(&w, 0, 0));
    ASSERT(!world_tile_walkable(&w, 1, 1));
    world_unload(&w);
}

void register_maze_data_tests(void) {
    RUN_TEST(test_maze_header);
    RUN_TEST(test_maze_entities);
    RUN_TEST(test_maze_tiles);
    RUN_TEST(test_maze_zones);
    RUN_TEST(test_world_walkable);
}

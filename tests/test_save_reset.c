#include "test_harness.h"
#include "save.h"

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

void test_reset_clears_maze_state_preserves_profile(void) {
    remove("/tmp/amz_reset.db");
    ASSERT(save_open("/tmp/amz_reset.db"));
    sqlite3_stmt *s = NULL;
    sqlite3_prepare_v2(save_db_handle(),
                       "INSERT INTO profiles(name,created_at,last_played) VALUES('p',0,0);", -1,
                       &s, NULL);
    sqlite3_step(s);
    sqlite3_finalize(s);
    int pid = (int)sqlite3_last_insert_rowid(save_db_handle());
    ASSERT(save_set_profile_item(pid, "rain_boots"));
    ASSERT(save_set_outfit_worn(pid, "rain_boots"));
    ASSERT(save_set_inventory_add(pid, "nature", "seed", 1));
    ASSERT(save_set_entity_override(pid, "nature", "nature.seed.west_glade", 1, 0, -1));
    ASSERT(save_reset_maze(pid, "nature"));
    char ids[8][48];
    ASSERT_EQ_INT(save_load_maze_inventory(pid, "nature", ids, 8), 0);
    ASSERT_EQ_INT(save_load_profile_items(pid, ids, 8), 1);
    ASSERT_EQ_INT(strcmp(ids[0], "rain_boots"), 0);
    char outfit[48];
    ASSERT(save_load_outfit_worn(pid, outfit));
    ASSERT_EQ_INT(strcmp(outfit, "rain_boots"), 0);
    MazeSnapshot snap;
    ASSERT(!save_load_maze_snapshot(pid, "nature", &snap));
    save_close();
}

void register_save_reset_tests(void) {
    RUN_TEST(test_reset_clears_maze_state_preserves_profile);
}

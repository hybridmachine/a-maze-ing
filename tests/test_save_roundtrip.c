#include "test_harness.h"
#include "profile.h"
#include "save.h"

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

void test_open_creates_db(void) {
    remove("/tmp/amazeing_test.db");
    ASSERT(save_open("/tmp/amazeing_test.db"));
    ASSERT(save_integrity_check());
    save_close();
}

void test_inventory_roundtrip(void) {
    remove("/tmp/amz_rt.db");
    ASSERT(save_open("/tmp/amz_rt.db"));
    sqlite3_stmt *s = NULL;
    sqlite3_prepare_v2(save_db_handle(),
                       "INSERT INTO profiles(name,created_at,last_played) VALUES('p',0,0);", -1,
                       &s, NULL);
    sqlite3_step(s);
    sqlite3_finalize(s);
    int pid = (int)sqlite3_last_insert_rowid(save_db_handle());
    ASSERT(save_set_inventory_add(pid, "nature", "seed", 1));
    ASSERT(save_set_inventory_add(pid, "nature", "seed", 2));
    char ids[8][48];
    int n = save_load_maze_inventory(pid, "nature", ids, 8);
    ASSERT_EQ_INT(n, 1);
    ASSERT_EQ_INT(strcmp(ids[0], "seed"), 0);
    int count = 0;
    ASSERT(save_load_maze_inventory_count(pid, "nature", "seed", &count));
    ASSERT_EQ_INT(count, 3);
    save_close();
}

void test_profile_crud(void) {
    remove("/tmp/amz_prof.db");
    ASSERT(save_open("/tmp/amz_prof.db"));
    int a = profile_create("Alice");
    ASSERT(a > 0);
    int b = profile_create("Bob");
    ASSERT(b > 0);
    Profile ps[4];
    ASSERT_EQ_INT(profile_list(ps, 4), 2);
    ASSERT(profile_delete(a));
    ASSERT_EQ_INT(profile_list(ps, 4), 1);
    save_close();
}

void test_profile_cap(void) {
    remove("/tmp/amz_pcap.db");
    ASSERT(save_open("/tmp/amz_pcap.db"));
    ASSERT(profile_create("a") > 0);
    ASSERT(profile_create("b") > 0);
    ASSERT(profile_create("c") > 0);
    ASSERT(profile_create("d") > 0);
    ASSERT_EQ_INT(profile_create("e"), -1);
    save_close();
}

void register_save_tests(void) {
    RUN_TEST(test_open_creates_db);
    RUN_TEST(test_inventory_roundtrip);
    RUN_TEST(test_profile_crud);
    RUN_TEST(test_profile_cap);
}

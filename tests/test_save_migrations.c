#include "test_harness.h"
#include "save.h"

#include <sqlite3.h>
#include <stdio.h>

void test_migrations_create_schema(void) {
    remove("/tmp/amz_mig.db");
    ASSERT(save_open("/tmp/amz_mig.db"));
    sqlite3_stmt *s = NULL;
    sqlite3_prepare_v2(save_db_handle(),
                       "SELECT count(*) FROM sqlite_master WHERE name='profiles';", -1, &s, NULL);
    ASSERT(sqlite3_step(s) == SQLITE_ROW);
    ASSERT_EQ_INT(sqlite3_column_int(s, 0), 1);
    sqlite3_finalize(s);
    save_close();
}

void test_reopen_is_idempotent(void) {
    ASSERT(save_open("/tmp/amz_mig.db"));
    save_close();
    ASSERT(save_open("/tmp/amz_mig.db"));
    save_close();
}

void register_save_migration_tests(void) {
    RUN_TEST(test_migrations_create_schema);
    RUN_TEST(test_reopen_is_idempotent);
}

#include "test_harness.h"
#include "dialogs.h"

#include <string.h>

void test_dialogs_load(void) {
    ASSERT(dialogs_load("tests/fixtures/dialogs.txt"));
    const DialogDef *d = dialogs_find("npc.gardener.greet");
    ASSERT(d != NULL);
    ASSERT_EQ_INT(d->repeat, REPEAT_ONCE);
    ASSERT_EQ_INT(strcmp(d->post_state, "give_item:rain_boots"), 0);
    ASSERT_EQ_INT(d->line_count, 1);
    dialogs_shutdown();
}

void test_dialogs_repeat_once_per_day(void) {
    ASSERT(dialogs_load("tests/fixtures/dialogs.txt"));
    const DialogDef *d = dialogs_find("npc.beaver.idle");
    ASSERT(d != NULL);
    ASSERT_EQ_INT(d->repeat, REPEAT_ONCE_PER_DAY);
    ASSERT_EQ_INT(d->line_count, 2);
    dialogs_shutdown();
}

void register_dialogs_tests(void) {
    RUN_TEST(test_dialogs_load);
    RUN_TEST(test_dialogs_repeat_once_per_day);
}

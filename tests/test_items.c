#include "test_harness.h"
#include "items.h"

#include <string.h>

void test_items_load(void) {
    ASSERT(items_load("tests/fixtures/items.txt"));
    ASSERT_EQ_INT(items_count(), 3);
    const ItemDef *d = items_find("watering_can");
    ASSERT(d != NULL);
    ASSERT_EQ_INT(d->scope, SCOPE_PROFILE);
    ASSERT_EQ_INT(d->kind, KIND_TOOL);
    ASSERT_EQ_INT(strcmp(d->use_tag, "water_plant"), 0);
    items_shutdown();
}

void test_items_unknown_returns_null(void) {
    ASSERT(items_load("tests/fixtures/items.txt"));
    ASSERT(items_find("not_a_thing") == NULL);
    items_shutdown();
}

void register_items_tests(void) {
    RUN_TEST(test_items_load);
    RUN_TEST(test_items_unknown_returns_null);
}

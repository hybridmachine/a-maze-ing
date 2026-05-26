#include "test_harness.h"
#include "localization.h"

#include <string.h>

void test_parse_basic(void) {
    LangTable table = {0};
    ASSERT(loc_parse_file("tests/fixtures/en.lang", &table));
    ASSERT_EQ_INT(table.count, 4);
    bool found = false;
    for (size_t i = 0; i < table.count; i++) {
        if (strcmp(table.entries[i].key, "item.seed.name") == 0 &&
            strcmp(table.entries[i].value, "Seed") == 0) {
            found = true;
        }
    }
    ASSERT(found);
    loc_table_free(&table);
}

void test_parse_malformed_recovers(void) {
    LangTable table = {0};
    ASSERT(loc_parse_file("tests/fixtures/malformed.lang", &table));
    bool found = false;
    for (size_t i = 0; i < table.count; i++) {
        if (strcmp(table.entries[i].key, "key.trim") == 0 &&
            strcmp(table.entries[i].value, "trimmed value") == 0) {
            found = true;
        }
    }
    ASSERT(found);
    loc_table_free(&table);
}

void test_t_falls_back_to_english(void) {
    ASSERT(loc_load_language_from("tests/fixtures", "de"));
    ASSERT_EQ_INT(strcmp(t("ui.title.start"), "Start (DE)"), 0);
    ASSERT_EQ_INT(strcmp(t("ui.title.quit"), "Quit"), 0);
    ASSERT_EQ_INT(strcmp(t("missing.key"), "missing.key"), 0);
    loc_shutdown();
}

void register_localization_tests(void) {
    RUN_TEST(test_parse_basic);
    RUN_TEST(test_parse_malformed_recovers);
    RUN_TEST(test_t_falls_back_to_english);
}

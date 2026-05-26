#include "test_harness.h"
#include "validate.h"

#include <stdbool.h>
#include <string.h>

void test_missing_header(void) {
    Diag diags[16];
    int n = validate_run("tests/fixtures/bad/missing_header.maze", diags, 16);
    ASSERT(n > 0);
    ASSERT_EQ_INT(diags[0].severity, DIAG_ERROR);
    ASSERT(strstr(diags[0].msg, "name") != NULL);
}

void test_duplicate_stable_id(void) {
    Diag diags[16];
    int n = validate_run("tests/fixtures/bad/duplicate_id.maze", diags, 16);
    bool found = false;
    for (int i = 0; i < n; i++) {
        if (strstr(diags[i].msg, "duplicate")) {
            found = true;
        }
    }
    ASSERT(found);
}

void test_unknown_item_reference(void) {
    Diag diags[16];
    int n = validate_run("tests/fixtures/bad/unknown_item.maze", diags, 16);
    bool found = false;
    for (int i = 0; i < n; i++) {
        if (strstr(diags[i].msg, "unknown item")) {
            found = true;
        }
    }
    ASSERT(found);
}

void test_unreachable_pickup(void) {
    Diag diags[16];
    int n = validate_run("tests/fixtures/bad/unreachable.maze", diags, 16);
    bool found = false;
    for (int i = 0; i < n; i++) {
        if (strstr(diags[i].msg, "unreachable")) {
            found = true;
        }
    }
    ASSERT(found);
}

void test_maze_prefix_convention_warning(void) {
    Diag diags[16];
    int n = validate_run("tests/fixtures/bad/wrong_prefix.maze", diags, 16);
    bool found = false;
    for (int i = 0; i < n; i++) {
        if (diags[i].severity == DIAG_WARN) {
            found = true;
        }
    }
    ASSERT(found);
}

void test_nature_maze_validates_cleanly(void) {
    Diag diags[16];
    int n = validate_run("data/mazes/nature.maze", diags, 16);
    ASSERT_EQ_INT(n, 0);
}

void register_validator_tests(void) {
    RUN_TEST(test_missing_header);
    RUN_TEST(test_duplicate_stable_id);
    RUN_TEST(test_unknown_item_reference);
    RUN_TEST(test_unreachable_pickup);
    RUN_TEST(test_maze_prefix_convention_warning);
    RUN_TEST(test_nature_maze_validates_cleanly);
}

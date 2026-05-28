#include "test_harness.h"
#include "util_paths.h"

#include <string.h>

void test_content_path_falls_back_to_relative(void) {
    char path[256];
    ASSERT(util_paths_content_path("data/strings", path, sizeof path));
    ASSERT_EQ_INT(strcmp(path, "data/strings"), 0);
}

void test_content_path_rejects_invalid_args(void) {
    char path[8];
    ASSERT(!util_paths_content_path(NULL, path, sizeof path));
    ASSERT(!util_paths_content_path("", path, sizeof path));
    ASSERT(!util_paths_content_path("data/strings", NULL, sizeof path));
    ASSERT(!util_paths_content_path("data/strings", path, 0));
}

void test_content_path_rejects_too_small_buffer(void) {
    char path[4];
    ASSERT(!util_paths_content_path("data/strings", path, sizeof path));
}

void register_util_paths_tests(void) {
    RUN_TEST(test_content_path_falls_back_to_relative);
    RUN_TEST(test_content_path_rejects_invalid_args);
    RUN_TEST(test_content_path_rejects_too_small_buffer);
}

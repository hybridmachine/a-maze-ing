#pragma once

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int g_test_passed;
extern int g_test_failed;
extern const char *g_current_test;

#define ASSERT(cond)                                                                           \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            fprintf(stderr, "  FAIL %s:%d in %s: %s\n", __FILE__, __LINE__, g_current_test,    \
                    #cond);                                                                    \
            g_test_failed++;                                                                   \
            return;                                                                            \
        }                                                                                      \
    } while (0)

#define ASSERT_EQ_INT(a, b)                                                                    \
    do {                                                                                       \
        long _a = (long)(a);                                                                   \
        long _b = (long)(b);                                                                   \
        if (_a != _b) {                                                                        \
            fprintf(stderr, "  FAIL %s:%d in %s: %s != %s (%ld vs %ld)\n", __FILE__, __LINE__, \
                    g_current_test, #a, #b, _a, _b);                                           \
            g_test_failed++;                                                                   \
            return;                                                                            \
        }                                                                                      \
    } while (0)

#define ASSERT_NEAR(a, b, eps)                                                                 \
    do {                                                                                       \
        double _a = (double)(a);                                                               \
        double _b = (double)(b);                                                               \
        double _eps = (double)(eps);                                                           \
        if (fabs(_a - _b) > _eps) {                                                            \
            fprintf(stderr, "  FAIL %s:%d in %s: %s !~ %s (%.6f vs %.6f)\n", __FILE__,         \
                    __LINE__, g_current_test, #a, #b, _a, _b);                                 \
            g_test_failed++;                                                                   \
            return;                                                                            \
        }                                                                                      \
    } while (0)

#define RUN_TEST(fn)                                                                           \
    do {                                                                                       \
        g_current_test = #fn;                                                                  \
        int _prev_failed = g_test_failed;                                                      \
        fn();                                                                                  \
        if (g_test_failed == _prev_failed) {                                                    \
            g_test_passed++;                                                                   \
            printf("  ok %s\n", #fn);                                                         \
        }                                                                                      \
    } while (0)

#define TEST_SUMMARY()                                                                         \
    do {                                                                                       \
        printf("\n%d passed, %d failed\n", g_test_passed, g_test_failed);                     \
        return g_test_failed == 0 ? 0 : 1;                                                     \
    } while (0)

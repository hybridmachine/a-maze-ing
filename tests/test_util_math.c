#include "test_harness.h"
#include "util_math.h"

void test_v2_add(void) {
    Vec2 r = v2_add((Vec2){1, 2}, (Vec2){3, 4});
    ASSERT_NEAR(r.x, 4.0f, 0.0001f);
    ASSERT_NEAR(r.y, 6.0f, 0.0001f);
}

void test_rect_rect_overlap(void) {
    Shape a = {SHAPE_RECT, 0, 0, 10, 10};
    Shape b = {SHAPE_RECT, 0, 0, 10, 10};
    ASSERT(shape_overlap(a, (Vec2){0, 0}, b, (Vec2){5, 5}));
    ASSERT(!shape_overlap(a, (Vec2){0, 0}, b, (Vec2){20, 20}));
}

void test_slide_resolves_overlap(void) {
    Shape foot = {SHAPE_RECT, -2, -2, 4, 4};
    Shape block = {SHAPE_RECT, 0, 0, 10, 10};
    Vec2 out = shape_slide((Vec2){5, 0}, foot, block, (Vec2){10, 0});
    ASSERT(out.x + 2 <= 10.0001f);
}

void register_util_math_tests(void) {
    RUN_TEST(test_v2_add);
    RUN_TEST(test_rect_rect_overlap);
    RUN_TEST(test_slide_resolves_overlap);
}

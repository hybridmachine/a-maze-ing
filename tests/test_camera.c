#include "test_harness.h"
#include "camera.h"

void test_camera_inside_deadzone_does_not_move(void) {
    GameCamera c;
    camera_init(&c, (Vec2){10, 10});
    c.deadzone_half = (Vec2){2, 2};
    camera_follow(&c, (Vec2){11, 11}, 1.0f);
    ASSERT_NEAR(c.pos.x, 10.0f, 0.001f);
}

void test_camera_outside_eases(void) {
    GameCamera c;
    camera_init(&c, (Vec2){0, 0});
    c.deadzone_half = (Vec2){1, 1};
    c.smoothing = 1.0f;
    camera_follow(&c, (Vec2){10, 0}, 1.0f);
    ASSERT(c.pos.x > 1.0f && c.pos.x < 9.0f);
}

void register_camera_tests(void) {
    RUN_TEST(test_camera_inside_deadzone_does_not_move);
    RUN_TEST(test_camera_outside_eases);
}

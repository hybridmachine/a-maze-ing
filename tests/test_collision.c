#include "test_harness.h"
#include "collision.h"

void test_blocked_against_entity(void) {
    World w = {0};
    w.entity_count = 1;
    w.entities[0].alive = true;
    w.entities[0].pos = (Vec2){10, 10};
    w.entities[0].collide = (Shape){SHAPE_RECT, -4, -4, 8, 8};
    Shape foot = {SHAPE_RECT, -2, -2, 4, 4};
    ASSERT(collision_blocked(&w, foot, (Vec2){10, 10}));
    ASSERT(!collision_blocked(&w, foot, (Vec2){30, 30}));
}

void test_slide_falls_back_axis(void) {
    World w = {0};
    w.entity_count = 1;
    w.entities[0].alive = true;
    w.entities[0].pos = (Vec2){10, 10};
    w.entities[0].collide = (Shape){SHAPE_RECT, -4, -4, 8, 8};
    Shape foot = {SHAPE_RECT, -2, -2, 4, 4};
    Vec2 out = collision_slide_player(&w, foot, (Vec2){3, 3}, (Vec2){7, 7});
    ASSERT(!collision_blocked(&w, foot, out));
    ASSERT(out.x == 7 || out.y == 7);
}

void register_collision_tests(void) {
    RUN_TEST(test_blocked_against_entity);
    RUN_TEST(test_slide_falls_back_axis);
}

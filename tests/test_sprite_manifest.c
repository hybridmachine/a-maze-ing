#include "test_harness.h"
#include "sprite_manifest.h"

void test_sprite_load(void) {
    ASSERT(sprites_load("tests/fixtures/manifest.txt"));
    const SpriteDef *s = sprites_find("tree_oak.png");
    ASSERT(s != NULL);
    ASSERT_NEAR(s->shadow_h, 1.4f, 0.0001f);
    ASSERT_EQ_INT(s->layer, LAYER_OBJECT);
    ASSERT_EQ_INT(s->collide.kind, SHAPE_RECT);
    ASSERT_NEAR(s->collide.w, 16.0f, 0.001f);
    sprites_shutdown();
}

void register_sprite_manifest_tests(void) {
    RUN_TEST(test_sprite_load);
}

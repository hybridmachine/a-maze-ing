#include "test_harness.h"
#include "shadow.h"

#include <math.h>

void test_shadow_at_noon_is_short(void) {
    ShadowOpts noon = shadow_for(sun_at(12 * 60), 1.0f);
    ShadowOpts dawn = shadow_for(sun_at(6 * 60), 1.0f);
    ASSERT(fabsf(noon.offx) < fabsf(dawn.offx));
    ASSERT(noon.scale_y < dawn.scale_y);
}

void register_shadow_tests(void) {
    RUN_TEST(test_shadow_at_noon_is_short);
}

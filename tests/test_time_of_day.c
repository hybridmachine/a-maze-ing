#include "test_harness.h"
#include "time_of_day.h"

void test_clock_rolls_day(void) {
    Clock c;
    clock_init(&c, 23 * 60 + 59);
    for (int i = 0; i < 31; i++) {
        clock_tick(&c, 0.1f, false);
    }
    ASSERT_EQ_INT(c.day_index, 1);
    ASSERT_EQ_INT(c.minute, 0);
}

void test_clock_pause_freezes(void) {
    Clock c;
    clock_init(&c, 10 * 60);
    for (int i = 0; i < 100; i++) {
        clock_tick(&c, 0.1f, true);
    }
    ASSERT_EQ_INT(c.minute, 10 * 60);
}

void test_advance_to_wraps(void) {
    Clock c;
    clock_init(&c, 23 * 60);
    clock_advance_to(&c, 60);
    ASSERT_EQ_INT(c.day_index, 1);
    ASSERT_EQ_INT(c.minute, 60);
}

void test_gate_wraps_midnight(void) {
    ASSERT(entity_gate_active(22, 4, 23));
    ASSERT(entity_gate_active(22, 4, 3));
    ASSERT(!entity_gate_active(22, 4, 12));
}

void test_sun_noon_short_shadow(void) {
    SunVector noon = sun_at(12 * 60);
    SunVector dawn = sun_at(6 * 60);
    ASSERT(noon.length < dawn.length);
    ASSERT(noon.elevation > dawn.elevation);
}

void register_time_of_day_tests(void) {
    RUN_TEST(test_clock_rolls_day);
    RUN_TEST(test_clock_pause_freezes);
    RUN_TEST(test_advance_to_wraps);
    RUN_TEST(test_gate_wraps_midnight);
    RUN_TEST(test_sun_noon_short_shadow);
}

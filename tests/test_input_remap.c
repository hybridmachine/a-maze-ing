#include "test_harness.h"
#include "input.h"

void test_swap_required_on_collision(void) {
    input_reset_to_defaults();
    Action other = ACT_COUNT;
    RemapStatus s = input_propose_binding(ACT_MOVE_UP, 69, &other);
    ASSERT_EQ_INT(s, REMAP_SWAP_REQUIRED);
    ASSERT_EQ_INT(other, ACT_INTERACT);
    input_apply_swap(ACT_MOVE_UP, 69, other);
    ASSERT_EQ_INT(input_get_binding(ACT_MOVE_UP), 69);
    ASSERT_EQ_INT(input_get_binding(ACT_INTERACT), 0);
}

void test_reserved_escape(void) {
    input_reset_to_defaults();
    Action other;
    ASSERT_EQ_INT(input_propose_binding(ACT_MOVE_UP, 256, &other), REMAP_RESERVED);
}

void test_missing_required_after_unbind(void) {
    input_reset_to_defaults();
    Action other;
    input_propose_binding(ACT_INTERACT, 87, &other);
    input_apply_swap(ACT_INTERACT, 87, ACT_MOVE_UP);
    ASSERT(input_missing_required());
}

void test_load_recovery_from_corrupt(void) {
    input_reset_to_defaults();
    int interact_default = input_get_binding(ACT_INTERACT);
    input_load_corrupt_for_tests();
    ASSERT_EQ_INT(input_get_binding(ACT_INTERACT), interact_default);
    ASSERT(input_load_recovery_notice());
}

void register_input_remap_tests(void) {
    RUN_TEST(test_swap_required_on_collision);
    RUN_TEST(test_reserved_escape);
    RUN_TEST(test_missing_required_after_unbind);
    RUN_TEST(test_load_recovery_from_corrupt);
}

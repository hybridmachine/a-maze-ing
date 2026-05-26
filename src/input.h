#pragma once

#include <stdbool.h>

typedef enum {
    ACT_MOVE_UP,
    ACT_MOVE_DOWN,
    ACT_MOVE_LEFT,
    ACT_MOVE_RIGHT,
    ACT_INTERACT,
    ACT_INVENTORY,
    ACT_PAUSE,
    ACT_CONFIRM,
    ACT_CANCEL,
    ACT_COUNT
} Action;

typedef struct {
    bool held[ACT_COUNT];
    int edge_count[ACT_COUNT];
} InputFrame;

typedef enum { REMAP_OK, REMAP_RESERVED, REMAP_SWAP_REQUIRED } RemapStatus;

void input_init(void);
void input_load_bindings(int profile_id);
bool input_set_binding(Action a, int raylib_key);
int input_get_binding(Action a);
void input_reset_to_defaults(void);
bool input_missing_required(void);
void input_sample(void);
void input_consume(InputFrame *out);
RemapStatus input_propose_binding(Action a, int raylib_key, Action *displaces_out);
void input_apply_swap(Action a, int raylib_key, Action displaced);
void input_save_bindings(int profile_id);
bool input_load_recovery_notice(void);

#ifdef AMAZEING_TESTS
void input_load_corrupt_for_tests(void);
#endif

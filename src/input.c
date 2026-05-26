#include "input.h"

#include "save.h"
#include "util_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef AMAZEING_HEADLESS
#include <raylib.h>
#endif

enum {
    AMZ_KEY_A = 65,
    AMZ_KEY_D = 68,
    AMZ_KEY_E = 69,
    AMZ_KEY_I = 73,
    AMZ_KEY_S = 83,
    AMZ_KEY_W = 87,
    AMZ_KEY_ESCAPE = 256,
    AMZ_KEY_ENTER = 257,
    AMZ_KEY_RIGHT = 262,
    AMZ_KEY_LEFT = 263,
    AMZ_KEY_DOWN = 264,
    AMZ_KEY_UP = 265
};

static int g_bindings[ACT_COUNT];
static InputFrame g_buffer;
static bool g_recovered;

static const char *action_setting_key(Action a) {
    static const char *keys[] = {
        "binding.move_up", "binding.move_down", "binding.move_left",
        "binding.move_right", "binding.interact", "binding.inventory",
        "binding.pause", "binding.confirm", "binding.cancel"};
    return keys[a];
}

void input_reset_to_defaults(void) {
    memset(g_bindings, 0, sizeof g_bindings);
    g_bindings[ACT_MOVE_UP] = AMZ_KEY_W;
    g_bindings[ACT_MOVE_DOWN] = AMZ_KEY_S;
    g_bindings[ACT_MOVE_LEFT] = AMZ_KEY_A;
    g_bindings[ACT_MOVE_RIGHT] = AMZ_KEY_D;
    g_bindings[ACT_INTERACT] = AMZ_KEY_E;
    g_bindings[ACT_INVENTORY] = AMZ_KEY_I;
    g_bindings[ACT_PAUSE] = AMZ_KEY_ESCAPE;
    g_bindings[ACT_CONFIRM] = AMZ_KEY_ENTER;
    g_bindings[ACT_CANCEL] = AMZ_KEY_ESCAPE;
}

void input_init(void) {
    input_reset_to_defaults();
    memset(&g_buffer, 0, sizeof g_buffer);
}

int input_get_binding(Action a) {
    return a >= 0 && a < ACT_COUNT ? g_bindings[a] : 0;
}

bool input_set_binding(Action a, int key) {
    if (a < 0 || a >= ACT_COUNT || key == AMZ_KEY_ESCAPE) {
        return false;
    }
    g_bindings[a] = key;
    return true;
}

bool input_missing_required(void) {
    for (int i = 0; i < ACT_COUNT; i++) {
        if (g_bindings[i] == 0) {
            return true;
        }
    }
    return false;
}

RemapStatus input_propose_binding(Action a, int key, Action *displaces_out) {
    if (a < 0 || a >= ACT_COUNT || key == AMZ_KEY_ESCAPE) {
        return REMAP_RESERVED;
    }
    for (int i = 0; i < ACT_COUNT; i++) {
        if (i != (int)a && g_bindings[i] == key) {
            if (displaces_out) {
                *displaces_out = (Action)i;
            }
            return REMAP_SWAP_REQUIRED;
        }
    }
    return REMAP_OK;
}

void input_apply_swap(Action a, int key, Action displaced) {
    if (a >= 0 && a < ACT_COUNT) {
        g_bindings[a] = key;
    }
    if (displaced >= 0 && displaced < ACT_COUNT && displaced != a) {
        g_bindings[displaced] = 0;
    }
}

void input_load_bindings(int profile_id) {
    input_reset_to_defaults();
    for (int i = 0; i < ACT_COUNT; i++) {
        char value[32];
        if (settings_get(profile_id, action_setting_key((Action)i), value, sizeof value)) {
            g_bindings[i] = atoi(value);
        }
    }
    if (input_missing_required()) {
        LOGW("missing required bindings; restoring defaults");
        input_reset_to_defaults();
        g_recovered = true;
    }
}

void input_save_bindings(int profile_id) {
    char value[32];
    for (int i = 0; i < ACT_COUNT; i++) {
        snprintf(value, sizeof value, "%d", g_bindings[i]);
        settings_set(profile_id, action_setting_key((Action)i), value);
    }
}

bool input_load_recovery_notice(void) {
    bool out = g_recovered;
    g_recovered = false;
    return out;
}

#ifndef AMAZEING_HEADLESS
static int movement_alias_key(Action a) {
    switch (a) {
    case ACT_MOVE_UP:
        return AMZ_KEY_UP;
    case ACT_MOVE_DOWN:
        return AMZ_KEY_DOWN;
    case ACT_MOVE_LEFT:
        return AMZ_KEY_LEFT;
    case ACT_MOVE_RIGHT:
        return AMZ_KEY_RIGHT;
    default:
        return 0;
    }
}

static bool alias_available(Action a, int key) {
    if (!key) {
        return false;
    }
    for (int i = 0; i < ACT_COUNT; i++) {
        if (i != (int)a && g_bindings[i] == key) {
            return false;
        }
    }
    return true;
}
#endif

void input_sample(void) {
    for (int i = 0; i < ACT_COUNT; i++) {
#ifndef AMAZEING_HEADLESS
        int alias = movement_alias_key((Action)i);
        bool use_alias = alias_available((Action)i, alias);
        g_buffer.held[i] = IsKeyDown(g_bindings[i]) || (use_alias && IsKeyDown(alias));
        if (IsKeyPressed(g_bindings[i]) || (use_alias && IsKeyPressed(alias))) {
            g_buffer.edge_count[i]++;
        }
#else
        g_buffer.held[i] = false;
#endif
    }
}

void input_consume(InputFrame *out) {
    if (!out) {
        return;
    }
    *out = g_buffer;
    memset(g_buffer.edge_count, 0, sizeof g_buffer.edge_count);
}

#ifdef AMAZEING_TESTS
void input_load_corrupt_for_tests(void) {
    g_bindings[ACT_INTERACT] = 0;
    if (input_missing_required()) {
        input_reset_to_defaults();
        g_recovered = true;
    }
}
#endif

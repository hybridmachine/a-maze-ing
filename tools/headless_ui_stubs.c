#include "ui.h"

#include "game.h"
#include "time_of_day.h"

#include <stdio.h>

static char g_last_dialog_key[128];

void ui_init(void) {
}

void ui_shutdown(void) {
}

void ui_tick(Game *g, const InputFrame *in) {
    (void)g;
    (void)in;
}

void ui_render_native(Game *g) {
    (void)g;
}

float ui_scale(void) {
    return 1.0f;
}

void ui_dialog_show_key(Game *g, const char *key) {
    snprintf(g_last_dialog_key, sizeof g_last_dialog_key, "%s", key);
    g->paused_for_dialog = true;
    g->state = GS_DIALOG;
}

void ui_dialog_play(Game *g, const DialogDef *def, Entity *entity) {
    (void)entity;
    snprintf(g_last_dialog_key, sizeof g_last_dialog_key, "%s",
             def && def->line_count > 0 ? def->lines[0] : "");
    g->paused_for_dialog = true;
    g->state = GS_DIALOG;
}

void ui_fade_then_advance_clock(Game *g, int target_minute) {
    clock_advance_to(&g->clock, target_minute);
}

bool ui_recovery_confirm(const char *message) {
    (void)message;
    return true;
}

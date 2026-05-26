#pragma once

#include "dialogs.h"
#include "input.h"

struct Entity;
struct Game;

void ui_init(void);
void ui_shutdown(void);
void ui_tick(struct Game *g, const InputFrame *in);
void ui_render_native(struct Game *g);
float ui_scale(void);
void ui_dialog_show_key(struct Game *g, const char *key);
void ui_dialog_play(struct Game *g, const DialogDef *def, struct Entity *entity);
void ui_fade_then_advance_clock(struct Game *g, int target_minute);
bool ui_recovery_confirm(const char *message);

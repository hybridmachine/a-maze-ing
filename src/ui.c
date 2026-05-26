#include "ui.h"

#include "game.h"
#include "localization.h"
#include "time_of_day.h"

#include <raylib.h>
#include <stdio.h>

static char g_dialog_key[128];

void ui_init(void) {
}

void ui_shutdown(void) {
}

float ui_scale(void) {
    return (float)GetScreenHeight() / 720.0f;
}

void ui_tick(Game *g, const InputFrame *in) {
    (void)g;
    (void)in;
}

void ui_dialog_show_key(Game *g, const char *key) {
    snprintf(g_dialog_key, sizeof g_dialog_key, "%s", key);
    g->paused_for_dialog = true;
    g->state = GS_DIALOG;
}

void ui_dialog_play(Game *g, const DialogDef *def, Entity *entity) {
    (void)entity;
    snprintf(g_dialog_key, sizeof g_dialog_key, "%s",
             def && def->line_count > 0 ? def->lines[0] : "dialog.missing");
    g->paused_for_dialog = true;
    g->state = GS_DIALOG;
}

void ui_fade_then_advance_clock(Game *g, int target_minute) {
    clock_advance_to(&g->clock, target_minute);
}

void ui_render_native(Game *g) {
    int x = 24;
    int y = 24;
    if (g->state == GS_TITLE) {
        DrawText("A Maze Ing", x, y, 36, RAYWHITE);
        DrawText(t("ui.title.start"), x, y + 52, 22, GOLD);
        DrawText(t("ui.title.quit"), x, y + 82, 22, LIGHTGRAY);
    } else if (g->state == GS_PROFILE) {
        DrawText("Profiles", x, y, 28, RAYWHITE);
        DrawText("Press Enter to continue", x, y + 40, 18, LIGHTGRAY);
    } else if (g->state == GS_MAZE_SELECT) {
        DrawText(t("maze.nature.name"), x, y, 28, RAYWHITE);
        DrawText("Press Enter to load Nature", x, y + 40, 18, LIGHTGRAY);
    } else if (g->state == GS_IN_MAZE) {
        char buf[64];
        snprintf(buf, sizeof buf, "%02d:%02d", g->clock.minute / 60, g->clock.minute % 60);
        DrawText(buf, x, y, 22, RAYWHITE);
        DrawText(g->player.outfit, GetScreenWidth() - 180, y, 18, RAYWHITE);
        DrawText(t("ui.hud.controls"), x, GetScreenHeight() - 36, 16, LIGHTGRAY);
    } else if (g->state == GS_DIALOG) {
        DrawRectangle(0, GetScreenHeight() - 140, GetScreenWidth(), 140, Fade(BLACK, 0.80f));
        DrawText(t(g_dialog_key), 32, GetScreenHeight() - 104, 22, RAYWHITE);
        DrawText("Esc", GetScreenWidth() - 72, GetScreenHeight() - 38, 18, LIGHTGRAY);
    }
}

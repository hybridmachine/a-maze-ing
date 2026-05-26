#include "render.h"

#include "game.h"
#include "ui.h"

#include <math.h>
#include <raylib.h>

static RenderTexture2D g_world_rt;
static bool g_ready;

void render_init(void) {
    g_world_rt = LoadRenderTexture(WORLD_RT_W, WORLD_RT_H);
    SetTextureFilter(g_world_rt.texture, TEXTURE_FILTER_POINT);
    g_ready = true;
}

void render_shutdown(void) {
    if (g_ready) {
        UnloadRenderTexture(g_world_rt);
        g_ready = false;
    }
}

void render_frame(Game *g, float alpha) {
    if (!g_ready) {
        render_init();
    }
    BeginTextureMode(g_world_rt);
    ClearBackground((Color){28, 32, 36, 255});
    render_world(g, alpha);
    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);
    int win_w = GetScreenWidth();
    int win_h = GetScreenHeight();
    int scale_i = (int)fmaxf(1.0f, floorf(fminf((float)win_w / WORLD_RT_W,
                                                (float)win_h / WORLD_RT_H)));
    float w = WORLD_RT_W * (float)scale_i;
    float h = WORLD_RT_H * (float)scale_i;
    Rectangle src = {0, 0, WORLD_RT_W, -WORLD_RT_H};
    Rectangle dst = {(win_w - w) * 0.5f, (win_h - h) * 0.5f, w, h};
    DrawTexturePro(g_world_rt.texture, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
    ui_render_native(g);
    EndDrawing();
}

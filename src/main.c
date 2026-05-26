#include "asset.h"
#include "audio.h"
#include "dialogs.h"
#include "game.h"
#include "items.h"
#include "localization.h"
#include "profile.h"
#include "render.h"
#include "save.h"
#include "ui.h"
#include "util_log.h"
#include "util_paths.h"

#include <raylib.h>
#include <stdio.h>

#define TICK_HZ 60
#define TICK_DT (1.0 / TICK_HZ)
#define MAX_CATCHUP_TICKS 5

static void load_slice_into_game(Game *g) {
    if (!g->world.maze.tiles &&
        world_load_maze(&g->world, "data/mazes/nature.maze", "assets/themes/nature")) {
        MazeSnapshot snap;
        if (g->active_profile_id > 0 &&
            save_load_maze_snapshot(g->active_profile_id, "nature", &snap)) {
            player_init(&g->player, (Vec2){snap.x, snap.y});
            g->player.facing = snap.facing;
            clock_init(&g->clock, snap.clock_min);
            g->clock.day_index = snap.day_index;
        } else {
            player_init(&g->player, (Vec2){(float)g->world.maze.start_x,
                                           (float)g->world.maze.start_y});
            clock_init(&g->clock, g->world.maze.time_start_min);
        }
        char outfit[48];
        if (g->active_profile_id > 0 && save_load_outfit_worn(g->active_profile_id, outfit)) {
            snprintf(g->player.outfit, sizeof g->player.outfit, "%s", outfit);
        }
        camera_init(&g->camera, g->player.pos);
    }
}

int main(void) {
    InitWindow(1280, 720, "A Maze Ing");
    InitAudioDevice();
    SetExitKey(0);
    util_log_init(NULL);

    char db[1024];
    if (!util_paths_save_db(db, sizeof db)) {
        LOGE("could not resolve save DB path");
        return 1;
    }
    SaveOpenResult save_result = save_open_checked(db);
    if (save_result == SAVE_FAIL) {
        CloseWindow();
        return 1;
    }
    if (save_result == SAVE_CORRUPT &&
        !ui_recovery_confirm("Save data looked corrupt. A backup was created.")) {
        CloseWindow();
        return 0;
    }
    if (save_result == SAVE_CORRUPT && !save_reset_to_blank()) {
        CloseWindow();
        return 1;
    }

    asset_init("assets");
    audio_init();
    ui_init();
    loc_load_language(loc_detect_os_lang());
    items_load("data/items.txt");
    dialogs_load("data/dialogs/nature.txt");
    input_init();

    Game *g = game_create();
    Profile profiles[4];
    int profile_count = profile_list(profiles, 4);
    g->active_profile_id = profile_count > 0 ? profiles[0].id : profile_create("Player");
    input_load_bindings(g->active_profile_id);

    double accum = 0.0;
    double last = GetTime();
    while (!WindowShouldClose() && !g->quit_requested) {
        if (g->state == GS_MAZE_SELECT && IsKeyPressed(KEY_ENTER)) {
            load_slice_into_game(g);
            game_set_state(g, GS_IN_MAZE);
        }
        double now = GetTime();
        accum += now - last;
        last = now;
        if (accum > MAX_CATCHUP_TICKS * TICK_DT) {
            accum = MAX_CATCHUP_TICKS * TICK_DT;
        }
        input_sample();
        while (accum >= TICK_DT) {
            InputFrame in;
            input_consume(&in);
            game_tick(g, &in, (float)TICK_DT);
            ui_tick(g, &in);
            accum -= TICK_DT;
        }
        game_render(g, (float)(accum / TICK_DT));
    }

    game_destroy(g);
    ui_shutdown();
    audio_shutdown();
    asset_shutdown();
    render_shutdown();
    save_close();
    util_log_close();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

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

static bool content_path(const char *relative_path, char *out, size_t out_size) {
    if (util_paths_content_path(relative_path, out, out_size)) {
        return true;
    }
    LOGE("could not resolve content path: %s", relative_path);
    return false;
}

static bool load_slice_into_game(Game *g) {
    if (g->world.maze.tiles) {
        return true;
    }

    char maze_path[1024];
    char theme_dir[1024];
    if (!content_path("data/mazes/nature.maze", maze_path, sizeof maze_path) ||
        !content_path("assets/themes/nature", theme_dir, sizeof theme_dir)) {
        return false;
    }

    if (world_load_maze(&g->world, maze_path, theme_dir)) {
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
        return true;
    }
    LOGE("could not load maze: %s", maze_path);
    return false;
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

    char assets_root[1024];
    char strings_dir[1024];
    char items_path[1024];
    char dialogs_path[1024];
    if (!content_path("assets", assets_root, sizeof assets_root)) {
        snprintf(assets_root, sizeof assets_root, "assets");
    }
    asset_init(assets_root);
    audio_init();
    ui_init();
    if (content_path("data/strings", strings_dir, sizeof strings_dir) &&
        !loc_load_language_from(strings_dir, loc_detect_os_lang())) {
        LOGE("could not load localization: %s", strings_dir);
    }
    if (content_path("data/items.txt", items_path, sizeof items_path) &&
        !items_load(items_path)) {
        LOGE("could not load items: %s", items_path);
    }
    if (content_path("data/dialogs/nature.txt", dialogs_path, sizeof dialogs_path) &&
        !dialogs_load(dialogs_path)) {
        LOGE("could not load dialogs: %s", dialogs_path);
    }
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
            if (load_slice_into_game(g)) {
                game_set_state(g, GS_IN_MAZE);
            }
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

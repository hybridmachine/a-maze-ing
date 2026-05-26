#include "dialogs.h"
#include "game.h"
#include "items.h"
#include "localization.h"
#include "profile.h"
#include "save.h"
#include "util_log.h"

#include <stdio.h>

int main(void) {
    remove("/tmp/amz_smoke.db");
    util_log_init(NULL);
    if (!save_open("/tmp/amz_smoke.db")) {
        return 1;
    }
    items_load("data/items.txt");
    dialogs_load("data/dialogs/nature.txt");
    loc_load_language_from("data/strings", "en");
    int pid = profile_create("smoke");
    Game *g = game_create();
    g->active_profile_id = pid;
    if (!world_load_maze(&g->world, "data/mazes/nature.maze", "assets/themes/nature")) {
        return 1;
    }
    player_init(&g->player,
                (Vec2){(float)g->world.maze.start_x, (float)g->world.maze.start_y});
    clock_init(&g->clock, g->world.maze.time_start_min);
    InputFrame in = {0};
    for (int i = 0; i < 1000; i++) {
        game_tick(g, &in, 1.0f / 60.0f);
    }
    game_destroy(g);
    save_close();
    util_log_close();
    return 0;
}

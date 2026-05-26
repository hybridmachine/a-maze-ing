#pragma once

#include "camera.h"
#include "input.h"
#include "interaction.h"
#include "player.h"
#include "save.h"
#include "time_of_day.h"
#include "world.h"

#include <stdbool.h>

typedef enum {
    GS_TITLE,
    GS_PROFILE,
    GS_MAZE_SELECT,
    GS_IN_MAZE,
    GS_PAUSE,
    GS_INVENTORY,
    GS_DIALOG,
    GS_COMPLETE
} GameState;

typedef struct Game {
    GameState state;
    int active_profile_id;
    bool quit_requested;
    bool auto_wear_first;
    CheckpointPayload pending_checkpoint;
    World world;
    Player player;
    Clock clock;
    GameCamera camera;
    bool paused_for_dialog;
    bool picker_active;
    InteractionTarget pending_picker_target;
    float dialog_chars_per_second;
} Game;

Game *game_create(void);
void game_destroy(Game *g);
void game_set_state(Game *g, GameState s);
GameState game_state(const Game *g);
void game_tick(Game *g, const InputFrame *in, float dt);
void game_render(Game *g, float alpha);

void game_dirty_inventory_add(Game *g, const char *item_id, int delta);
void game_dirty_profile_item_add(Game *g, const char *item_id);
void game_dirty_outfit_set(Game *g, const char *outfit_id);
void game_dirty_entity_override(Game *g, const char *stable_id, int taken, int active,
                                int last_dialog_day);
void game_dirty_entity_override_dialog_day(Game *g, const char *stable_id, int day);
void game_request_checkpoint(Game *g, CheckpointReason reason);
bool game_player_has_item(Game *g, const char *item_id);
int game_entity_last_dialog_day(Game *g, const char *stable_id);

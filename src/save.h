#pragma once

#include <stdbool.h>

typedef enum {
    CKPT_ITEM_PICKED,
    CKPT_AREA_TRANSITION,
    CKPT_DIALOG_DONE,
    CKPT_DAY_ROLLED,
    CKPT_PAUSE,
    CKPT_QUIT
} CheckpointReason;

typedef enum { SAVE_OK, SAVE_CORRUPT, SAVE_FAIL } SaveOpenResult;

typedef struct {
    float x, y;
    int facing;
    int clock_min;
    int day_index;
} MazeSnapshot;

typedef struct CheckpointPayload {
    int profile_id;
    char maze_id[32];
    bool pos_dirty;
    MazeSnapshot pos;
    bool prog_dirty;
    char prog_state[16];
    int inv_add_count;
    struct {
        char item_id[48];
        int delta;
    } inv_adds[16];
    int ovr_count;
    struct {
        char stable_id[48];
        int taken;
        int active;
        int last_dialog_day;
    } ovrs[32];
    int profile_item_count;
    char profile_items[8][48];
    bool outfit_dirty;
    char outfit_id[48];
} CheckpointPayload;

bool save_open(const char *path);
SaveOpenResult save_open_checked(const char *path);
void save_close(void);
bool save_integrity_check(void);
bool save_backup(const char *reason);
bool save_reset_to_blank(void);

bool save_set_clock(int profile_id, const char *maze_id, int clock_min, int day_index);
bool save_set_position(int profile_id, const char *maze_id, float x, float y, int facing);
bool save_set_progress(int profile_id, const char *maze_id, const char *state);
bool save_set_inventory_add(int profile_id, const char *maze_id, const char *item_id, int delta);
bool save_set_profile_item(int profile_id, const char *item_id);
bool save_set_outfit_worn(int profile_id, const char *outfit_id);
bool save_set_entity_override(int profile_id, const char *maze_id, const char *stable_id,
                              int taken, int active, int last_dialog_day);

bool save_load_maze_snapshot(int profile_id, const char *maze_id, MazeSnapshot *out);
int save_load_profile_items(int profile_id, char ids[][48], int max);
int save_load_maze_inventory(int profile_id, const char *maze_id, char ids[][48], int max);
bool save_load_maze_inventory_count(int profile_id, const char *maze_id, const char *item_id,
                                    int *out_count);
bool save_load_outfit_worn(int profile_id, char out[48]);
bool save_load_entity_override(int profile_id, const char *maze_id, const char *stable_id,
                               int *taken, int *active, int *last_dialog_day);
bool save_checkpoint(const CheckpointPayload *p, CheckpointReason reason);
bool save_reset_maze(int profile_id, const char *maze_id);
bool settings_set(int profile_id, const char *key, const char *value);
bool settings_get(int profile_id, const char *key, char *out, int out_size);

struct sqlite3;
struct sqlite3 *save_db_handle(void);

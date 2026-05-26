#pragma once

#include "util_math.h"

#include <stdbool.h>

typedef struct {
    char behavior[32];
    char stable_id[48];
    int tile_x, tile_y;
    char args[8][48];
    int arg_count;
    bool has_shape_override;
    Shape shape_override;
    bool has_collide_override;
    Shape collide_override;
    bool has_proximity_override;
    Shape proximity_override;
} MazeEntityLine;

typedef struct {
    int x0, y0, x1, y1;
    char clip[64];
} AudioZoneLine;

typedef struct MazeFile {
    char name_key[64];
    char theme[32];
    char maze_id[32];
    int width, height;
    int start_x, start_y;
    int time_start_min;
    char ambient[4][48];
    int ambient_count;
    char *tiles;
    MazeEntityLine *entities;
    int entity_count;
    AudioZoneLine *zones;
    int zone_count;
} MazeFile;

bool maze_data_load(const char *path, MazeFile *out);
void maze_data_free(MazeFile *m);

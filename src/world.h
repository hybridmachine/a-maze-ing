#pragma once

#include "entity.h"
#include "maze_data.h"

#include <stdbool.h>

#define WORLD_MAX_ENTITIES 1024

typedef struct World {
    MazeFile maze;
    char theme[32];
    bool walkable[128];
    char tile_sprite[128][96];
    Entity entities[WORLD_MAX_ENTITIES];
    int entity_count;
} World;

bool world_load_maze(World *w, const char *maze_path, const char *theme_dir);
void world_unload(World *w);
bool world_tile_walkable(const World *w, int tx, int ty);
const char *world_tile_sprite(const World *w, char tile);

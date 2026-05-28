#pragma once

struct Game;

#define WORLD_RT_W 480
#define WORLD_RT_H 270
#define TILE_W 32
#define TILE_H 32

void render_init(void);
void render_shutdown(void);
void render_frame(struct Game *g, float alpha);
void render_world(struct Game *g, float alpha);

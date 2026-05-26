#include "render.h"

#include "asset.h"
#include "game.h"
#include "sprite_manifest.h"

#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

static void iso_world_to_screen(float tx, float ty, float *sx, float *sy) {
    *sx = (tx - ty) * (float)(TILE_W / 2);
    *sy = (tx + ty) * (float)(TILE_H / 2);
}

static Color color_for_tile(char c) {
    switch (c) {
    case 'g':
    case 'G':
        return (Color){114, 154, 83, 255};
    case 'm':
        return (Color){116, 88, 58, 255};
    case 'w':
    case 'W':
        return (Color){72, 137, 170, 255};
    default:
        return (Color){152, 127, 83, 255};
    }
}

static void theme_asset_key(const Game *g, const char *sprite, char *out, size_t out_size) {
    snprintf(out, out_size, "themes/%s/%s", g->world.theme, sprite);
}

static void draw_iso_tile(float tx, float ty, Color c) {
    float sx, sy;
    iso_world_to_screen(tx, ty, &sx, &sy);
    sx += WORLD_RT_W * 0.5f;
    sy += WORLD_RT_H * 0.5f;
    Vector2 pts[4] = {{sx, sy - TILE_H * 0.5f},
                      {sx + TILE_W * 0.5f, sy},
                      {sx, sy + TILE_H * 0.5f},
                      {sx - TILE_W * 0.5f, sy}};
    DrawTriangle(pts[0], pts[1], pts[2], c);
    DrawTriangle(pts[0], pts[2], pts[3], c);
    DrawLineV(pts[0], pts[1], Fade(BLACK, 0.15f));
    DrawLineV(pts[1], pts[2], Fade(BLACK, 0.15f));
    DrawLineV(pts[2], pts[3], Fade(BLACK, 0.15f));
    DrawLineV(pts[3], pts[0], Fade(BLACK, 0.15f));
}

static void draw_tile(Game *g, int x, int y, char c) {
    float sx, sy;
    iso_world_to_screen((float)x - g->camera.pos.x, (float)y - g->camera.pos.y, &sx, &sy);
    sx += WORLD_RT_W * 0.5f;
    sy += WORLD_RT_H * 0.5f;

    const char *sprite = world_tile_sprite(&g->world, c);
    if (sprite) {
        char key[160];
        theme_asset_key(g, sprite, key, sizeof key);
        Texture2D *tex = asset_acquire_texture(key);
        if (tex && tex->id) {
            DrawTexture(*tex, (int)(sx - TILE_W * 0.5f), (int)(sy - TILE_H * 0.5f), WHITE);
            asset_release(key);
            return;
        }
        asset_release(key);
    }

    draw_iso_tile((float)x - g->camera.pos.x, (float)y - g->camera.pos.y, color_for_tile(c));
}

typedef struct {
    Entity *entity;
    bool is_player;
    float sort_key;
} WorldDrawItem;

static int compare_draw_item(const void *a, const void *b) {
    const WorldDrawItem *ia = a;
    const WorldDrawItem *ib = b;
    if (ia->sort_key < ib->sort_key) {
        return -1;
    }
    if (ia->sort_key > ib->sort_key) {
        return 1;
    }
    return 0;
}

static void draw_theme_sprite(Game *g, const SpriteDef *sd, Vec2 pos) {
    float sx, sy;
    iso_world_to_screen(pos.x - g->camera.pos.x, pos.y - g->camera.pos.y, &sx, &sy);
    sx += WORLD_RT_W * 0.5f;
    sy += WORLD_RT_H * 0.5f;

    char key[160];
    theme_asset_key(g, sd->name, key, sizeof key);
    Texture2D *tex = asset_acquire_texture(key);
    if (tex && tex->id) {
        DrawTexture(*tex, (int)(sx - sd->origin_x), (int)(sy - sd->origin_y), WHITE);
        asset_release(key);
        return;
    }
    asset_release(key);
    DrawCircle((int)sx, (int)(sy - 12), 6, SKYBLUE);
}

static void draw_player_sprite(Game *g) {
    float sx, sy;
    iso_world_to_screen(g->player.pos.x - g->camera.pos.x, g->player.pos.y - g->camera.pos.y,
                        &sx, &sy);
    sx += WORLD_RT_W * 0.5f;
    sy += WORLD_RT_H * 0.5f;

    char outfit_key[160];
    snprintf(outfit_key, sizeof outfit_key, "character/outfits/%s.png", g->player.outfit);
    Texture2D *outfit = asset_acquire_texture(outfit_key);
    if (outfit && outfit->id) {
        DrawTexture(*outfit, (int)(sx - 16), (int)(sy - 48), WHITE);
        asset_release(outfit_key);

        char face_key[160];
        snprintf(face_key, sizeof face_key, "character/faces/%s.png", g->player.face);
        Texture2D *face = asset_acquire_texture(face_key);
        if (face && face->id) {
            DrawTexture(*face, (int)(sx - 8), (int)(sy - 44), WHITE);
        }
        asset_release(face_key);
        return;
    }
    asset_release(outfit_key);
    DrawCircle((int)sx, (int)(sy - 14), 7, RAYWHITE);
}

void render_world(Game *g, float alpha) {
    (void)alpha;
    if (!g->world.maze.tiles) {
        DrawText("A Maze Ing", 24, 24, 22, RAYWHITE);
        DrawText("Press Start to reach profile flow", 24, 54, 10, LIGHTGRAY);
        return;
    }
    for (int y = 0; y < g->world.maze.height; y++) {
        for (int x = 0; x < g->world.maze.width; x++) {
            char c = g->world.maze.tiles[y * g->world.maze.width + x];
            draw_tile(g, x, y, c);
        }
    }

    WorldDrawItem items[WORLD_MAX_ENTITIES + 1];
    int item_count = 0;
    for (int i = 0; i < g->world.entity_count; i++) {
        Entity *e = &g->world.entities[i];
        if (!e->alive) {
            continue;
        }
        items[item_count++] =
            (WorldDrawItem){.entity = e, .is_player = false, .sort_key = e->pos.x + e->pos.y};
    }
    items[item_count++] =
        (WorldDrawItem){.is_player = true, .sort_key = g->player.pos.x + g->player.pos.y};
    qsort(items, (size_t)item_count, sizeof items[0], compare_draw_item);

    for (int i = 0; i < item_count; i++) {
        if (items[i].is_player) {
            draw_player_sprite(g);
            continue;
        }
        Entity *e = items[i].entity;
        const SpriteDef *sd = sprites_at(e->sprite_id);
        if (sd) {
            draw_theme_sprite(g, sd, e->pos);
        } else {
            float sx, sy;
            iso_world_to_screen(e->pos.x - g->camera.pos.x, e->pos.y - g->camera.pos.y, &sx,
                                &sy);
            DrawCircle((int)(sx + WORLD_RT_W * 0.5f), (int)(sy + WORLD_RT_H * 0.5f - 12), 6,
                       e->kind == ENT_NPC ? GOLD : SKYBLUE);
        }
    }
}

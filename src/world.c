#include "world.h"

#include "sprite_manifest.h"
#include "util_log.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSET_SHAPE_PX_PER_WORLD_X 32.0f
#define ASSET_SHAPE_PX_PER_WORLD_Y 32.0f

static char *trim_world(char *s) {
    while (*s && isspace((unsigned char)*s)) {
        s++;
    }
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) {
        *--end = '\0';
    }
    return s;
}

static Shape asset_shape_to_world(Shape s) {
    if (s.w <= 0.0f || s.h <= 0.0f) {
        return s;
    }
    s.ox /= ASSET_SHAPE_PX_PER_WORLD_X;
    s.w /= ASSET_SHAPE_PX_PER_WORLD_X;
    s.oy /= ASSET_SHAPE_PX_PER_WORLD_Y;
    s.h /= ASSET_SHAPE_PX_PER_WORLD_Y;
    return s;
}

static void load_tiles(World *w, const char *theme_dir) {
    for (int i = 0; i < 128; i++) {
        w->walkable[i] = true;
        w->tile_sprite[i][0] = '\0';
    }
    char path[512];
    snprintf(path, sizeof path, "%s/tiles.txt", theme_dir);
    FILE *f = fopen(path, "rb");
    if (!f) {
        LOGW("missing tile table: %s", path);
        return;
    }
    char line[512];
    while (fgets(line, sizeof line, f)) {
        char *s = trim_world(line);
        if (!*s || *s == '#') {
            continue;
        }
        unsigned char tile = 0;
        int walk = 1;
        char sprite[96] = {0};
        if (sscanf(s, "%c %d %95s", (char *)&tile, &walk, sprite) >= 2 && tile < 128) {
            w->walkable[(int)tile] = walk != 0;
            char *colon = strchr(sprite, ':');
            if (colon) {
                snprintf(w->tile_sprite[(int)tile], sizeof w->tile_sprite[0], "%s", colon + 1);
            }
        }
    }
    fclose(f);
}

static const char *arg_value(const MazeEntityLine *line, const char *prefix) {
    size_t n = strlen(prefix);
    for (int i = 0; i < line->arg_count; i++) {
        if (strncmp(line->args[i], prefix, n) == 0) {
            return line->args[i] + n;
        }
    }
    return NULL;
}

static void parse_gate(Entity *e, const char *value) {
    int a = -1, b = -1;
    if (value && sscanf(value, "%d-%d", &a, &b) == 2) {
        e->gate_hour_a = (int8_t)a;
        e->gate_hour_b = (int8_t)b;
    }
}

static void attach_entity(Entity *e, const MazeEntityLine *line) {
    memset(e, 0, sizeof *e);
    e->alive = true;
    e->sprite_id = -1;
    e->gate_hour_a = -1;
    e->gate_hour_b = -1;
    snprintf(e->stable_id, sizeof e->stable_id, "%s", line->stable_id);
    e->pos = (Vec2){(float)line->tile_x, (float)line->tile_y};
    e->cb = entity_registry_find(line->behavior);
    if (!e->cb) {
        e->cb = entity_registry_find("decor");
    }

    const char *sprite = arg_value(line, "sprite:");
    if (sprite) {
        const SpriteDef *sd = sprites_find(sprite);
        if (sd) {
            e->sprite_id = sd->id;
            e->collide = asset_shape_to_world(sd->collide);
            e->proximity = asset_shape_to_world(sd->proximity);
        }
    }
    if (line->has_collide_override) {
        e->collide = asset_shape_to_world(line->collide_override);
    }
    if (line->has_proximity_override) {
        e->proximity = asset_shape_to_world(line->proximity_override);
    }
    const char *hint = arg_value(line, "hint:");
    if (hint) {
        snprintf(e->hint_key, sizeof e->hint_key, "%s", hint);
    }
    const char *needs = arg_value(line, "needs:");
    if (needs) {
        snprintf(e->needs_item, sizeof e->needs_item, "%s", needs);
    }
    const char *use_tag = arg_value(line, "use_tag:");
    if (use_tag) {
        snprintf(e->use_tag, sizeof e->use_tag, "%s", use_tag);
    }
    parse_gate(e, arg_value(line, "gate:"));

    if (strcmp(line->behavior, "pickup") == 0) {
        e->kind = ENT_ITEM;
        const char *item = arg_value(line, "item:");
        if (item) {
            snprintf(e->item.item_id, sizeof e->item.item_id, "%s", item);
            const ItemDef *def = items_find(item);
            e->item.scope = def ? def->scope : SCOPE_MAZE;
        }
    } else if (strcmp(line->behavior, "npc") == 0) {
        e->kind = ENT_NPC;
        const char *dialog = arg_value(line, "dialog:");
        if (dialog) {
            snprintf(e->npc.dialog_id, sizeof e->npc.dialog_id, "%s", dialog);
        }
    } else if (strcmp(line->behavior, "hazard") == 0 || strcmp(line->behavior, "water") == 0 ||
               strcmp(line->behavior, "mud") == 0 || strcmp(line->behavior, "ice") == 0 ||
               strcmp(line->behavior, "thorns") == 0) {
        e->kind = ENT_HAZARD;
        const char *type = arg_value(line, "hazard:");
        if (!type) {
            type = line->behavior;
        }
        if (strcmp(type, "mud") == 0) {
            e->hazard.type = HAZ_MUD;
        } else if (strcmp(type, "ice") == 0) {
            e->hazard.type = HAZ_ICE;
        } else if (strcmp(type, "thorns") == 0) {
            e->hazard.type = HAZ_THORNS;
        } else {
            e->hazard.type = HAZ_WATER;
        }
    } else if (strcmp(line->behavior, "door") == 0 || strcmp(line->behavior, "sign") == 0 ||
               strcmp(line->behavior, "sundial") == 0 ||
               strcmp(line->behavior, "withered_plant") == 0) {
        e->kind = ENT_INTERACT;
        const char *sign = arg_value(line, "sign:");
        if (sign) {
            snprintf(e->hint_key, sizeof e->hint_key, "%s", sign);
        }
    } else {
        e->kind = ENT_DECOR;
    }
}

bool world_load_maze(World *w, const char *maze_path, const char *theme_dir) {
    world_unload(w);
    if (!maze_data_load(maze_path, &w->maze)) {
        return false;
    }
    snprintf(w->theme, sizeof w->theme, "%s", w->maze.theme);
    load_tiles(w, theme_dir);
    char manifest[512];
    snprintf(manifest, sizeof manifest, "%s/manifest.txt", theme_dir);
    sprites_load(manifest);
    for (int i = 0; i < w->maze.entity_count && i < WORLD_MAX_ENTITIES; i++) {
        attach_entity(&w->entities[w->entity_count], &w->maze.entities[i]);
        w->entities[w->entity_count].id = (uint32_t)(w->entity_count + 1);
        w->entity_count++;
    }
    return true;
}

void world_unload(World *w) {
    if (!w) {
        return;
    }
    maze_data_free(&w->maze);
    memset(w, 0, sizeof *w);
}

bool world_tile_walkable(const World *w, int tx, int ty) {
    if (!w || !w->maze.tiles || tx < 0 || ty < 0 || tx >= w->maze.width || ty >= w->maze.height) {
        return false;
    }
    unsigned char c = (unsigned char)w->maze.tiles[ty * w->maze.width + tx];
    return c < 128 ? w->walkable[c] : true;
}

const char *world_tile_sprite(const World *w, char tile) {
    unsigned char c = (unsigned char)tile;
    return c < 128 && w->tile_sprite[c][0] ? w->tile_sprite[c] : NULL;
}

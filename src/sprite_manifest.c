#include "sprite_manifest.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SpriteDef *g_sprites;
static int g_count;
static int g_cap;

static char *trim_sprite(char *s) {
    while (*s && isspace((unsigned char)*s)) {
        s++;
    }
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) {
        *--end = '\0';
    }
    return s;
}

static void push_sprite(SpriteDef *s) {
    if (!s->name[0]) {
        return;
    }
    if (s->proximity.w <= 0 && s->collide.w > 0) {
        s->proximity = s->collide;
    }
    if (g_count == g_cap) {
        int next = g_cap ? g_cap * 2 : 32;
        SpriteDef *arr = realloc(g_sprites, (size_t)next * sizeof *arr);
        if (!arr) {
            return;
        }
        g_sprites = arr;
        g_cap = next;
    }
    s->id = g_count;
    g_sprites[g_count++] = *s;
}

static ShapeKind parse_shape_kind(const char *s) {
    return strcmp(s, "capsule") == 0 ? SHAPE_CAPSULE : SHAPE_RECT;
}

static bool parse_shape(const char *value, Shape *out) {
    char kind[32];
    float ox, oy, w, h;
    if (sscanf(value, "%31s %f %f %f %f", kind, &ox, &oy, &w, &h) != 5) {
        return false;
    }
    *out = (Shape){parse_shape_kind(kind), ox, oy, w, h};
    return true;
}

static SpriteLayer parse_layer(const char *value) {
    if (strcmp(value, "decor") == 0) {
        return LAYER_DECOR;
    }
    if (strcmp(value, "character") == 0) {
        return LAYER_CHARACTER;
    }
    if (strcmp(value, "hud") == 0) {
        return LAYER_HUD;
    }
    return LAYER_OBJECT;
}

bool sprites_load(const char *path) {
    sprites_shutdown();
    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    SpriteDef cur = {.shadow_h = 1.0f, .layer = LAYER_OBJECT};
    char line[512];
    while (fgets(line, sizeof line, f)) {
        char *s = trim_sprite(line);
        if (!*s || *s == '#') {
            continue;
        }
        char *colon = strchr(s, ':');
        if (!colon) {
            continue;
        }
        *colon = '\0';
        char *key = trim_sprite(s);
        char *value = trim_sprite(colon + 1);
        if (strcmp(key, "sprite") == 0) {
            push_sprite(&cur);
            cur = (SpriteDef){.shadow_h = 1.0f, .layer = LAYER_OBJECT};
            snprintf(cur.name, sizeof cur.name, "%s", value);
        } else if (strcmp(key, "origin") == 0) {
            sscanf(value, "%f, %f", &cur.origin_x, &cur.origin_y);
        } else if (strcmp(key, "foot") == 0) {
            sscanf(value, "%f, %f", &cur.foot_x, &cur.foot_y);
        } else if (strcmp(key, "collide") == 0) {
            parse_shape(value, &cur.collide);
        } else if (strcmp(key, "proximity") == 0) {
            parse_shape(value, &cur.proximity);
        } else if (strcmp(key, "shadow_h") == 0) {
            cur.shadow_h = strtof(value, NULL);
        } else if (strcmp(key, "layer") == 0) {
            cur.layer = parse_layer(value);
        } else if (strcmp(key, "interact") == 0) {
            sscanf(value, "%f, %f", &cur.interact_x, &cur.interact_y);
        }
    }
    push_sprite(&cur);
    fclose(f);
    return true;
}

const SpriteDef *sprites_find(const char *name) {
    for (int i = 0; i < g_count; i++) {
        if (strcmp(g_sprites[i].name, name) == 0) {
            return &g_sprites[i];
        }
    }
    return NULL;
}

const SpriteDef *sprites_at(int id) {
    return id >= 0 && id < g_count ? &g_sprites[id] : NULL;
}

void sprites_shutdown(void) {
    free(g_sprites);
    g_sprites = NULL;
    g_count = 0;
    g_cap = 0;
}

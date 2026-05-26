#include "asset.h"

#include <stdio.h>
#include <string.h>

#define ASSET_MAX 128

typedef struct {
    char key[128];
    AssetKind kind;
    int refs;
    bool loaded;
    union {
        Texture2D texture;
        Sound sound;
        Music music;
        Font font;
    };
} AssetEntry;

static AssetEntry g_entries[ASSET_MAX];
static char g_root[256] = "assets";

void asset_init(const char *assets_root) {
    if (assets_root) {
        snprintf(g_root, sizeof g_root, "%s", assets_root);
    }
}

static AssetEntry *find_entry(const char *key, AssetKind kind) {
    for (int i = 0; i < ASSET_MAX; i++) {
        if (g_entries[i].loaded && g_entries[i].kind == kind &&
            strcmp(g_entries[i].key, key) == 0) {
            return &g_entries[i];
        }
    }
    return NULL;
}

static AssetEntry *new_entry(const char *key, AssetKind kind) {
    for (int i = 0; i < ASSET_MAX; i++) {
        if (!g_entries[i].loaded) {
            memset(&g_entries[i], 0, sizeof g_entries[i]);
            snprintf(g_entries[i].key, sizeof g_entries[i].key, "%s", key);
            g_entries[i].kind = kind;
            g_entries[i].refs = 1;
            g_entries[i].loaded = true;
            return &g_entries[i];
        }
    }
    return NULL;
}

Texture2D *asset_acquire_texture(const char *key) {
    AssetEntry *e = find_entry(key, ASSET_TEXTURE);
    if (e) {
        e->refs++;
        return &e->texture;
    }
    e = new_entry(key, ASSET_TEXTURE);
    if (!e) {
        return NULL;
    }
    char path[512];
    snprintf(path, sizeof path, "%s/%s", g_root, key);
    e->texture = LoadTexture(path);
    return &e->texture;
}

Sound *asset_acquire_sound(const char *key) {
    AssetEntry *e = find_entry(key, ASSET_SOUND);
    if (e) {
        e->refs++;
        return &e->sound;
    }
    e = new_entry(key, ASSET_SOUND);
    char path[512];
    snprintf(path, sizeof path, "%s/%s", g_root, key);
    e->sound = LoadSound(path);
    return &e->sound;
}

Music *asset_acquire_music(const char *key) {
    AssetEntry *e = find_entry(key, ASSET_MUSIC);
    if (e) {
        e->refs++;
        return &e->music;
    }
    e = new_entry(key, ASSET_MUSIC);
    char path[512];
    snprintf(path, sizeof path, "%s/%s", g_root, key);
    e->music = LoadMusicStream(path);
    return &e->music;
}

Font *asset_acquire_font(const char *key, int size, const int *codepoints, int n) {
    (void)size;
    (void)codepoints;
    (void)n;
    AssetEntry *e = find_entry(key, ASSET_FONT);
    if (e) {
        e->refs++;
        return &e->font;
    }
    e = new_entry(key, ASSET_FONT);
    char path[512];
    snprintf(path, sizeof path, "%s/%s", g_root, key);
    e->font = LoadFont(path);
    if (e->font.texture.id == 0) {
        e->font = GetFontDefault();
    }
    return &e->font;
}

void asset_release(const char *key) {
    for (int i = 0; i < ASSET_MAX; i++) {
        if (g_entries[i].loaded && strcmp(g_entries[i].key, key) == 0 && g_entries[i].refs > 0) {
            g_entries[i].refs--;
        }
    }
}

void asset_gc(void) {
    for (int i = 0; i < ASSET_MAX; i++) {
        AssetEntry *e = &g_entries[i];
        if (!e->loaded || e->refs > 0) {
            continue;
        }
        if (e->kind == ASSET_TEXTURE && e->texture.id) {
            UnloadTexture(e->texture);
        } else if (e->kind == ASSET_SOUND && e->sound.frameCount) {
            UnloadSound(e->sound);
        } else if (e->kind == ASSET_MUSIC && e->music.ctxData) {
            UnloadMusicStream(e->music);
        } else if (e->kind == ASSET_FONT && e->font.texture.id != GetFontDefault().texture.id) {
            UnloadFont(e->font);
        }
        memset(e, 0, sizeof *e);
    }
}

void asset_shutdown(void) {
    for (int i = 0; i < ASSET_MAX; i++) {
        if (g_entries[i].loaded) {
            g_entries[i].refs = 0;
        }
    }
    asset_gc();
}

#pragma once

#include <raylib.h>

typedef enum { ASSET_TEXTURE, ASSET_SOUND, ASSET_MUSIC, ASSET_FONT } AssetKind;

void asset_init(const char *assets_root);
void asset_shutdown(void);
Texture2D *asset_acquire_texture(const char *key);
Sound *asset_acquire_sound(const char *key);
Music *asset_acquire_music(const char *key);
Font *asset_acquire_font(const char *key, int size, const int *codepoints, int n);
void asset_release(const char *key);
void asset_gc(void);

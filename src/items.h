#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum { SCOPE_MAZE, SCOPE_PROFILE } ItemScope;
typedef enum { KIND_OUTFIT, KIND_TOOL, KIND_CONSUMABLE, KIND_ACCESS } ItemKind;

typedef struct ItemDef {
    char id[48];
    char name_key[64];
    char desc_key[64];
    char icon_path[96];
    ItemScope scope;
    ItemKind kind;
    char use_tag[32];
} ItemDef;

bool items_load(const char *path);
const ItemDef *items_find(const char *id);
size_t items_count(void);
const ItemDef *items_at(size_t i);
void items_shutdown(void);

#include "items.h"

#include "util_log.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ItemDef *g_items;
static size_t g_count;
static size_t g_cap;

static char *trim_item(char *s) {
    while (*s && isspace((unsigned char)*s)) {
        s++;
    }
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) {
        *--end = '\0';
    }
    return s;
}

static bool parse_scope(const char *s, ItemScope *out) {
    if (strcmp(s, "profile") == 0) {
        *out = SCOPE_PROFILE;
        return true;
    }
    if (strcmp(s, "maze") == 0) {
        *out = SCOPE_MAZE;
        return true;
    }
    return false;
}

static bool parse_kind(const char *s, ItemKind *out) {
    if (strcmp(s, "outfit") == 0) {
        *out = KIND_OUTFIT;
    } else if (strcmp(s, "tool") == 0) {
        *out = KIND_TOOL;
    } else if (strcmp(s, "consumable") == 0) {
        *out = KIND_CONSUMABLE;
    } else if (strcmp(s, "access") == 0) {
        *out = KIND_ACCESS;
    } else {
        return false;
    }
    return true;
}

static bool add_item(const ItemDef *d) {
    if (!d->id[0] || !d->name_key[0] || !d->icon_path[0]) {
        return false;
    }
    if (g_count == g_cap) {
        size_t next = g_cap ? g_cap * 2 : 16;
        ItemDef *items = realloc(g_items, next * sizeof *items);
        if (!items) {
            return false;
        }
        g_items = items;
        g_cap = next;
    }
    g_items[g_count++] = *d;
    return true;
}

bool items_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    ItemDef cur = {0};
    cur.scope = SCOPE_MAZE;
    cur.kind = KIND_CONSUMABLE;
    char line[512];
    while (fgets(line, sizeof line, f)) {
        char *s = trim_item(line);
        if (!*s || *s == '#') {
            continue;
        }
        char *colon = strchr(s, ':');
        if (!colon) {
            continue;
        }
        *colon = '\0';
        char *key = trim_item(s);
        char *value = trim_item(colon + 1);
        if (strcmp(key, "item") == 0) {
            if (cur.id[0]) {
                add_item(&cur);
                cur = (ItemDef){0};
                cur.scope = SCOPE_MAZE;
                cur.kind = KIND_CONSUMABLE;
            }
            snprintf(cur.id, sizeof cur.id, "%s", value);
        } else if (strcmp(key, "name") == 0) {
            snprintf(cur.name_key, sizeof cur.name_key, "%s", value);
        } else if (strcmp(key, "desc") == 0) {
            snprintf(cur.desc_key, sizeof cur.desc_key, "%s", value);
        } else if (strcmp(key, "scope") == 0) {
            if (!parse_scope(value, &cur.scope)) {
                LOGW("unknown item scope: %s", value);
            }
        } else if (strcmp(key, "kind") == 0) {
            if (!parse_kind(value, &cur.kind)) {
                LOGW("unknown item kind: %s", value);
            }
        } else if (strcmp(key, "icon") == 0) {
            snprintf(cur.icon_path, sizeof cur.icon_path, "%s", value);
        } else if (strcmp(key, "use_tag") == 0) {
            snprintf(cur.use_tag, sizeof cur.use_tag, "%s", value);
        }
    }
    if (cur.id[0]) {
        add_item(&cur);
    }
    fclose(f);
    return true;
}

const ItemDef *items_find(const char *id) {
    for (size_t i = 0; i < g_count; i++) {
        if (strcmp(g_items[i].id, id) == 0) {
            return &g_items[i];
        }
    }
    return NULL;
}

size_t items_count(void) {
    return g_count;
}

const ItemDef *items_at(size_t i) {
    return i < g_count ? &g_items[i] : NULL;
}

void items_shutdown(void) {
    free(g_items);
    g_items = NULL;
    g_count = 0;
    g_cap = 0;
}

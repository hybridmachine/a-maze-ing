#include "entity.h"

#include <stdio.h>
#include <string.h>

#define REG_MAX 64

typedef struct {
    char name[32];
    const EntityCallbacks *cb;
} RegEntry;

static RegEntry g_entries[REG_MAX];
static int g_count;

static void noop_loaded(struct Game *g, Entity *e) {
    (void)g;
    (void)e;
}

static const EntityCallbacks DECOR_CB = {.on_loaded = noop_loaded};

void entity_registry_init(void) {
    memset(g_entries, 0, sizeof g_entries);
    g_count = 0;
    entity_registry_register("decor", &DECOR_CB);
}

void entity_registry_register(const char *behavior, const EntityCallbacks *cb) {
    for (int i = 0; i < g_count; i++) {
        if (strcmp(g_entries[i].name, behavior) == 0) {
            g_entries[i].cb = cb;
            return;
        }
    }
    if (g_count < REG_MAX) {
        snprintf(g_entries[g_count].name, sizeof g_entries[g_count].name, "%s", behavior);
        g_entries[g_count].cb = cb;
        g_count++;
    }
}

const EntityCallbacks *entity_registry_find(const char *behavior) {
    for (int i = 0; i < g_count; i++) {
        if (strcmp(g_entries[i].name, behavior) == 0) {
            return g_entries[i].cb;
        }
    }
    return NULL;
}

void entity_register_builtin_stubs(void) {
    entity_registry_register("pickup", &DECOR_CB);
    entity_registry_register("door", &DECOR_CB);
    entity_registry_register("sign", &DECOR_CB);
    entity_registry_register("sundial", &DECOR_CB);
    entity_registry_register("npc", &DECOR_CB);
    entity_registry_register("withered_plant", &DECOR_CB);
    entity_registry_register("hazard", &DECOR_CB);
    entity_registry_register("water", &DECOR_CB);
    entity_registry_register("mud", &DECOR_CB);
    entity_registry_register("ice", &DECOR_CB);
    entity_registry_register("thorns", &DECOR_CB);
}

#pragma once

#include "items.h"
#include "util_math.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum { ENT_ITEM, ENT_HAZARD, ENT_INTERACT, ENT_NPC, ENT_DECOR } EntityKind;
typedef enum { HAZ_WATER, HAZ_MUD, HAZ_ICE, HAZ_THORNS } HazardType;

struct Game;
struct Entity;

typedef struct EntityCallbacks {
    void (*on_loaded)(struct Game *, struct Entity *);
    void (*on_player_enter)(struct Game *, struct Entity *);
    void (*on_player_exit)(struct Game *, struct Entity *);
    void (*on_interact)(struct Game *, struct Entity *, const char *item_id);
    void (*on_step)(struct Game *, struct Entity *, float dt);
    void (*on_time_changed)(struct Game *, struct Entity *, int hour);
} EntityCallbacks;

typedef struct Entity {
    EntityKind kind;
    uint32_t id;
    char stable_id[48];
    Vec2 pos;
    Shape collide;
    Shape proximity;
    int sprite_id;
    int shadow_id;
    uint32_t flags;
    int8_t gate_hour_a, gate_hour_b;
    const EntityCallbacks *cb;
    bool alive;
    char needs_item[48];
    char hint_key[64];
    char use_tag[32];
    union {
        struct {
            char item_id[48];
            ItemScope scope;
            bool taken;
        } item;
        struct {
            int type;
            int param;
        } hazard;
        struct {
            int target;
            bool active;
        } interact;
        struct {
            char dialog_id[64];
            int state;
        } npc;
    };
} Entity;

void entity_registry_init(void);
void entity_registry_register(const char *behavior, const EntityCallbacks *cb);
const EntityCallbacks *entity_registry_find(const char *behavior);
void entity_register_builtin_stubs(void);

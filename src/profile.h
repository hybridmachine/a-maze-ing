#pragma once

#include <stdbool.h>

#define PROFILE_NAME_MAX 32
#define PROFILE_MAX_COUNT 4

typedef struct {
    int id;
    char name[PROFILE_NAME_MAX];
    long last_played;
} Profile;

int profile_list(Profile *out, int max);
int profile_create(const char *name);
bool profile_delete(int id);
bool profile_touch(int id);

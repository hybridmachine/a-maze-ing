#pragma once

#include <stdbool.h>

typedef enum { REPEAT_ONCE, REPEAT_ONCE_PER_DAY, REPEAT_ALWAYS } DialogRepeat;

typedef struct {
    char id[64];
    char speaker_key[64];
    char lines[8][96];
    int line_count;
    DialogRepeat repeat;
    char post_state[96];
} DialogDef;

bool dialogs_load(const char *path);
const DialogDef *dialogs_find(const char *id);
void dialogs_shutdown(void);

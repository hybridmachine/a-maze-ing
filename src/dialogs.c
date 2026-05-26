#include "dialogs.h"

#include "util_log.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static DialogDef *g_dialogs;
static int g_count;
static int g_cap;

static char *trim_dialog(char *s) {
    while (*s && isspace((unsigned char)*s)) {
        s++;
    }
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) {
        *--end = '\0';
    }
    return s;
}

static void push_dialog(const DialogDef *d) {
    if (!d->id[0]) {
        return;
    }
    if (g_count == g_cap) {
        int next = g_cap ? g_cap * 2 : 16;
        DialogDef *arr = realloc(g_dialogs, (size_t)next * sizeof *arr);
        if (!arr) {
            return;
        }
        g_dialogs = arr;
        g_cap = next;
    }
    g_dialogs[g_count++] = *d;
}

static DialogRepeat parse_repeat(const char *s) {
    if (strcmp(s, "once") == 0) {
        return REPEAT_ONCE;
    }
    if (strcmp(s, "once_per_day") == 0) {
        return REPEAT_ONCE_PER_DAY;
    }
    if (strcmp(s, "always") != 0) {
        LOGW("unknown repeat policy: %s", s);
    }
    return REPEAT_ALWAYS;
}

bool dialogs_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    DialogDef cur = {.repeat = REPEAT_ALWAYS};
    bool in_lines = false;
    char line[512];
    while (fgets(line, sizeof line, f)) {
        bool indented = line[0] == ' ' || line[0] == '\t';
        char *s = trim_dialog(line);
        if (!*s || *s == '#') {
            in_lines = false;
            continue;
        }
        if (in_lines && indented) {
            if (cur.line_count < 8) {
                snprintf(cur.lines[cur.line_count++], sizeof cur.lines[0], "%s", s);
            }
            continue;
        }
        in_lines = false;
        char *colon = strchr(s, ':');
        if (!colon) {
            continue;
        }
        *colon = '\0';
        char *key = trim_dialog(s);
        char *value = trim_dialog(colon + 1);
        if (strcmp(key, "dialog") == 0) {
            push_dialog(&cur);
            cur = (DialogDef){.repeat = REPEAT_ALWAYS};
            snprintf(cur.id, sizeof cur.id, "%s", value);
        } else if (strcmp(key, "speaker") == 0) {
            snprintf(cur.speaker_key, sizeof cur.speaker_key, "%s", value);
        } else if (strcmp(key, "lines") == 0) {
            in_lines = true;
        } else if (strcmp(key, "repeat") == 0) {
            cur.repeat = parse_repeat(value);
        } else if (strcmp(key, "post_state") == 0) {
            snprintf(cur.post_state, sizeof cur.post_state, "%s", value);
        }
    }
    push_dialog(&cur);
    fclose(f);
    return true;
}

const DialogDef *dialogs_find(const char *id) {
    for (int i = 0; i < g_count; i++) {
        if (strcmp(g_dialogs[i].id, id) == 0) {
            return &g_dialogs[i];
        }
    }
    return NULL;
}

void dialogs_shutdown(void) {
    free(g_dialogs);
    g_dialogs = NULL;
    g_count = 0;
    g_cap = 0;
}

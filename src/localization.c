#include "localization.h"

#include "util_log.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static LangTable g_en;
static LangTable g_active;

static char *xstrdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *out = malloc(n);
    if (out) {
        memcpy(out, s, n);
    }
    return out;
}

static char *trim(char *s) {
    while (*s && isspace((unsigned char)*s)) {
        s++;
    }
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) {
        *--end = '\0';
    }
    return s;
}

static bool table_push(LangTable *t, const char *key, const char *value) {
    if (t->count == t->cap) {
        size_t next = t->cap ? t->cap * 2 : 16;
        LangEntry *entries = realloc(t->entries, next * sizeof *entries);
        if (!entries) {
            return false;
        }
        t->entries = entries;
        t->cap = next;
    }
    t->entries[t->count].key = xstrdup(key);
    t->entries[t->count].value = xstrdup(value);
    if (!t->entries[t->count].key || !t->entries[t->count].value) {
        return false;
    }
    t->count++;
    return true;
}

bool loc_parse_file(const char *path, LangTable *out) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    char line[1024];
    while (fgets(line, sizeof line, f)) {
        char *s = trim(line);
        if (!*s || *s == '#') {
            continue;
        }
        char *eq = strchr(s, '=');
        if (!eq) {
            LOGW("malformed lang line skipped: %s", s);
            continue;
        }
        *eq = '\0';
        char *key = trim(s);
        char *value = trim(eq + 1);
        if (!*key || !*value) {
            continue;
        }
        if (!table_push(out, key, value)) {
            fclose(f);
            return false;
        }
    }
    fclose(f);
    return true;
}

void loc_table_free(LangTable *lt) {
    if (!lt) {
        return;
    }
    for (size_t i = 0; i < lt->count; i++) {
        free(lt->entries[i].key);
        free(lt->entries[i].value);
    }
    free(lt->entries);
    *lt = (LangTable){0};
}

static const char *table_lookup(const LangTable *lt, const char *key) {
    for (size_t i = 0; i < lt->count; i++) {
        if (strcmp(lt->entries[i].key, key) == 0) {
            return lt->entries[i].value;
        }
    }
    return NULL;
}

bool loc_load_language_from(const char *strings_dir, const char *lang_code) {
    loc_shutdown();
    char path[512];
    snprintf(path, sizeof path, "%s/en.lang", strings_dir);
    if (!loc_parse_file(path, &g_en)) {
        return false;
    }
    if (!lang_code || strcmp(lang_code, "en") == 0) {
        return true;
    }
    snprintf(path, sizeof path, "%s/%s.lang", strings_dir, lang_code);
    if (!loc_parse_file(path, &g_active)) {
        LOGW("language not found, using English fallback: %s", lang_code);
    }
    return true;
}

bool loc_load_language(const char *lang_code) {
    return loc_load_language_from("data/strings", lang_code);
}

void loc_shutdown(void) {
    loc_table_free(&g_en);
    loc_table_free(&g_active);
}

const char *t(const char *key) {
    const char *found = table_lookup(&g_active, key);
    if (found) {
        return found;
    }
    found = table_lookup(&g_en, key);
    return found ? found : key;
}

const char *loc_detect_os_lang(void) {
#ifdef _WIN32
    return "en";
#else
    const char *env = getenv("LC_ALL");
    if (!env || !*env) {
        env = getenv("LANG");
    }
    static char out[3] = "en";
    if (env && strlen(env) >= 2 && isalpha((unsigned char)env[0]) &&
        isalpha((unsigned char)env[1])) {
        out[0] = (char)tolower((unsigned char)env[0]);
        out[1] = (char)tolower((unsigned char)env[1]);
        out[2] = '\0';
    }
    return out;
#endif
}

#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct LangEntry {
    char *key;
    char *value;
} LangEntry;

typedef struct LangTable {
    LangEntry *entries;
    size_t count;
    size_t cap;
} LangTable;

bool loc_parse_file(const char *path, LangTable *out);
void loc_table_free(LangTable *t);
bool loc_load_language(const char *lang_code);
bool loc_load_language_from(const char *strings_dir, const char *lang_code);
void loc_shutdown(void);
const char *t(const char *key);
const char *loc_detect_os_lang(void);

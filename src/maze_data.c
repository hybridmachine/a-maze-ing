#include "maze_data.h"

#include "util_log.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { SEC_HEADER, SEC_TILES, SEC_ENTITIES, SEC_ZONES } Section;

static char *trim_maze(char *s) {
    while (*s && isspace((unsigned char)*s)) {
        s++;
    }
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) {
        *--end = '\0';
    }
    return s;
}

static void derive_id(const char *path, char out[32]) {
    const char *slash = strrchr(path, '/');
    const char *base = slash ? slash + 1 : path;
    snprintf(out, 32, "%s", base);
    char *dot = strrchr(out, '.');
    if (dot) {
        *dot = '\0';
    }
}

static bool parse_shape_override(const char *arg, Shape *out) {
    const char *colon = strchr(arg, ':');
    if (!colon) {
        return false;
    }
    const char *kind = colon + 1;
    const char *nums = strchr(kind, ':');
    if (!nums) {
        return false;
    }
    char kind_buf[16];
    size_t n = (size_t)(nums - kind);
    if (n >= sizeof kind_buf) {
        return false;
    }
    memcpy(kind_buf, kind, n);
    kind_buf[n] = '\0';
    float ox, oy, w, h;
    if (sscanf(nums + 1, "%f,%f,%f,%f", &ox, &oy, &w, &h) != 4) {
        return false;
    }
    *out = (Shape){strcmp(kind_buf, "capsule") == 0 ? SHAPE_CAPSULE : SHAPE_RECT, ox, oy, w, h};
    return true;
}

static bool push_entity(MazeFile *m, const MazeEntityLine *e) {
    MazeEntityLine *arr =
        realloc(m->entities, (size_t)(m->entity_count + 1) * sizeof *m->entities);
    if (!arr) {
        return false;
    }
    m->entities = arr;
    m->entities[m->entity_count++] = *e;
    return true;
}

static bool push_zone(MazeFile *m, const AudioZoneLine *z) {
    AudioZoneLine *arr = realloc(m->zones, (size_t)(m->zone_count + 1) * sizeof *m->zones);
    if (!arr) {
        return false;
    }
    m->zones = arr;
    m->zones[m->zone_count++] = *z;
    return true;
}

static void parse_header_line(MazeFile *m, char *s) {
    char *colon = strchr(s, ':');
    if (!colon) {
        return;
    }
    *colon = '\0';
    char *key = trim_maze(s);
    char *value = trim_maze(colon + 1);
    if (strcmp(key, "name") == 0) {
        snprintf(m->name_key, sizeof m->name_key, "%s", value);
    } else if (strcmp(key, "theme") == 0) {
        snprintf(m->theme, sizeof m->theme, "%s", value);
    } else if (strcmp(key, "size") == 0) {
        sscanf(value, "%dx%d", &m->width, &m->height);
    } else if (strcmp(key, "start") == 0) {
        sscanf(value, "%d,%d", &m->start_x, &m->start_y);
    } else if (strcmp(key, "time_start") == 0) {
        int h = 0, min = 0;
        sscanf(value, "%d:%d", &h, &min);
        m->time_start_min = h * 60 + min;
    } else if (strcmp(key, "ambient") == 0) {
        char *tok = strtok(value, ",");
        while (tok && m->ambient_count < 4) {
            snprintf(m->ambient[m->ambient_count++], sizeof m->ambient[0], "%s", trim_maze(tok));
            tok = strtok(NULL, ",");
        }
    } else {
        LOGW("unknown maze header key: %s", key);
    }
}

static bool parse_entity_line(MazeFile *m, char *s) {
    MazeEntityLine e = {0};
    char *tok = strtok(s, " \t");
    if (!tok) {
        return true;
    }
    char *hash = strchr(tok, '#');
    if (!hash) {
        return false;
    }
    *hash = '\0';
    snprintf(e.behavior, sizeof e.behavior, "%s", tok);
    snprintf(e.stable_id, sizeof e.stable_id, "%s", hash + 1);
    tok = strtok(NULL, " \t");
    if (!tok) {
        return false;
    }
    e.tile_x = atoi(tok);
    tok = strtok(NULL, " \t");
    if (!tok) {
        return false;
    }
    e.tile_y = atoi(tok);
    while ((tok = strtok(NULL, " \t")) != NULL && e.arg_count < 8) {
        if (strncmp(tok, "collide:", 8) == 0) {
            e.has_collide_override = parse_shape_override(tok, &e.collide_override);
            e.has_shape_override = e.has_collide_override;
            if (e.has_collide_override) {
                e.shape_override = e.collide_override;
            }
        } else if (strncmp(tok, "proximity:", 10) == 0) {
            e.has_proximity_override = parse_shape_override(tok, &e.proximity_override);
            e.has_shape_override = e.has_proximity_override;
            if (e.has_proximity_override) {
                e.shape_override = e.proximity_override;
            }
        }
        snprintf(e.args[e.arg_count++], sizeof e.args[0], "%s", tok);
    }
    return push_entity(m, &e);
}

static bool parse_zone_line(MazeFile *m, char *s) {
    char name[32], clip_token[96];
    AudioZoneLine z = {0};
    if (sscanf(s, "%31s %d %d %d %d %95s", name, &z.x0, &z.y0, &z.x1, &z.y1, clip_token) < 6) {
        return false;
    }
    char *colon = strchr(clip_token, ':');
    snprintf(z.clip, sizeof z.clip, "%s", colon ? colon + 1 : clip_token);
    return push_zone(m, &z);
}

bool maze_data_load(const char *path, MazeFile *out) {
    memset(out, 0, sizeof *out);
    derive_id(path, out->maze_id);
    FILE *f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    Section section = SEC_HEADER;
    int tile_row = 0;
    char line[1024];
    while (fgets(line, sizeof line, f)) {
        char *s = trim_maze(line);
        if (!*s || *s == '#') {
            continue;
        }
        if (strcmp(s, "tiles:") == 0) {
            section = SEC_TILES;
            if (out->width <= 0 || out->height <= 0) {
                fclose(f);
                return false;
            }
            out->tiles = calloc((size_t)out->width * (size_t)out->height, 1);
            if (!out->tiles) {
                fclose(f);
                return false;
            }
            continue;
        }
        if (strcmp(s, "entities:") == 0) {
            section = SEC_ENTITIES;
            continue;
        }
        if (strcmp(s, "audio_zones:") == 0) {
            section = SEC_ZONES;
            continue;
        }

        if (section == SEC_HEADER) {
            parse_header_line(out, s);
        } else if (section == SEC_TILES) {
            if (tile_row >= out->height || (int)strlen(s) != out->width) {
                fclose(f);
                return false;
            }
            memcpy(&out->tiles[tile_row * out->width], s, (size_t)out->width);
            tile_row++;
        } else if (section == SEC_ENTITIES) {
            if (!parse_entity_line(out, s)) {
                fclose(f);
                return false;
            }
        } else if (section == SEC_ZONES) {
            if (!parse_zone_line(out, s)) {
                fclose(f);
                return false;
            }
        }
    }
    fclose(f);
    return out->name_key[0] && out->theme[0] && out->width > 0 && out->height > 0 &&
           out->tiles != NULL;
}

void maze_data_free(MazeFile *m) {
    if (!m) {
        return;
    }
    free(m->tiles);
    free(m->entities);
    free(m->zones);
    *m = (MazeFile){0};
}

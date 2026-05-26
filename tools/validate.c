#include "validate.h"

#include "dialogs.h"
#include "entity.h"
#include "items.h"
#include "localization.h"
#include "maze_data.h"
#include "world.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int add_diag(Diag *out, int max, int n, DiagSeverity severity, const char *msg) {
    if (n < max) {
        out[n].severity = severity;
        snprintf(out[n].msg, sizeof out[n].msg, "%s", msg);
    }
    return n + 1;
}

static const char *arg_value(const MazeEntityLine *line, const char *prefix) {
    size_t n = strlen(prefix);
    for (int i = 0; i < line->arg_count; i++) {
        if (strncmp(line->args[i], prefix, n) == 0) {
            return line->args[i] + n;
        }
    }
    return NULL;
}

static void load_registries(void) {
    items_shutdown();
    dialogs_shutdown();
    loc_shutdown();
    items_load("tests/fixtures/items.txt");
    dialogs_load("tests/fixtures/dialogs.txt");
    loc_load_language_from("tests/fixtures", "en");
    items_load("data/items.txt");
    dialogs_load("data/dialogs/nature.txt");
    loc_load_language_from("data/strings", "en");
    entity_registry_init();
    entity_register_builtin_stubs();
}

static bool has_prefix(const char *s, const char *prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

static const char *theme_dir_for(const MazeFile *m, const char *maze_path, char out[256]) {
    if (strncmp(maze_path, "tests/", 6) == 0) {
        snprintf(out, 256, "tests/fixtures/themes/test");
    } else {
        snprintf(out, 256, "assets/themes/%s", m->theme);
    }
    return out;
}

static int check_reachability(const char *maze_path, const MazeFile *m, Diag *out, int max, int n) {
    World w = {0};
    char theme_dir[256];
    if (!world_load_maze(&w, maze_path, theme_dir_for(m, maze_path, theme_dir))) {
        return add_diag(out, max, n, DIAG_ERROR, "world load failed for reachability");
    }
    if (!world_tile_walkable(&w, w.maze.start_x, w.maze.start_y)) {
        n = add_diag(out, max, n, DIAG_ERROR, "start tile is not walkable");
        world_unload(&w);
        return n;
    }
    int total = w.maze.width * w.maze.height;
    bool *seen = calloc((size_t)total, sizeof *seen);
    int *qx = calloc((size_t)total, sizeof *qx);
    int *qy = calloc((size_t)total, sizeof *qy);
    if (!seen || !qx || !qy) {
        free(seen);
        free(qx);
        free(qy);
        world_unload(&w);
        return add_diag(out, max, n, DIAG_ERROR, "reachability allocation failed");
    }
    int head = 0, tail = 0;
    qx[tail] = w.maze.start_x;
    qy[tail++] = w.maze.start_y;
    seen[w.maze.start_y * w.maze.width + w.maze.start_x] = true;
    static const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (head < tail) {
        int x = qx[head], y = qy[head++];
        for (int i = 0; i < 4; i++) {
            int nx = x + dirs[i][0], ny = y + dirs[i][1];
            if (nx < 0 || ny < 0 || nx >= w.maze.width || ny >= w.maze.height ||
                seen[ny * w.maze.width + nx] || !world_tile_walkable(&w, nx, ny)) {
                continue;
            }
            seen[ny * w.maze.width + nx] = true;
            qx[tail] = nx;
            qy[tail++] = ny;
        }
    }
    for (int i = 0; i < m->entity_count; i++) {
        const MazeEntityLine *e = &m->entities[i];
        if (strcmp(e->behavior, "pickup") == 0) {
            int idx = e->tile_y * m->width + e->tile_x;
            if (idx < 0 || idx >= total || !seen[idx]) {
                char msg[256];
                snprintf(msg, sizeof msg, "unreachable pickup: %s", e->stable_id);
                n = add_diag(out, max, n, DIAG_ERROR, msg);
            }
        }
    }
    free(seen);
    free(qx);
    free(qy);
    world_unload(&w);
    return n;
}

int validate_run(const char *maze_path, Diag *out, int max) {
    int n = 0;
    load_registries();
    MazeFile m = {0};
    if (!maze_data_load(maze_path, &m)) {
        return add_diag(out, max, n, DIAG_ERROR, "missing or malformed required header: name");
    }
    if (!m.name_key[0] || !m.theme[0] || m.width <= 0 || m.height <= 0) {
        n = add_diag(out, max, n, DIAG_ERROR, "missing required header");
    }
    for (int i = 0; i < m.entity_count; i++) {
        const MazeEntityLine *e = &m.entities[i];
        for (int j = i + 1; j < m.entity_count; j++) {
            if (strcmp(e->stable_id, m.entities[j].stable_id) == 0) {
                n = add_diag(out, max, n, DIAG_ERROR, "duplicate stable_id");
            }
        }
        char prefix[64];
        snprintf(prefix, sizeof prefix, "%s.", m.maze_id);
        if (!has_prefix(e->stable_id, prefix)) {
            n = add_diag(out, max, n, DIAG_WARN, "stable_id does not start with maze prefix");
        }
        if (!entity_registry_find(e->behavior)) {
            n = add_diag(out, max, n, DIAG_ERROR, "unknown behavior");
        }
        const char *item = arg_value(e, "item:");
        if (item && !items_find(item)) {
            n = add_diag(out, max, n, DIAG_ERROR, "unknown item reference");
        }
        const char *dialog = arg_value(e, "dialog:");
        if (dialog && !dialogs_find(dialog)) {
            n = add_diag(out, max, n, DIAG_ERROR, "unknown dialog reference");
        }
    }
    for (int i = 0; i < m.zone_count; i++) {
        AudioZoneLine *z = &m.zones[i];
        if (z->x0 < 0 || z->y0 < 0 || z->x1 >= m.width || z->y1 >= m.height) {
            n = add_diag(out, max, n, DIAG_ERROR, "audio zone out of bounds");
        }
    }
    n = check_reachability(maze_path, &m, out, max, n);
    maze_data_free(&m);
    return n;
}

#ifndef AMAZEING_TESTS
int main(int argc, char **argv) {
    bool strict = false;
    const char *path = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--strict") == 0) {
            strict = true;
        } else {
            path = argv[i];
        }
    }
    if (!path) {
        fprintf(stderr, "usage: validate [--strict] <maze.maze>\n");
        return 2;
    }
    Diag diags[256];
    int n = validate_run(path, diags, 256);
    int fatal = 0;
    for (int i = 0; i < n; i++) {
        bool is_fatal = diags[i].severity == DIAG_ERROR || (strict && diags[i].severity == DIAG_WARN);
        printf("%s: %s\n", is_fatal ? "ERROR" : "WARN", diags[i].msg);
        if (is_fatal) {
            fatal++;
        }
    }
    return fatal ? 1 : 0;
}
#endif

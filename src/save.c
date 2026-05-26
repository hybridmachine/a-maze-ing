#include "save.h"

#include "util_log.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static sqlite3 *g_db;
static char g_db_path[1024];

static const char *const MIGRATIONS[] = {
    "CREATE TABLE profiles ("
    "id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE,"
    "created_at INTEGER NOT NULL, last_played INTEGER NOT NULL);"
    "CREATE TABLE settings ("
    "profile_id INTEGER REFERENCES profiles(id) ON DELETE CASCADE,"
    "key TEXT NOT NULL, value TEXT NOT NULL,"
    "PRIMARY KEY (profile_id, key));"
    "CREATE TABLE profile_items ("
    "profile_id INTEGER REFERENCES profiles(id) ON DELETE CASCADE,"
    "item_id TEXT NOT NULL, acquired_at INTEGER NOT NULL,"
    "PRIMARY KEY (profile_id, item_id));"
    "CREATE TABLE outfit_worn ("
    "profile_id INTEGER PRIMARY KEY REFERENCES profiles(id) ON DELETE CASCADE,"
    "outfit_id TEXT NOT NULL);"
    "CREATE TABLE maze_inventory ("
    "profile_id INTEGER REFERENCES profiles(id) ON DELETE CASCADE,"
    "maze_id TEXT NOT NULL, item_id TEXT NOT NULL,"
    "count INTEGER NOT NULL DEFAULT 1, acquired_at INTEGER NOT NULL,"
    "PRIMARY KEY (profile_id, maze_id, item_id));"
    "CREATE TABLE maze_progress ("
    "profile_id INTEGER REFERENCES profiles(id) ON DELETE CASCADE,"
    "maze_id TEXT NOT NULL, state TEXT NOT NULL,"
    "completed_at INTEGER, PRIMARY KEY (profile_id, maze_id));"
    "CREATE TABLE maze_snapshot ("
    "profile_id INTEGER, maze_id TEXT,"
    "player_x REAL NOT NULL, player_y REAL NOT NULL,"
    "facing INTEGER NOT NULL,"
    "clock_min INTEGER NOT NULL, day_index INTEGER NOT NULL,"
    "PRIMARY KEY (profile_id, maze_id),"
    "FOREIGN KEY (profile_id) REFERENCES profiles(id) ON DELETE CASCADE);"
    "CREATE TABLE entity_overrides ("
    "profile_id INTEGER, maze_id TEXT, stable_id TEXT NOT NULL,"
    "taken INTEGER NOT NULL DEFAULT 0, active INTEGER NOT NULL DEFAULT 0,"
    "last_dialog_day INTEGER NOT NULL DEFAULT -1, state_blob BLOB,"
    "PRIMARY KEY (profile_id, maze_id, stable_id),"
    "FOREIGN KEY (profile_id) REFERENCES profiles(id) ON DELETE CASCADE);"};

#define MIGRATION_COUNT ((int)(sizeof MIGRATIONS / sizeof MIGRATIONS[0]))

sqlite3 *save_db_handle(void) {
    return g_db;
}

static bool exec_sql(const char *sql) {
    char *err = NULL;
    int rc = sqlite3_exec(g_db, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        LOGE("sqlite exec failed: %s", err ? err : sqlite3_errmsg(g_db));
        sqlite3_free(err);
        return false;
    }
    return true;
}

static bool run_migrations(void) {
    sqlite3_stmt *s = NULL;
    int version = 0;
    if (sqlite3_prepare_v2(g_db, "PRAGMA user_version;", -1, &s, NULL) != SQLITE_OK) {
        return false;
    }
    if (sqlite3_step(s) == SQLITE_ROW) {
        version = sqlite3_column_int(s, 0);
    }
    sqlite3_finalize(s);
    if (version >= MIGRATION_COUNT) {
        return true;
    }
    if (!exec_sql("BEGIN;")) {
        return false;
    }
    for (int i = version; i < MIGRATION_COUNT; i++) {
        if (!exec_sql(MIGRATIONS[i])) {
            exec_sql("ROLLBACK;");
            return false;
        }
    }
    char pragma[64];
    snprintf(pragma, sizeof pragma, "PRAGMA user_version=%d;", MIGRATION_COUNT);
    if (!exec_sql(pragma)) {
        exec_sql("ROLLBACK;");
        return false;
    }
    return exec_sql("COMMIT;");
}

bool save_integrity_check(void) {
    sqlite3_stmt *s = NULL;
    if (!g_db || sqlite3_prepare_v2(g_db, "PRAGMA integrity_check;", -1, &s, NULL) != SQLITE_OK) {
        return false;
    }
    bool ok = false;
    if (sqlite3_step(s) == SQLITE_ROW) {
        const unsigned char *txt = sqlite3_column_text(s, 0);
        ok = txt && strcmp((const char *)txt, "ok") == 0;
    }
    sqlite3_finalize(s);
    return ok;
}

SaveOpenResult save_open_checked(const char *path) {
    save_close();
    snprintf(g_db_path, sizeof g_db_path, "%s", path);
    if (sqlite3_open(path, &g_db) != SQLITE_OK) {
        LOGE("sqlite open failed: %s", g_db ? sqlite3_errmsg(g_db) : path);
        save_close();
        return SAVE_FAIL;
    }
    exec_sql("PRAGMA journal_mode=WAL;");
    exec_sql("PRAGMA foreign_keys=ON;");
    if (!save_integrity_check()) {
        save_backup("integrity_check_failed");
        return SAVE_CORRUPT;
    }
    return run_migrations() ? SAVE_OK : SAVE_FAIL;
}

bool save_open(const char *path) {
    return save_open_checked(path) == SAVE_OK;
}

void save_close(void) {
    if (g_db) {
        sqlite3_wal_checkpoint_v2(g_db, NULL, SQLITE_CHECKPOINT_TRUNCATE, NULL, NULL);
        sqlite3_close(g_db);
        g_db = NULL;
    }
}

bool save_backup(const char *reason) {
    if (!g_db || !g_db_path[0]) {
        return false;
    }
    time_t now = time(NULL);
    struct tm tmv;
#ifdef _WIN32
    localtime_s(&tmv, &now);
#else
    localtime_r(&now, &tmv);
#endif
    char dst[1200];
    snprintf(dst, sizeof dst, "%s.%04d%02d%02d-%02d%02d%02d.bak", g_db_path,
             tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min,
             tmv.tm_sec);
    sqlite3 *out = NULL;
    if (sqlite3_open(dst, &out) != SQLITE_OK) {
        return false;
    }
    sqlite3_backup *b = sqlite3_backup_init(out, "main", g_db, "main");
    if (!b) {
        sqlite3_close(out);
        return false;
    }
    sqlite3_backup_step(b, -1);
    sqlite3_backup_finish(b);
    sqlite3_close(out);
    LOGW("save backup written (%s): %s", reason, dst);
    return true;
}

bool save_reset_to_blank(void) {
    char path[1024];
    snprintf(path, sizeof path, "%s", g_db_path);
    save_close();
    unlink(path);
    return save_open(path);
}

static bool step_done(sqlite3_stmt *s) {
    int rc = sqlite3_step(s);
    if (rc != SQLITE_DONE) {
        LOGE("sqlite step failed: %s", sqlite3_errmsg(g_db));
        sqlite3_finalize(s);
        return false;
    }
    sqlite3_finalize(s);
    return true;
}

bool save_set_clock(int pid, const char *mid, int clock_min, int day_index) {
    sqlite3_stmt *s = NULL;
    const char *sql =
        "INSERT INTO maze_snapshot(profile_id,maze_id,player_x,player_y,facing,clock_min,day_index)"
        "VALUES(?,?,0,0,0,?,?) "
        "ON CONFLICT(profile_id,maze_id) DO UPDATE SET clock_min=excluded.clock_min,"
        "day_index=excluded.day_index;";
    if (sqlite3_prepare_v2(g_db, sql, -1, &s, NULL) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(s, 1, pid);
    sqlite3_bind_text(s, 2, mid, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(s, 3, clock_min);
    sqlite3_bind_int(s, 4, day_index);
    return step_done(s);
}

bool save_set_position(int pid, const char *mid, float x, float y, int facing) {
    sqlite3_stmt *s = NULL;
    const char *sql =
        "INSERT INTO maze_snapshot(profile_id,maze_id,player_x,player_y,facing,clock_min,day_index)"
        "VALUES(?,?,?,?,?,0,0) "
        "ON CONFLICT(profile_id,maze_id) DO UPDATE SET player_x=excluded.player_x,"
        "player_y=excluded.player_y,facing=excluded.facing;";
    if (sqlite3_prepare_v2(g_db, sql, -1, &s, NULL) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(s, 1, pid);
    sqlite3_bind_text(s, 2, mid, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(s, 3, x);
    sqlite3_bind_double(s, 4, y);
    sqlite3_bind_int(s, 5, facing);
    return step_done(s);
}

bool save_set_progress(int pid, const char *mid, const char *state) {
    sqlite3_stmt *s = NULL;
    const char *sql =
        "INSERT INTO maze_progress(profile_id,maze_id,state,completed_at) VALUES(?,?,?,NULL) "
        "ON CONFLICT(profile_id,maze_id) DO UPDATE SET state=excluded.state,"
        "completed_at=CASE WHEN excluded.state='complete' THEN strftime('%s','now') ELSE NULL END;";
    if (sqlite3_prepare_v2(g_db, sql, -1, &s, NULL) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(s, 1, pid);
    sqlite3_bind_text(s, 2, mid, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s, 3, state, -1, SQLITE_TRANSIENT);
    return step_done(s);
}

bool save_set_inventory_add(int pid, const char *mid, const char *item_id, int delta) {
    sqlite3_stmt *s = NULL;
    const char *sql =
        "INSERT INTO maze_inventory(profile_id,maze_id,item_id,count,acquired_at) "
        "VALUES(?,?,?,?,strftime('%s','now')) "
        "ON CONFLICT(profile_id,maze_id,item_id) DO UPDATE SET "
        "count=max(0, count + excluded.count);";
    if (sqlite3_prepare_v2(g_db, sql, -1, &s, NULL) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(s, 1, pid);
    sqlite3_bind_text(s, 2, mid, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s, 3, item_id, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(s, 4, delta);
    if (!step_done(s)) {
        return false;
    }
    return exec_sql("DELETE FROM maze_inventory WHERE count<=0;");
}

bool save_set_profile_item(int pid, const char *item_id) {
    sqlite3_stmt *s = NULL;
    const char *sql =
        "INSERT INTO profile_items(profile_id,item_id,acquired_at) VALUES(?,?,strftime('%s','now')) "
        "ON CONFLICT(profile_id,item_id) DO NOTHING;";
    if (sqlite3_prepare_v2(g_db, sql, -1, &s, NULL) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(s, 1, pid);
    sqlite3_bind_text(s, 2, item_id, -1, SQLITE_TRANSIENT);
    return step_done(s);
}

bool save_set_outfit_worn(int pid, const char *outfit_id) {
    sqlite3_stmt *s = NULL;
    const char *sql =
        "INSERT INTO outfit_worn(profile_id,outfit_id) VALUES(?,?) "
        "ON CONFLICT(profile_id) DO UPDATE SET outfit_id=excluded.outfit_id;";
    if (sqlite3_prepare_v2(g_db, sql, -1, &s, NULL) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(s, 1, pid);
    sqlite3_bind_text(s, 2, outfit_id, -1, SQLITE_TRANSIENT);
    return step_done(s);
}

bool save_set_entity_override(int pid, const char *mid, const char *sid, int taken, int active,
                              int last_dialog_day) {
    sqlite3_stmt *s = NULL;
    const char *sql =
        "INSERT INTO entity_overrides(profile_id,maze_id,stable_id,taken,active,last_dialog_day)"
        "VALUES(?,?,?,?,?,?) "
        "ON CONFLICT(profile_id,maze_id,stable_id) DO UPDATE SET taken=excluded.taken,"
        "active=excluded.active,last_dialog_day=excluded.last_dialog_day;";
    if (sqlite3_prepare_v2(g_db, sql, -1, &s, NULL) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(s, 1, pid);
    sqlite3_bind_text(s, 2, mid, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s, 3, sid, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(s, 4, taken);
    sqlite3_bind_int(s, 5, active);
    sqlite3_bind_int(s, 6, last_dialog_day);
    return step_done(s);
}

bool save_load_maze_snapshot(int pid, const char *mid, MazeSnapshot *out) {
    sqlite3_stmt *s = NULL;
    const char *sql =
        "SELECT player_x,player_y,facing,clock_min,day_index FROM maze_snapshot "
        "WHERE profile_id=? AND maze_id=?;";
    if (sqlite3_prepare_v2(g_db, sql, -1, &s, NULL) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(s, 1, pid);
    sqlite3_bind_text(s, 2, mid, -1, SQLITE_TRANSIENT);
    bool found = sqlite3_step(s) == SQLITE_ROW;
    if (found && out) {
        out->x = (float)sqlite3_column_double(s, 0);
        out->y = (float)sqlite3_column_double(s, 1);
        out->facing = sqlite3_column_int(s, 2);
        out->clock_min = sqlite3_column_int(s, 3);
        out->day_index = sqlite3_column_int(s, 4);
    }
    sqlite3_finalize(s);
    return found;
}

static int load_ids(const char *sql, int pid, const char *mid, char ids[][48], int max) {
    sqlite3_stmt *s = NULL;
    if (sqlite3_prepare_v2(g_db, sql, -1, &s, NULL) != SQLITE_OK) {
        return 0;
    }
    sqlite3_bind_int(s, 1, pid);
    if (mid) {
        sqlite3_bind_text(s, 2, mid, -1, SQLITE_TRANSIENT);
    }
    int n = 0;
    while (n < max && sqlite3_step(s) == SQLITE_ROW) {
        snprintf(ids[n++], 48, "%s", sqlite3_column_text(s, 0));
    }
    sqlite3_finalize(s);
    return n;
}

int save_load_profile_items(int pid, char ids[][48], int max) {
    return load_ids("SELECT item_id FROM profile_items WHERE profile_id=? ORDER BY item_id;", pid,
                    NULL, ids, max);
}

int save_load_maze_inventory(int pid, const char *mid, char ids[][48], int max) {
    return load_ids("SELECT item_id FROM maze_inventory WHERE profile_id=? AND maze_id=? "
                    "AND count>0 ORDER BY item_id;",
                    pid, mid, ids, max);
}

bool save_load_maze_inventory_count(int pid, const char *mid, const char *item_id, int *out_count) {
    sqlite3_stmt *s = NULL;
    const char *sql =
        "SELECT count FROM maze_inventory WHERE profile_id=? AND maze_id=? AND item_id=?;";
    if (sqlite3_prepare_v2(g_db, sql, -1, &s, NULL) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(s, 1, pid);
    sqlite3_bind_text(s, 2, mid, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s, 3, item_id, -1, SQLITE_TRANSIENT);
    bool found = sqlite3_step(s) == SQLITE_ROW;
    if (found && out_count) {
        *out_count = sqlite3_column_int(s, 0);
    }
    sqlite3_finalize(s);
    return found;
}

bool save_load_outfit_worn(int pid, char out[48]) {
    sqlite3_stmt *s = NULL;
    if (sqlite3_prepare_v2(g_db, "SELECT outfit_id FROM outfit_worn WHERE profile_id=?;", -1, &s,
                           NULL) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(s, 1, pid);
    bool found = sqlite3_step(s) == SQLITE_ROW;
    if (found) {
        snprintf(out, 48, "%s", sqlite3_column_text(s, 0));
    }
    sqlite3_finalize(s);
    return found;
}

bool save_load_entity_override(int pid, const char *mid, const char *sid, int *taken, int *active,
                               int *last_dialog_day) {
    sqlite3_stmt *s = NULL;
    const char *sql =
        "SELECT taken,active,last_dialog_day FROM entity_overrides "
        "WHERE profile_id=? AND maze_id=? AND stable_id=?;";
    if (sqlite3_prepare_v2(g_db, sql, -1, &s, NULL) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(s, 1, pid);
    sqlite3_bind_text(s, 2, mid, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s, 3, sid, -1, SQLITE_TRANSIENT);
    bool found = sqlite3_step(s) == SQLITE_ROW;
    if (found) {
        if (taken) {
            *taken = sqlite3_column_int(s, 0);
        }
        if (active) {
            *active = sqlite3_column_int(s, 1);
        }
        if (last_dialog_day) {
            *last_dialog_day = sqlite3_column_int(s, 2);
        }
    }
    sqlite3_finalize(s);
    return found;
}

bool save_checkpoint(const CheckpointPayload *p, CheckpointReason reason) {
    if (!p || !g_db) {
        return false;
    }
    static const char *names[] = {"item", "area", "dialog", "day", "pause", "quit"};
    if (!exec_sql("BEGIN;")) {
        return false;
    }
    bool ok = true;
    if (p->pos_dirty) {
        ok = save_set_position(p->profile_id, p->maze_id, p->pos.x, p->pos.y, p->pos.facing) &&
             save_set_clock(p->profile_id, p->maze_id, p->pos.clock_min, p->pos.day_index);
    }
    if (ok && p->prog_dirty) {
        ok = save_set_progress(p->profile_id, p->maze_id, p->prog_state);
    }
    for (int i = 0; ok && i < p->inv_add_count; i++) {
        ok = save_set_inventory_add(p->profile_id, p->maze_id, p->inv_adds[i].item_id,
                                    p->inv_adds[i].delta);
    }
    for (int i = 0; ok && i < p->profile_item_count; i++) {
        ok = save_set_profile_item(p->profile_id, p->profile_items[i]);
    }
    if (ok && p->outfit_dirty) {
        ok = save_set_outfit_worn(p->profile_id, p->outfit_id);
    }
    for (int i = 0; ok && i < p->ovr_count; i++) {
        ok = save_set_entity_override(p->profile_id, p->maze_id, p->ovrs[i].stable_id,
                                      p->ovrs[i].taken, p->ovrs[i].active,
                                      p->ovrs[i].last_dialog_day);
    }
    if (!ok) {
        exec_sql("ROLLBACK;");
        LOGE("checkpoint failed: %s", names[reason]);
        return false;
    }
    LOGI("checkpoint: %s", names[reason]);
    return exec_sql("COMMIT;");
}

bool save_reset_maze(int pid, const char *mid) {
    sqlite3_stmt *s = NULL;
    if (!exec_sql("BEGIN;")) {
        return false;
    }
    const char *sqls[] = {
        "DELETE FROM maze_inventory WHERE profile_id=? AND maze_id=?;",
        "DELETE FROM entity_overrides WHERE profile_id=? AND maze_id=?;",
        "DELETE FROM maze_snapshot WHERE profile_id=? AND maze_id=?;",
        ("INSERT INTO maze_progress(profile_id,maze_id,state) VALUES(?,?, 'in_progress') "
         "ON CONFLICT(profile_id,maze_id) DO UPDATE SET state='in_progress', completed_at=NULL;")};
    for (int i = 0; i < 4; i++) {
        if (sqlite3_prepare_v2(g_db, sqls[i], -1, &s, NULL) != SQLITE_OK) {
            exec_sql("ROLLBACK;");
            return false;
        }
        sqlite3_bind_int(s, 1, pid);
        sqlite3_bind_text(s, 2, mid, -1, SQLITE_TRANSIENT);
        if (!step_done(s)) {
            exec_sql("ROLLBACK;");
            return false;
        }
    }
    return exec_sql("COMMIT;");
}

bool settings_set(int pid, const char *key, const char *value) {
    sqlite3_stmt *s = NULL;
    const char *sql =
        "INSERT INTO settings(profile_id,key,value) VALUES(?,?,?) "
        "ON CONFLICT(profile_id,key) DO UPDATE SET value=excluded.value;";
    if (sqlite3_prepare_v2(g_db, sql, -1, &s, NULL) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(s, 1, pid);
    sqlite3_bind_text(s, 2, key, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s, 3, value, -1, SQLITE_TRANSIENT);
    return step_done(s);
}

bool settings_get(int pid, const char *key, char *out, int out_size) {
    sqlite3_stmt *s = NULL;
    if (sqlite3_prepare_v2(g_db, "SELECT value FROM settings WHERE profile_id=? AND key=?;", -1,
                           &s, NULL) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(s, 1, pid);
    sqlite3_bind_text(s, 2, key, -1, SQLITE_TRANSIENT);
    bool found = sqlite3_step(s) == SQLITE_ROW;
    if (found && out_size > 0) {
        snprintf(out, (size_t)out_size, "%s", sqlite3_column_text(s, 0));
    }
    sqlite3_finalize(s);
    return found;
}

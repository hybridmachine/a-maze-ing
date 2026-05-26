#include "profile.h"

#include "save.h"

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int profile_list(Profile *out, int max) {
    sqlite3 *db = save_db_handle();
    sqlite3_stmt *s = NULL;
    if (!db ||
        sqlite3_prepare_v2(db, "SELECT id,name,last_played FROM profiles ORDER BY last_played DESC;",
                           -1, &s, NULL) != SQLITE_OK) {
        return 0;
    }
    int n = 0;
    while (n < max && sqlite3_step(s) == SQLITE_ROW) {
        out[n].id = sqlite3_column_int(s, 0);
        snprintf(out[n].name, sizeof out[n].name, "%s", sqlite3_column_text(s, 1));
        out[n].last_played = (long)sqlite3_column_int64(s, 2);
        n++;
    }
    sqlite3_finalize(s);
    return n;
}

int profile_create(const char *name) {
    Profile ps[PROFILE_MAX_COUNT];
    if (profile_list(ps, PROFILE_MAX_COUNT) >= PROFILE_MAX_COUNT || !name || !*name) {
        return -1;
    }
    sqlite3 *db = save_db_handle();
    sqlite3_stmt *s = NULL;
    long now = (long)time(NULL);
    if (sqlite3_prepare_v2(db,
                           "INSERT INTO profiles(name,created_at,last_played) VALUES(?,?,?);", -1,
                           &s, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_text(s, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(s, 2, now);
    sqlite3_bind_int64(s, 3, now);
    int rc = sqlite3_step(s);
    sqlite3_finalize(s);
    return rc == SQLITE_DONE ? (int)sqlite3_last_insert_rowid(db) : -1;
}

bool profile_delete(int id) {
    sqlite3 *db = save_db_handle();
    sqlite3_stmt *s = NULL;
    if (sqlite3_prepare_v2(db, "DELETE FROM profiles WHERE id=?;", -1, &s, NULL) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(s, 1, id);
    bool ok = sqlite3_step(s) == SQLITE_DONE;
    sqlite3_finalize(s);
    return ok;
}

bool profile_touch(int id) {
    sqlite3 *db = save_db_handle();
    sqlite3_stmt *s = NULL;
    if (sqlite3_prepare_v2(db, "UPDATE profiles SET last_played=? WHERE id=?;", -1, &s, NULL) !=
        SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(s, 1, (sqlite3_int64)time(NULL));
    sqlite3_bind_int(s, 2, id);
    bool ok = sqlite3_step(s) == SQLITE_DONE;
    sqlite3_finalize(s);
    return ok;
}

#include "util_paths.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <shlobj.h>
#endif

static bool ensure_dir(const char *path) {
#ifdef _WIN32
    return _mkdir(path) == 0 || errno == EEXIST;
#else
    return mkdir(path, 0700) == 0 || errno == EEXIST;
#endif
}

bool util_paths_userdata_dir(char *out, size_t out_size) {
    if (!out || out_size == 0) {
        return false;
    }
#ifdef __APPLE__
    const char *home = getenv("HOME");
    if (!home) {
        return false;
    }
    snprintf(out, out_size, "%s/Library/Application Support/AMazeIng", home);
#elif defined(_WIN32)
    char appdata[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata) != S_OK) {
        return false;
    }
    snprintf(out, out_size, "%s\\AMazeIng", appdata);
#else
    const char *xdg = getenv("XDG_DATA_HOME");
    const char *home = getenv("HOME");
    if (xdg && *xdg) {
        snprintf(out, out_size, "%s/AMazeIng", xdg);
    } else if (home) {
        snprintf(out, out_size, "%s/.local/share/AMazeIng", home);
    } else {
        return false;
    }
#endif
    return ensure_dir(out);
}

bool util_paths_save_db(char *out, size_t out_size) {
    char dir[1024];
    if (!util_paths_userdata_dir(dir, sizeof dir)) {
        return false;
    }
#ifdef _WIN32
    snprintf(out, out_size, "%s\\amazeing.db", dir);
#else
    snprintf(out, out_size, "%s/amazeing.db", dir);
#endif
    return true;
}

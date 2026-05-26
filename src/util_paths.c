#include "util_paths.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#ifdef _WIN32
#include <direct.h>
#include <shlobj.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static bool ensure_dir(const char *path) {
#ifdef _WIN32
    return _mkdir(path) == 0 || errno == EEXIST;
#else
    return mkdir(path, 0700) == 0 || errno == EEXIST;
#endif
}

static bool copy_path(char *out, size_t out_size, const char *value) {
    if (!out || out_size == 0 || !value) {
        return false;
    }
    int n = snprintf(out, out_size, "%s", value);
    return n >= 0 && (size_t)n < out_size;
}

static bool is_absolute_path(const char *path) {
    if (!path || !*path) {
        return false;
    }
#ifdef _WIN32
    return path[0] == '\\' || path[0] == '/' ||
           (strlen(path) > 2 && path[1] == ':' && (path[2] == '\\' || path[2] == '/'));
#else
    return path[0] == '/';
#endif
}

static bool path_is_dir(const char *path) {
#ifdef _WIN32
    struct _stat st;
    return _stat(path, &st) == 0 && (st.st_mode & _S_IFDIR) != 0;
#else
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
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

bool util_paths_resource_dir(char *out, size_t out_size) {
    if (!out || out_size == 0) {
        return false;
    }
#ifdef __APPLE__
    char executable[PATH_MAX];
    uint32_t executable_size = (uint32_t)sizeof executable;
    if (_NSGetExecutablePath(executable, &executable_size) != 0) {
        return false;
    }

    char resolved[PATH_MAX];
    const char *executable_path = realpath(executable, resolved);
    if (!executable_path) {
        executable_path = executable;
    }

    const char marker[] = "/Contents/MacOS/";
    const char *match = NULL;
    const char *scan = executable_path;
    while ((scan = strstr(scan, marker)) != NULL) {
        match = scan;
        scan++;
    }
    if (!match || match - executable_path < 4 || strncmp(match - 4, ".app", 4) != 0) {
        return false;
    }

    size_t bundle_len = (size_t)(match - executable_path);
    if (bundle_len > INT_MAX) {
        return false;
    }
    int n = snprintf(out, out_size, "%.*s/Contents/Resources", (int)bundle_len,
                     executable_path);
    return n >= 0 && (size_t)n < out_size && path_is_dir(out);
#else
    (void)out;
    (void)out_size;
    return false;
#endif
}

bool util_paths_content_path(const char *relative_path, char *out, size_t out_size) {
    if (!relative_path || !*relative_path || !out || out_size == 0) {
        return false;
    }
    if (is_absolute_path(relative_path)) {
        return copy_path(out, out_size, relative_path);
    }

    char resource_dir[PATH_MAX];
    if (util_paths_resource_dir(resource_dir, sizeof resource_dir)) {
        int n = snprintf(out, out_size, "%s/%s", resource_dir, relative_path);
        return n >= 0 && (size_t)n < out_size;
    }
    return copy_path(out, out_size, relative_path);
}

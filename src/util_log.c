#include "util_log.h"

#include <stdio.h>
#include <time.h>

static FILE *g_log_file;

void util_log_init(const char *file_path) {
    if (file_path) {
        g_log_file = fopen(file_path, "a");
    }
}

void util_log(LogLevel lvl, const char *fmt, ...) {
    static const char *names[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    if ((int)lvl < 0 || lvl > AMZ_LOG_ERROR) {
        lvl = AMZ_LOG_ERROR;
    }

    time_t now = time(NULL);
    struct tm tmv;
#ifdef _WIN32
    localtime_s(&tmv, &now);
#else
    localtime_r(&now, &tmv);
#endif

    char prefix[64];
    snprintf(prefix, sizeof prefix, "[%02d:%02d:%02d %s] ", tmv.tm_hour, tmv.tm_min,
             tmv.tm_sec, names[lvl]);

    va_list ap;
    va_start(ap, fmt);
    fputs(prefix, stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);

    if (g_log_file) {
        va_start(ap, fmt);
        fputs(prefix, g_log_file);
        vfprintf(g_log_file, fmt, ap);
        fputc('\n', g_log_file);
        fflush(g_log_file);
        va_end(ap);
    }
}

void util_log_close(void) {
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
}

#pragma once

#include <stdarg.h>

typedef enum { AMZ_LOG_DEBUG, AMZ_LOG_INFO, AMZ_LOG_WARN, AMZ_LOG_ERROR } LogLevel;

void util_log_init(const char *file_path);
void util_log(LogLevel lvl, const char *fmt, ...);
void util_log_close(void);

#define LOGD(...) util_log(AMZ_LOG_DEBUG, __VA_ARGS__)
#define LOGI(...) util_log(AMZ_LOG_INFO, __VA_ARGS__)
#define LOGW(...) util_log(AMZ_LOG_WARN, __VA_ARGS__)
#define LOGE(...) util_log(AMZ_LOG_ERROR, __VA_ARGS__)

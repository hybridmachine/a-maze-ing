#pragma once

typedef enum { DIAG_WARN, DIAG_ERROR } DiagSeverity;

typedef struct Diag {
    DiagSeverity severity;
    char msg[256];
} Diag;

int validate_run(const char *maze_path, Diag *out, int max);

#include "ui.h"

#include <raylib.h>

bool ui_recovery_confirm(const char *message) {
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ENTER)) {
            return true;
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            return false;
        }
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText(message, 32, 120, 20, RAYWHITE);
        DrawText("Enter: fresh save   Esc: quit", 32, 160, 18, LIGHTGRAY);
        EndDrawing();
    }
    return false;
}

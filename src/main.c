#include "raylib.h"

int main(void) {
    InitWindow(1600, 900, "print3");

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(WHITE);
        DrawRectangle(200, 200, 1200, 500, BLUE);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}

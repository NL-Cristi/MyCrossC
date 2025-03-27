#include "raylib.h"

int main(void) {
    // Initialization
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Hello World - Raylib");

    SetTargetFPS(60); // Set the game to run at 60 frames-per-second

    // Main game loop
    while (!WindowShouldClose()) { // Detect window close button or ESC key
      // Start drawing
      BeginDrawing();

      ClearBackground(RAYWHITE);
      DrawText("Hello, World!", 350, 200, 20, GREEN);
      DrawText("Press ESC to exit", 290, 240, 20, GREEN);
      EndDrawing();
    }

    // De-Initialization
    CloseWindow(); // Close window and OpenGL context

    return 0;
  }
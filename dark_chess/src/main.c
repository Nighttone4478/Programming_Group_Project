#include "raylib.h"
#include "game.h"
#include "render.h"
#include "assets.h"
#include <stdlib.h>
#include <time.h>

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 540;

    InitWindow(screenWidth, screenHeight, "暗棋");
    SetTargetFPS(60);

    srand((unsigned int)time(NULL));

    Assets assets;
    if (!load_assets(&assets)) {
        CloseWindow();
        return 1;
    }

    Game game;
    init_game(&game);

    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            handle_player_click(&game, GetMouseX(), GetMouseY());
        }

        update_game(&game);

        BeginDrawing();
        draw_game(&game, &assets);
        EndDrawing();
    }

    unload_assets(&assets);
    CloseWindow();
    return 0;
}
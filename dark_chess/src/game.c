#include "game.h"
#include "ai.h"
#include "raylib.h"

#define CELL_SIZE 90
#define BOARD_OFFSET_X 40
#define BOARD_OFFSET_Y 100

#define AI_THINK_DELAY      1.0f   // 電腦翻牌前延遲
#define AI_SHOW_DELAY       0.6f   // 電腦翻牌後高光保留時間

void init_game(Game *game)
{
    init_board(&game->board);
    game->turn = TURN_PLAYER;
    game->game_over = 0;

    game->ai_delay_timer = 0.0f;
    game->highlight_timer = 0.0f;

    game->last_flip_row = -1;
    game->last_flip_col = -1;
}

int screen_to_board(int mouseX, int mouseY, int *row, int *col)
{
    int localX = mouseX - BOARD_OFFSET_X;
    int localY = mouseY - BOARD_OFFSET_Y;

    if (localX < 0 || localY < 0) {
        return 0;
    }

    *col = localX / CELL_SIZE;
    *row = localY / CELL_SIZE;

    if (!is_valid_position(*row, *col)) {
        return 0;
    }

    return 1;
}

void handle_player_click(Game *game, int mouseX, int mouseY)
{
    if (game->game_over) {
        return;
    }

    if (game->turn != TURN_PLAYER) {
        return;
    }

    int row, col;
    if (!screen_to_board(mouseX, mouseY, &row, &col)) {
        return;
    }

    if (game->board.cells[row][col].revealed) {
        return;
    }

    if (flip_piece(&game->board, row, col)) {
        game->last_flip_row = row;
        game->last_flip_col = col;

        if (count_unrevealed(&game->board) == 0) {
            game->game_over = 1;
            return;
        }

        game->turn = TURN_COMPUTER_THINKING;
        game->ai_delay_timer = AI_THINK_DELAY;
    }
}

void update_game(Game *game)
{
    if (game->game_over) {
        return;
    }

    if (game->turn == TURN_COMPUTER_THINKING) {
        game->ai_delay_timer -= GetFrameTime();

        if (game->ai_delay_timer <= 0.0f) {
            int ai_row = -1;
            int ai_col = -1;

            ai_flip_random(&game->board, &ai_row, &ai_col);

            game->last_flip_row = ai_row;
            game->last_flip_col = ai_col;

            if (count_unrevealed(&game->board) == 0) {
                game->game_over = 1;
                return;
            }

            game->turn = TURN_COMPUTER_SHOWING;
            game->highlight_timer = AI_SHOW_DELAY;
        }
    }
    else if (game->turn == TURN_COMPUTER_SHOWING) {
        game->highlight_timer -= GetFrameTime();

        if (game->highlight_timer <= 0.0f) {
            game->turn = TURN_PLAYER;
        }
    }
}
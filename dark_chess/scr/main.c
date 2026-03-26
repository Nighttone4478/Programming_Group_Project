#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "piece.h"
#include "board.h"
#include "game.h"
#include "input.h"

int main(void)
{
    Piece board[ROWS][COLS];
    GameState state = PLAYER_TURN;

    srand((unsigned)time(NULL));
    setup_board(board);

    while (1) {
        clear_screen();
        draw_board_grid();
        draw_board_pieces(board);

        if (all_revealed(board)) {
            state = GAME_OVER;
        }

        if (state == PLAYER_TURN) {
            int row, col;

            draw_message("Player Turn");

            if (!get_player_input(&row, &col)) {
                printf("Input error.\n");
                break;
            }

            if (!player_flip(board, row, col)) {
                printf("Invalid move. Press Enter to continue...");
                getchar();
                getchar();
                continue;
            }

            if (all_revealed(board)) {
                state = GAME_OVER;
            } else {
                state = COMPUTER_TURN;
            }
        }
        else if (state == COMPUTER_TURN) {
            int row, col;

            draw_message("Computer Turn");

            if (computer_flip(board, &row, &col)) {
                printf("Computer flipped: (%d, %d)\n", row, col);
                printf("Press Enter to continue...");
                getchar();
                getchar();
            }

            if (all_revealed(board)) {
                state = GAME_OVER;
            } else {
                state = PLAYER_TURN;
            }
        }
        else if (state == GAME_OVER) {
            clear_screen();
            draw_board_grid();
            draw_board_pieces(board);
            draw_message("Game Over");
            break;
        }
    }

    return 0;
}
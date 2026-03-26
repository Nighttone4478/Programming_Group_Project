#include <stdio.h>
#include "board.h"
#include "piece.h"

void clear_screen(void)
{
    // Windows 可用 system("cls");
    // 這裡先用簡單換行避免不同環境出問題
    for (int i = 0; i < 30; i++) {
        printf("\n");
    }
}

void draw_board_grid(void)
{
    printf("========== DARK CHESS ==========\n");
    printf("Board Size: %d x %d\n\n", ROWS, COLS);
}

void draw_board_pieces(Piece board[ROWS][COLS])
{
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (board[r][c].revealed == 0) {
                printf("[X]");
            } else {
                printf("[%s]", get_piece_name(board[r][c]));
            }
        }
        printf("\n");
    }
    printf("\n");
}

void draw_message(const char *msg)
{
    printf("%s\n\n", msg);
}
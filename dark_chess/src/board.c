#include "board.h"
#include <stdlib.h>

static void swap_piece(Piece *a, Piece *b)
{
    Piece temp = *a;
    *a = *b;
    *b = temp;
}

int is_valid_position(int row, int col)
{
    return row >= 0 && row < BOARD_ROWS && col >= 0 && col < BOARD_COLS;
}

void init_board(Board *board)
{
    Piece pool[TOTAL_CELLS];
    int index = 0;

    PieceSide sides[2] = { SIDE_RED, SIDE_BLACK };

    for (int s = 0; s < 2; s++) {
        PieceSide side = sides[s];

        pool[index++] = (Piece){ side, KING, 0 };

        for (int i = 0; i < 2; i++) pool[index++] = (Piece){ side, ADVISOR, 0 };
        for (int i = 0; i < 2; i++) pool[index++] = (Piece){ side, ELEPHANT, 0 };
        for (int i = 0; i < 2; i++) pool[index++] = (Piece){ side, ROOK, 0 };
        for (int i = 0; i < 2; i++) pool[index++] = (Piece){ side, KNIGHT, 0 };
        for (int i = 0; i < 2; i++) pool[index++] = (Piece){ side, CANNON, 0 };
        for (int i = 0; i < 5; i++) pool[index++] = (Piece){ side, PAWN, 0 };
    }

    for (int i = TOTAL_CELLS - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        swap_piece(&pool[i], &pool[j]);
    }

    index = 0;
    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            board->cells[r][c] = pool[index++];
        }
    }
}

int flip_piece(Board *board, int row, int col)
{
    if (!is_valid_position(row, col)) {
        return 0;
    }

    if (board->cells[row][col].revealed) {
        return 0;
    }

    board->cells[row][col].revealed = 1;
    return 1;
}

int count_unrevealed(const Board *board)
{
    int count = 0;

    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            if (!board->cells[r][c].revealed) {
                count++;
            }
        }
    }

    return count;
}
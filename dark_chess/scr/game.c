#include <stdlib.h>
#include "game.h"
#include "piece.h"

void setup_board(Piece board[ROWS][COLS])
{
    Piece pieces[TOTAL_PIECES];
    init_all_pieces(pieces);
    shuffle_pieces(pieces);

    int index = 0;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            board[r][c] = pieces[index++];
            board[r][c].revealed = 0;
        }
    }
}

int player_flip(Piece board[ROWS][COLS], int row, int col)
{
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS) {
        return 0;
    }

    if (board[row][col].revealed == 1) {
        return 0;
    }

    board[row][col].revealed = 1;
    return 1;
}

int computer_flip(Piece board[ROWS][COLS], int *out_row, int *out_col)
{
    int hidden[TOTAL_PIECES][2];
    int count = 0;

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (board[r][c].revealed == 0) {
                hidden[count][0] = r;
                hidden[count][1] = c;
                count++;
            }
        }
    }

    if (count == 0) {
        return 0;
    }

    int pick = rand() % count;
    int row = hidden[pick][0];
    int col = hidden[pick][1];

    board[row][col].revealed = 1;

    if (out_row != NULL) *out_row = row;
    if (out_col != NULL) *out_col = col;

    return 1;
}

int all_revealed(Piece board[ROWS][COLS])
{
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (board[r][c].revealed == 0) {
                return 0;
            }
        }
    }
    return 1;
}
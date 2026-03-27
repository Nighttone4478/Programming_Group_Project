#include "ai.h"
#include <stdlib.h>

void ai_flip_random(Board *board, int *out_row, int *out_col)
{
    int hidden_positions[TOTAL_CELLS][2];
    int count = 0;

    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            if (!board->cells[r][c].revealed) {
                hidden_positions[count][0] = r;
                hidden_positions[count][1] = c;
                count++;
            }
        }
    }

    if (count == 0) {
        if (out_row) *out_row = -1;
        if (out_col) *out_col = -1;
        return;
    }

    int pick = rand() % count;
    int row = hidden_positions[pick][0];
    int col = hidden_positions[pick][1];

    board->cells[row][col].revealed = 1;

    if (out_row) *out_row = row;
    if (out_col) *out_col = col;
}
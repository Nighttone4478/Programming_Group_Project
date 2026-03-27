#ifndef BOARD_H
#define BOARD_H

#include "piece.h"

#define BOARD_ROWS 4
#define BOARD_COLS 8
#define TOTAL_CELLS 32

typedef struct {
    Piece cells[BOARD_ROWS][BOARD_COLS];
} Board;

void init_board(Board *board);
int flip_piece(Board *board, int row, int col);
int count_unrevealed(const Board *board);
int is_valid_position(int row, int col);

#endif
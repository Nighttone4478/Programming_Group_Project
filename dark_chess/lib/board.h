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

int is_adjacent_orthogonal(int r1, int c1, int r2, int c2);
int can_move_piece(const Board *board, int fromRow, int fromCol, int toRow, int toCol);
int can_capture_piece(const Board *board, int fromRow, int fromCol, int toRow, int toCol);

void move_piece(Board *board, int fromRow, int fromCol, int toRow, int toCol);
void capture_piece(Board *board, int fromRow, int fromCol, int toRow, int toCol);

int has_any_legal_action(const Board *board, PieceSide side);
int board_material_score(const Board *board, PieceSide side);

#endif
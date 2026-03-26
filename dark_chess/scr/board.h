#ifndef BOARD_H
#define BOARD_H

#include "piece.h"

#define ROWS 4
#define COLS 8

void draw_board_grid(void);
void draw_board_pieces(Piece board[ROWS][COLS]);
void draw_message(const char *msg);
void clear_screen(void);

#endif
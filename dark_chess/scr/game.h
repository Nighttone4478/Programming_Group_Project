#ifndef GAME_H
#define GAME_H

#include "board.h"
#include "piece.h"

typedef enum {
    PLAYER_TURN,
    COMPUTER_TURN,
    GAME_OVER
} GameState;

void setup_board(Piece board[ROWS][COLS]);
int player_flip(Piece board[ROWS][COLS], int row, int col);
int computer_flip(Piece board[ROWS][COLS], int *out_row, int *out_col);
int all_revealed(Piece board[ROWS][COLS]);

#endif
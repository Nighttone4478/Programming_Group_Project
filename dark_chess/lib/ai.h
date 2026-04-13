#ifndef AI_H
#define AI_H

#include "board.h"

typedef enum {
    AI_ACTION_NONE,
    AI_ACTION_FLIP,
    AI_ACTION_MOVE,
    AI_ACTION_CAPTURE
} AIActionType;

typedef struct {
    AIActionType action;
    int from_row;
    int from_col;
    int to_row;
    int to_col;
} AIMove;

int ai_choose_move(const Board *board, PieceSide aiSide, PieceSide enemySide, AIMove *out_move);

#endif
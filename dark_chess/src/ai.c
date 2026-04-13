#include "ai.h"
#include <stdlib.h>

static int is_square_dangerous(const Board *board, int row, int col, PieceSide enemySide)
{
    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            Piece enemy = board->cells[r][c];
            if (is_empty_piece(enemy) || !enemy.revealed || enemy.side != enemySide) {
                continue;
            }

            if (can_capture_piece(board, r, c, row, col)) {
                return 1;
            }
        }
    }
    return 0;
}

static void apply_ai_move(Board *board, AIMove move)
{
    if (move.action == AI_ACTION_MOVE) {
        move_piece(board, move.from_row, move.from_col, move.to_row, move.to_col);
    } else if (move.action == AI_ACTION_CAPTURE) {
        capture_piece(board, move.from_row, move.from_col, move.to_row, move.to_col);
    } else if (move.action == AI_ACTION_FLIP) {
        flip_piece(board, move.to_row, move.to_col);
    }
}

static int capture_priority(PieceType type)
{
    switch (type) {
        case KING:     return 100;
        case ROOK:     return 80;
        case CANNON:   return 75;
        case KNIGHT:   return 70;
        case ADVISOR:  return 60;
        case ELEPHANT: return 55;
        case PAWN:     return 40;
        default:       return 0;
    }
}
int ai_choose_move(const Board *board, PieceSide aiSide, PieceSide enemySide, AIMove *out_move)
{
    AIMove bestCapture = { AI_ACTION_NONE, -1, -1, -1, -1 };
    int bestCaptureScore = -100000;

    AIMove bestSafeMove = { AI_ACTION_NONE, -1, -1, -1, -1 };
    int foundSafeMove = 0;

    AIMove bestAnyMove = { AI_ACTION_NONE, -1, -1, -1, -1 };
    int foundAnyMove = 0;

    int hidden[TOTAL_CELLS][2];
    int hiddenCount = 0;

    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            Piece p = board->cells[r][c];

            if (!is_empty_piece(p) && !p.revealed) {
                hidden[hiddenCount][0] = r;
                hidden[hiddenCount][1] = c;
                hiddenCount++;
            }

            if (is_empty_piece(p) || !p.revealed || p.side != aiSide) {
                continue;
            }

            for (int rr = 0; rr < BOARD_ROWS; rr++) {
                for (int cc = 0; cc < BOARD_COLS; cc++) {
                    if (can_capture_piece(board, r, c, rr, cc)) {
                        Board temp = *board;
                        AIMove candidate = { AI_ACTION_CAPTURE, r, c, rr, cc };
                        apply_ai_move(&temp, candidate);

                        int danger = is_square_dangerous(&temp, rr, cc, enemySide);
                        int score = capture_priority(board->cells[rr][cc].type);

                        if (!danger) {
                            score += 1000;
                        }

                        if (score > bestCaptureScore) {
                            bestCaptureScore = score;
                            bestCapture = candidate;
                        }
                    }
                    else if (can_move_piece(board, r, c, rr, cc)) {
                        AIMove candidate = { AI_ACTION_MOVE, r, c, rr, cc };

                        if (!foundAnyMove) {
                            bestAnyMove = candidate;
                            foundAnyMove = 1;
                        }

                        Board temp = *board;
                        apply_ai_move(&temp, candidate);

                        if (!is_square_dangerous(&temp, rr, cc, enemySide)) {
                            if (!foundSafeMove) {
                                bestSafeMove = candidate;
                                foundSafeMove = 1;
                            }
                        }
                    }
                }
            }
        }
    }

    // 1. 先吃子
    if (bestCapture.action != AI_ACTION_NONE) {
        *out_move = bestCapture;
        return 1;
    }

    // 2. 只要還有未翻棋，優先翻牌，避免一直來回走
    if (hiddenCount > 0) {
        int pick = rand() % hiddenCount;
        out_move->action = AI_ACTION_FLIP;
        out_move->from_row = -1;
        out_move->from_col = -1;
        out_move->to_row = hidden[pick][0];
        out_move->to_col = hidden[pick][1];
        return 1;
    }

    // 3. 沒棋可翻，再走安全步
    if (foundSafeMove) {
        *out_move = bestSafeMove;
        return 1;
    }

    // 4. 最後才做一般移動
    if (foundAnyMove) {
        *out_move = bestAnyMove;
        return 1;
    }

    out_move->action = AI_ACTION_NONE;
    out_move->from_row = -1;
    out_move->from_col = -1;
    out_move->to_row = -1;
    out_move->to_col = -1;
    return 0;
}
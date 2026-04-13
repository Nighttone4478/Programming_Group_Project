#include "ai.h"
#include <stdlib.h>

#define MAX_AI_MOVES 256
#define SEARCH_DEPTH 2

static int piece_base_value(PieceType type)
{
    switch (type) {
        case KING:     return 1000;
        case ROOK:     return 120;
        case CANNON:   return 110;
        case KNIGHT:   return 90;
        case ADVISOR:  return 70;
        case ELEPHANT: return 65;
        case PAWN:     return 40;
        default:       return 0;
    }
}

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

static int count_legal_moves_for_side(const Board *board, PieceSide side)
{
    int count = 0;

    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            Piece p = board->cells[r][c];
            if (is_empty_piece(p) || !p.revealed || p.side != side) {
                continue;
            }

            for (int rr = 0; rr < BOARD_ROWS; rr++) {
                for (int cc = 0; cc < BOARD_COLS; cc++) {
                    if (can_move_piece(board, r, c, rr, cc) ||
                        can_capture_piece(board, r, c, rr, cc)) {
                        count++;
                    }
                }
            }
        }
    }

    return count;
}

static int count_capture_moves_for_side(const Board *board, PieceSide side)
{
    int count = 0;

    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            Piece p = board->cells[r][c];
            if (is_empty_piece(p) || !p.revealed || p.side != side) {
                continue;
            }

            for (int rr = 0; rr < BOARD_ROWS; rr++) {
                for (int cc = 0; cc < BOARD_COLS; cc++) {
                    if (can_capture_piece(board, r, c, rr, cc)) {
                        count++;
                    }
                }
            }
        }
    }

    return count;
}

static int count_threatened_value(const Board *board, PieceSide side, PieceSide enemySide)
{
    int score = 0;

    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            Piece p = board->cells[r][c];
            if (is_empty_piece(p) || !p.revealed || p.side != side) {
                continue;
            }

            if (is_square_dangerous(board, r, c, enemySide)) {
                int penalty = piece_base_value(p.type) / 2;
                if (p.type == KING || p.type == ROOK || p.type == CANNON) {
                    penalty += 25;
                }
                score += penalty;
            }
        }
    }

    return score;
}

static int center_control_score(const Board *board, PieceSide side)
{
    int score = 0;

    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            Piece p = board->cells[r][c];
            if (is_empty_piece(p) || !p.revealed || p.side != side) {
                continue;
            }

            int centerBonus = 0;

            if ((r == 1 || r == 2) && (c >= 2 && c <= 5)) {
                centerBonus = 10;
            } else if (c >= 1 && c <= 6) {
                centerBonus = 4;
            }

            if (p.type == CANNON) {
                centerBonus += 3;
            }

            score += centerBonus;
        }
    }

    return score;
}

static int cannon_activity_score(const Board *board, PieceSide side)
{
    int score = 0;

    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            Piece p = board->cells[r][c];
            if (is_empty_piece(p) || !p.revealed || p.side != side || p.type != CANNON) {
                continue;
            }

            for (int rr = 0; rr < BOARD_ROWS; rr++) {
                for (int cc = 0; cc < BOARD_COLS; cc++) {
                    if (can_capture_piece(board, r, c, rr, cc)) {
                        score += 18;
                    }
                }
            }
        }
    }

    return score;
}

static int material_score(const Board *board, PieceSide side)
{
    int score = 0;

    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            Piece p = board->cells[r][c];
            if (!is_empty_piece(p) && p.side == side) {
                score += piece_base_value(p.type);
            }
        }
    }

    return score;
}

static int hidden_flip_potential(const Board *board, PieceSide aiSide)
{
    int unrevealed = count_unrevealed(board);
    int aiMaterial = material_score(board, aiSide);

    if (unrevealed == 0) {
        return 0;
    }

    if (aiMaterial < 1300) {
        return unrevealed * 9;
    } else if (aiMaterial < 1600) {
        return unrevealed * 6;
    } else {
        return unrevealed * 3;
    }
}

static int evaluate_board(const Board *board, PieceSide aiSide, PieceSide enemySide)
{
    int aiMaterial = material_score(board, aiSide);
    int enemyMaterial = material_score(board, enemySide);

    int aiMobility = count_legal_moves_for_side(board, aiSide);
    int enemyMobility = count_legal_moves_for_side(board, enemySide);

    int aiCaptures = count_capture_moves_for_side(board, aiSide);
    int enemyCaptures = count_capture_moves_for_side(board, enemySide);

    int aiThreatened = count_threatened_value(board, aiSide, enemySide);
    int enemyThreatened = count_threatened_value(board, enemySide, aiSide);

    int aiCenter = center_control_score(board, aiSide);
    int enemyCenter = center_control_score(board, enemySide);

    int aiCannon = cannon_activity_score(board, aiSide);
    int enemyCannon = cannon_activity_score(board, enemySide);

    int flipPotential = hidden_flip_potential(board, aiSide);

    int score = 0;
    score += (aiMaterial - enemyMaterial) * 10;
    score += (aiMobility - enemyMobility) * 4;
    score += (aiCaptures - enemyCaptures) * 12;
    score -= (aiThreatened - enemyThreatened) * 5;
    score += (aiCenter - enemyCenter) * 3;
    score += (aiCannon - enemyCannon) * 2;
    score += flipPotential;

    return score;
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

static int move_priority(const Board *board, AIMove move, PieceSide selfSide, PieceSide enemySide)
{
    int priority = 0;

    if (move.action == AI_ACTION_CAPTURE) {
        Piece attacker = board->cells[move.from_row][move.from_col];
        Piece defender = board->cells[move.to_row][move.to_col];

        priority += 10000;
        priority += piece_base_value(defender.type) * 8;
        priority -= piece_base_value(attacker.type) * 2;

        Board temp = *board;
        apply_ai_move(&temp, move);
        if (!is_square_dangerous(&temp, move.to_row, move.to_col, enemySide)) {
            priority += 5000;
        } else {
            priority -= 1500;
        }
    }
    else if (move.action == AI_ACTION_FLIP) {
        priority += 4000;

        int adjacentKnown = 0;
        int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
        for (int i = 0; i < 4; i++) {
            int rr = move.to_row + dirs[i][0];
            int cc = move.to_col + dirs[i][1];
            if (is_valid_position(rr, cc) && !is_empty_piece(board->cells[rr][cc]) && board->cells[rr][cc].revealed) {
                adjacentKnown++;
            }
        }

        priority += adjacentKnown * 60;
    }
    else if (move.action == AI_ACTION_MOVE) {
        priority += 1000;

        Piece mover = board->cells[move.from_row][move.from_col];
        Board temp = *board;
        apply_ai_move(&temp, move);

        if (!is_square_dangerous(&temp, move.to_row, move.to_col, enemySide)) {
            priority += 1000;
        }

        if ((move.to_row == 1 || move.to_row == 2) && (move.to_col >= 2 && move.to_col <= 5)) {
            priority += 120;
        }

        if (mover.type == CANNON) {
            priority += 80;
        }
    }

    return priority;
}

static void sort_moves(const Board *board, AIMove *moves, int count, PieceSide selfSide, PieceSide enemySide)
{
    for (int i = 0; i < count; i++) {
        int best = i;
        int bestScore = move_priority(board, moves[i], selfSide, enemySide);

        for (int j = i + 1; j < count; j++) {
            int score = move_priority(board, moves[j], selfSide, enemySide);
            if (score > bestScore) {
                best = j;
                bestScore = score;
            }
        }

        if (best != i) {
            AIMove temp = moves[i];
            moves[i] = moves[best];
            moves[best] = temp;
        }
    }
}

static int generate_all_moves(const Board *board, PieceSide side, AIMove *moves, int max_moves)
{
    int count = 0;

    // 1. 所有吃子/移動
    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            Piece p = board->cells[r][c];
            if (is_empty_piece(p) || !p.revealed || p.side != side) {
                continue;
            }

            for (int rr = 0; rr < BOARD_ROWS; rr++) {
                for (int cc = 0; cc < BOARD_COLS; cc++) {
                    if (count >= max_moves) {
                        return count;
                    }

                    if (can_capture_piece(board, r, c, rr, cc)) {
                        moves[count++] = (AIMove){ AI_ACTION_CAPTURE, r, c, rr, cc };
                    } else if (can_move_piece(board, r, c, rr, cc)) {
                        moves[count++] = (AIMove){ AI_ACTION_MOVE, r, c, rr, cc };
                    }
                }
            }
        }
    }

    // 2. 翻牌動作
    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            if (count >= max_moves) {
                return count;
            }

            Piece p = board->cells[r][c];
            if (!is_empty_piece(p) && !p.revealed) {
                moves[count++] = (AIMove){ AI_ACTION_FLIP, -1, -1, r, c };
            }
        }
    }

    return count;
}

static int minimax(
    const Board *board,
    int depth,
    int alpha,
    int beta,
    int maximizingPlayer,
    PieceSide aiSide,
    PieceSide enemySide
)
{
    if (depth == 0) {
        return evaluate_board(board, aiSide, enemySide);
    }

    PieceSide currentSide = maximizingPlayer ? aiSide : enemySide;
    PieceSide oppositeSide = maximizingPlayer ? enemySide : aiSide;

    AIMove moves[MAX_AI_MOVES];
    int moveCount = generate_all_moves(board, currentSide, moves, MAX_AI_MOVES);

    if (moveCount == 0) {
        return evaluate_board(board, aiSide, enemySide);
    }

    sort_moves(board, moves, moveCount, currentSide, oppositeSide);

    if (maximizingPlayer) {
        int bestValue = -1000000000;

        for (int i = 0; i < moveCount; i++) {
            Board temp = *board;
            apply_ai_move(&temp, moves[i]);

            int value = minimax(&temp, depth - 1, alpha, beta, 0, aiSide, enemySide);
            if (value > bestValue) {
                bestValue = value;
            }
            if (value > alpha) {
                alpha = value;
            }
            if (beta <= alpha) {
                break;
            }
        }

        return bestValue;
    } else {
        int bestValue = 1000000000;

        for (int i = 0; i < moveCount; i++) {
            Board temp = *board;
            apply_ai_move(&temp, moves[i]);

            int value = minimax(&temp, depth - 1, alpha, beta, 1, aiSide, enemySide);
            if (value < bestValue) {
                bestValue = value;
            }
            if (value < beta) {
                beta = value;
            }
            if (beta <= alpha) {
                break;
            }
        }

        return bestValue;
    }
}

int ai_choose_move(const Board *board, PieceSide aiSide, PieceSide enemySide, AIMove *out_move)
{
    AIMove moves[MAX_AI_MOVES];
    int moveCount = generate_all_moves(board, aiSide, moves, MAX_AI_MOVES);

    if (moveCount == 0) {
        out_move->action = AI_ACTION_NONE;
        out_move->from_row = -1;
        out_move->from_col = -1;
        out_move->to_row = -1;
        out_move->to_col = -1;
        return 0;
    }

    sort_moves(board, moves, moveCount, aiSide, enemySide);

    int bestScore = -1000000000;
    int bestIndex = 0;

    for (int i = 0; i < moveCount; i++) {
        Board temp = *board;
        apply_ai_move(&temp, moves[i]);

        int score = minimax(
            &temp,
            SEARCH_DEPTH - 1,
            -1000000000,
            1000000000,
            0,
            aiSide,
            enemySide
        );

        // 額外根據當前動作再做微調，讓行為更像人
        if (moves[i].action == AI_ACTION_CAPTURE) {
            Piece defender = board->cells[moves[i].to_row][moves[i].to_col];
            score += piece_base_value(defender.type) * 3;
        } else if (moves[i].action == AI_ACTION_FLIP) {
            int unrevealed = count_unrevealed(board);
            if (unrevealed > 8) {
                score += 150;
            } else if (unrevealed > 3) {
                score += 60;
            }
        } else if (moves[i].action == AI_ACTION_MOVE) {
            Board temp2 = *board;
            apply_ai_move(&temp2, moves[i]);
            if (is_square_dangerous(&temp2, moves[i].to_row, moves[i].to_col, enemySide)) {
                Piece mover = board->cells[moves[i].from_row][moves[i].from_col];
                score -= piece_base_value(mover.type);
            }
        }

        if (score > bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }

    *out_move = moves[bestIndex];
    return 1;
}
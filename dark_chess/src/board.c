#include "board.h"
#include <stdlib.h>

static void swap_piece(Piece *a, Piece *b)
{
    Piece temp = *a;
    *a = *b;
    *b = temp;
}

static int count_between_on_line(const Board *board, int r1, int c1, int r2, int c2)
{
    int count = 0;

    if (r1 == r2) {
        int start = (c1 < c2) ? c1 + 1 : c2 + 1;
        int end = (c1 < c2) ? c2 : c1;
        for (int c = start; c < end; c++) {
            if (!is_empty_piece(board->cells[r1][c])) {
                count++;
            }
        }
        return count;
    }

    if (c1 == c2) {
        int start = (r1 < r2) ? r1 + 1 : r2 + 1;
        int end = (r1 < r2) ? r2 : r1;
        for (int r = start; r < end; r++) {
            if (!is_empty_piece(board->cells[r][c1])) {
                count++;
            }
        }
        return count;
    }

    return -1;
}

static int normal_piece_can_capture(Piece attacker, Piece defender)
{
    if (is_empty_piece(attacker) || is_empty_piece(defender)) {
        return 0;
    }

    if (!attacker.revealed || !defender.revealed) {
        return 0;
    }

    if (attacker.side == defender.side) {
        return 0;
    }

    if (attacker.type == CANNON) {
        return 0;
    }

    if (attacker.type == KING && defender.type == PAWN) {
        return 0;
    }

    if (attacker.type == PAWN && defender.type == KING) {
        return 1;
    }

    return get_piece_rank(attacker.type) >= get_piece_rank(defender.type);
}

int is_valid_position(int row, int col)
{
    return row >= 0 && row < BOARD_ROWS && col >= 0 && col < BOARD_COLS;
}

int is_adjacent_orthogonal(int r1, int c1, int r2, int c2)
{
    int dr = r1 - r2;
    if (dr < 0) dr = -dr;
    int dc = c1 - c2;
    if (dc < 0) dc = -dc;
    return (dr + dc) == 1;
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

    if (is_empty_piece(board->cells[row][col])) {
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
            if (!is_empty_piece(board->cells[r][c]) && !board->cells[r][c].revealed) {
                count++;
            }
        }
    }

    return count;
}

int can_move_piece(const Board *board, int fromRow, int fromCol, int toRow, int toCol)
{
    if (!is_valid_position(fromRow, fromCol) || !is_valid_position(toRow, toCol)) {
        return 0;
    }

    Piece from = board->cells[fromRow][fromCol];
    Piece to = board->cells[toRow][toCol];

    if (is_empty_piece(from) || !from.revealed) {
        return 0;
    }

    if (!is_empty_piece(to)) {
        return 0;
    }

    if (from.type == CANNON) {
        return is_adjacent_orthogonal(fromRow, fromCol, toRow, toCol);
    }

    return is_adjacent_orthogonal(fromRow, fromCol, toRow, toCol);
}

int can_capture_piece(const Board *board, int fromRow, int fromCol, int toRow, int toCol)
{
    if (!is_valid_position(fromRow, fromCol) || !is_valid_position(toRow, toCol)) {
        return 0;
    }

    Piece attacker = board->cells[fromRow][fromCol];
    Piece defender = board->cells[toRow][toCol];

    if (is_empty_piece(attacker) || is_empty_piece(defender)) {
        return 0;
    }

    if (!attacker.revealed || !defender.revealed) {
        return 0;
    }

    if (attacker.side == defender.side) {
        return 0;
    }

    if (attacker.type == CANNON) {
        if (!(fromRow == toRow || fromCol == toCol)) {
            return 0;
        }
        return count_between_on_line(board, fromRow, fromCol, toRow, toCol) == 1;
    }

    if (!is_adjacent_orthogonal(fromRow, fromCol, toRow, toCol)) {
        return 0;
    }

    return normal_piece_can_capture(attacker, defender);
}

void move_piece(Board *board, int fromRow, int fromCol, int toRow, int toCol)
{
    board->cells[toRow][toCol] = board->cells[fromRow][fromCol];
    board->cells[fromRow][fromCol] = (Piece){ SIDE_NONE, PIECE_EMPTY, 1 };
}

void capture_piece(Board *board, int fromRow, int fromCol, int toRow, int toCol)
{
    board->cells[toRow][toCol] = board->cells[fromRow][fromCol];
    board->cells[fromRow][fromCol] = (Piece){ SIDE_NONE, PIECE_EMPTY, 1 };
}

int has_any_legal_action(const Board *board, PieceSide side)
{
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
                        return 1;
                    }
                }
            }
        }
    }

    return count_unrevealed(board) > 0;
}

int board_material_score(const Board *board, PieceSide side)
{
    int score = 0;

    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            Piece p = board->cells[r][c];
            if (!is_empty_piece(p) && p.side == side) {
                score += get_piece_score(p.type);
            }
        }
    }

    return score;
}
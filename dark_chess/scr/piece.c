#include <stdlib.h>
#include "piece.h"

static void swap_piece(Piece *a, Piece *b)
{
    Piece temp = *a;
    *a = *b;
    *b = temp;
}

void init_all_pieces(Piece pieces[TOTAL_PIECES])
{
    int index = 0;

    Side sides[2] = {BLACK, RED};

    for (int s = 0; s < 2; s++) {
        Side side = sides[s];

        pieces[index++] = (Piece){KING, side, 0};

        for (int i = 0; i < 2; i++) pieces[index++] = (Piece){GUARD, side, 0};
        for (int i = 0; i < 2; i++) pieces[index++] = (Piece){MINISTER, side, 0};
        for (int i = 0; i < 2; i++) pieces[index++] = (Piece){ROOK, side, 0};
        for (int i = 0; i < 2; i++) pieces[index++] = (Piece){KNIGHT, side, 0};
        for (int i = 0; i < 2; i++) pieces[index++] = (Piece){CANNON, side, 0};
        for (int i = 0; i < 5; i++) pieces[index++] = (Piece){PAWN, side, 0};
    }
}

void shuffle_pieces(Piece pieces[TOTAL_PIECES])
{
    for (int i = TOTAL_PIECES - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        swap_piece(&pieces[i], &pieces[j]);
    }
}

const char* get_piece_name(Piece piece)
{
    if (piece.side == BLACK) {
        switch (piece.type) {
            case KING:     return "將";
            case GUARD:    return "士";
            case MINISTER: return "象";
            case ROOK:     return "車";
            case KNIGHT:   return "馬";
            case CANNON:   return "包";
            case PAWN:     return "卒";
            default:       return "?";
        }
    } else {
        switch (piece.type) {
            case KING:     return "帥";
            case GUARD:    return "仕";
            case MINISTER: return "相";
            case ROOK:     return "俥";
            case KNIGHT:   return "傌";
            case CANNON:   return "炮";
            case PAWN:     return "兵";
            default:       return "?";
        }
    }
}

const char* get_side_name(Side side)
{
    return (side == BLACK) ? "Black" : "Red";
}
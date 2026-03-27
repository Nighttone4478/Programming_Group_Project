#include "piece.h"

const char *get_piece_code(Piece piece)
{
    if (piece.side == SIDE_RED) {
        switch (piece.type) {
            case KING:     return "帥";
            case ADVISOR:  return "仕";
            case ELEPHANT: return "相";
            case ROOK:     return "俥";
            case KNIGHT:   return "傌";
            case CANNON:   return "炮";
            case PAWN:     return "兵";
            default:       return "?";
        }
    } else {
        switch (piece.type) {
            case KING:     return "將";
            case ADVISOR:  return "士";
            case ELEPHANT: return "象";
            case ROOK:     return "車";
            case KNIGHT:   return "馬";
            case CANNON:   return "包";
            case PAWN:     return "卒";
            default:       return "?";
        }
    }
}
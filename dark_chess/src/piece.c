#include "piece.h"

const char *get_piece_code(Piece piece)
{
    if (piece.type == PIECE_EMPTY || piece.side == SIDE_NONE) {
        return "";
    }

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

int is_empty_piece(Piece piece)
{
    return piece.type == PIECE_EMPTY || piece.side == SIDE_NONE;
}

int get_piece_rank(PieceType type)
{
    switch (type) {
        case KING:     return 7;
        case ADVISOR:  return 6;
        case ELEPHANT: return 5;
        case ROOK:     return 4;
        case KNIGHT:   return 3;
        case CANNON:   return 2;
        case PAWN:     return 1;
        default:       return 0;
    }
}

int get_piece_score(PieceType type)
{
    switch (type) {
        case KING:     return 30;
        case ADVISOR:  return 10;
        case ELEPHANT: return 10;
        case ROOK:     return 14;
        case KNIGHT:   return 12;
        case CANNON:   return 12;
        case PAWN:     return 5;
        default:       return 0;
    }
}
#ifndef PIECE_H
#define PIECE_H

typedef enum {
    SIDE_RED,
    SIDE_BLACK
} PieceSide;

typedef enum {
    KING,
    ADVISOR,
    ELEPHANT,
    ROOK,
    KNIGHT,
    CANNON,
    PAWN
} PieceType;

typedef struct {
    PieceSide side;
    PieceType type;
    int revealed;   // 0 = 背面朝上, 1 = 已翻開
} Piece;

const char *get_piece_code(Piece piece);

#endif
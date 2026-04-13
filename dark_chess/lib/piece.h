#ifndef PIECE_H
#define PIECE_H

typedef enum {
    SIDE_NONE = -1,
    SIDE_RED,
    SIDE_BLACK
} PieceSide;

typedef enum {
    PIECE_EMPTY = -1,
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
int is_empty_piece(Piece piece);
int get_piece_rank(PieceType type);
int get_piece_score(PieceType type);

#endif
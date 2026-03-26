#ifndef PIECE_H
#define PIECE_H

#define TOTAL_PIECES 32 //棋子數

typedef enum {
    KING,  //將、帥
    GUARD, //士
    MINISTER, //象
    ROOK, //車
    KNIGHT, //馬
    CANNON, //炮
    PAWN  //卒
} PieceType;

typedef enum {
    BLACK,
    RED
} Side;

typedef struct {
    PieceType type;
    Side side;
    int revealed;
} Piece;

void init_all_pieces(Piece pieces[TOTAL_PIECES]);
void shuffle_pieces(Piece pieces[TOTAL_PIECES]);
const char* get_piece_name(Piece piece);

#endif
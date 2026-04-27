#include "dark_chess_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* =========================================================
   Config
========================================================= */

#define ROWS 4
#define COLS 8
#define N 32

#define PIECE_TYPES 14
#define MAX_DUP_PER_TYPE 5

#define MAX_MOVES 256
#define MAX_Q_DEPTH 8

#define SEARCH_DEPTH_OPENING 4
#define SEARCH_DEPTH_MIDGAME 5
#define SEARCH_DEPTH_REVEALED 5
#define SEARCH_DEPTH_ENDGAME 6

#define ENDGAME_VISIBLE_THRESHOLD 4

#define ALPHABETA_BRANCH_LIMIT 10
#define CAPTURE_BRANCH_LIMIT 8
#define FLIP_CANDIDATE_LIMIT 4

#define TT_SIZE 262144
#define PATH_SIZE 128
#define PV_MAX_LEN 16

#define TIME_LIMIT_MS 850
#define ACTION_DELAY_MS 1000

/*
   Offensive style
*/
#define CAPTURE_BASE_BONUS 500
#define SAME_RANK_CAPTURE_BONUS 120
#define DANGER_PENALTY_RATIO 0.45
#define ATTACK_BONUS_RATIO 0.55
#define SELF_DANGER_RATIO 0.20

#define FLIP_SCORE_SCALE 1000.0
#define FLIP_MARGIN_IF_CAPTURE 999999.0
#define FLIP_MARGIN_IF_MOVE 800.0

#define HIGH_VALUE_COVERED_PENALTY 10
#define KING_SOLDIER_DANGER_PENALTY 350

/*
   Endgame positional weights
*/
#define ENDGAME_COVERED_LIMIT 4
#define ENDGAME_VISIBLE_LIMIT 8
#define ENDGAME_MOVE_POSITIONAL_WEIGHT 0.08
#define ENDGAME_CAPTURE_POSITIONAL_WEIGHT 0.10

#define TT_EXACT 0
#define TT_LOWER 1
#define TT_UPPER 2

#define STATE_COVERED 14
#define STATE_EMPTY 15
#define STATE_COUNT 16

/* =========================================================
   Types
========================================================= */

typedef enum {
    EMPTY = -1,

    RED_KING = 0,
    RED_GUARD,
    RED_ELEPHANT,
    RED_CAR,
    RED_HORSE,
    RED_CANNON,
    RED_SOLDIER,

    BLACK_KING,
    BLACK_GUARD,
    BLACK_ELEPHANT,
    BLACK_CAR,
    BLACK_HORSE,
    BLACK_CANNON,
    BLACK_SOLDIER
} Piece;

typedef enum {
    COLOR_NONE = -1,
    COLOR_RED = 0,
    COLOR_BLACK = 1
} Color;

typedef struct {
    int piece;
    int covered;
} Cell;

typedef struct {
    Cell cell[N];

    int piece_count[PIECE_TYPES];
    int piece_pos[PIECE_TYPES][MAX_DUP_PER_TYPE];

    int material_red;
    int material_black;
} Board;

typedef struct {
    int r, c;
    int tr, tc;
    int is_flip;
} Move;

typedef struct {
    unsigned long long key;
    int valid;
    int depth;
    int flag;
    double value;
    Move best_move;
} TTEntry;

/* =========================================================
   Globals
========================================================= */

static const int TOTAL_COUNTS[PIECE_TYPES] = {
    1, 2, 2, 2, 2, 2, 5,
    1, 2, 2, 2, 2, 2, 5
};

static const char* PIECE_NAMES[PIECE_TYPES] = {
    "Red_King",
    "Red_Guard",
    "Red_Elephant",
    "Red_Car",
    "Red_Horse",
    "Red_Cannon",
    "Red_Soldier",

    "Black_King",
    "Black_Guard",
    "Black_Elephant",
    "Black_Car",
    "Black_Horse",
    "Black_Cannon",
    "Black_Soldier"
};

static TTEntry tt[TT_SIZE];

static int MoveTab[N][5];
static int OrderTab[PIECE_TYPES][PIECE_TYPES];

static unsigned long long Zobrist[STATE_COUNT][N];
static unsigned long long ZobristTurn[2];

static int dead_counts[PIECE_TYPES] = {0};
static Board previous_visible_board;
static int has_previous_visible_board = 0;

static unsigned long long path_keys[PATH_SIZE];
static int path_len = 0;

static clock_t search_start_time;
static int search_timeout = 0;

/* =========================================================
   Search timer
========================================================= */

void search_timer_start(void) {
    search_start_time = clock();
    search_timeout = 0;
}

int time_is_up(void) {
    clock_t now = clock();
    double elapsed_ms = (double)(now - search_start_time) * 1000.0 / CLOCKS_PER_SEC;

    if (elapsed_ms >= TIME_LIMIT_MS) {
        search_timeout = 1;
        return 1;
    }

    return 0;
}

/* =========================================================
   Utility
========================================================= */

int in_board(int r, int c) {
    return r >= 0 && r < ROWS && c >= 0 && c < COLS;
}

int idx_of(int r, int c) {
    return r * COLS + c;
}

Color opposite_color(Color c) {
    if (c == COLOR_RED) return COLOR_BLACK;
    if (c == COLOR_BLACK) return COLOR_RED;
    return COLOR_NONE;
}

int same_move(Move a, Move b) {
    return a.r == b.r &&
           a.c == b.c &&
           a.tr == b.tr &&
           a.tc == b.tc &&
           a.is_flip == b.is_flip;
}

int manhattan_distance(int a, int b) {
    int ar = a / COLS;
    int ac = a % COLS;
    int br = b / COLS;
    int bc = b % COLS;

    int dr = ar - br;
    int dc = ac - bc;

    if (dr < 0) dr = -dr;
    if (dc < 0) dc = -dc;

    return dr + dc;
}

/* =========================================================
   Piece helpers
========================================================= */

int str_to_piece(const char* s) {
    for (int i = 0; i < PIECE_TYPES; i++) {
        if (strcmp(s, PIECE_NAMES[i]) == 0) {
            return i;
        }
    }

    return EMPTY;
}

Color str_to_color(const char* s) {
    if (strcmp(s, "Red") == 0) return COLOR_RED;
    if (strcmp(s, "Black") == 0) return COLOR_BLACK;
    return COLOR_NONE;
}

Color piece_color(int p) {
    if (p >= RED_KING && p <= RED_SOLDIER) return COLOR_RED;
    if (p >= BLACK_KING && p <= BLACK_SOLDIER) return COLOR_BLACK;
    return COLOR_NONE;
}

int is_cannon(int p) {
    return p == RED_CANNON || p == BLACK_CANNON;
}

int is_soldier(int p) {
    return p == RED_SOLDIER || p == BLACK_SOLDIER;
}

int is_king(int p) {
    return p == RED_KING || p == BLACK_KING;
}

int piece_rank(int p) {
    switch (p) {
        case RED_KING:
        case BLACK_KING:
            return 7;

        case RED_GUARD:
        case BLACK_GUARD:
            return 6;

        case RED_ELEPHANT:
        case BLACK_ELEPHANT:
            return 5;

        case RED_CAR:
        case BLACK_CAR:
            return 4;

        case RED_HORSE:
        case BLACK_HORSE:
            return 3;

        case RED_CANNON:
        case BLACK_CANNON:
            return 2;

        case RED_SOLDIER:
        case BLACK_SOLDIER:
            return 1;
    }

    return 0;
}

int piece_value(int p) {
    switch (p) {
        case RED_KING:
        case BLACK_KING:
            return 810;

        case RED_GUARD:
        case BLACK_GUARD:
            return 270;

        case RED_ELEPHANT:
        case BLACK_ELEPHANT:
            return 90;

        case RED_CAR:
        case BLACK_CAR:
            return 36;

        case RED_CANNON:
        case BLACK_CANNON:
            return 40;

        case RED_HORSE:
        case BLACK_HORSE:
            return 18;

        case RED_SOLDIER:
        case BLACK_SOLDIER:
            return 6;
    }

    return 0;
}

int can_normal_capture_piece(int attacker, int target) {
    if (attacker == EMPTY || target == EMPTY) return 0;
    if (piece_color(attacker) == piece_color(target)) return 0;

    if (is_soldier(attacker) && is_king(target)) return 1;
    if (is_king(attacker) && is_soldier(target)) return 0;

    return piece_rank(attacker) >= piece_rank(target);
}

/* =========================================================
   Precomputed tables
========================================================= */

void init_move_tab(void) {
    for (int pos = 0; pos < N; pos++) {
        int r = pos / COLS;
        int c = pos % COLS;
        int count = 0;

        int dr[4] = {1, -1, 0, 0};
        int dc[4] = {0, 0, 1, -1};

        for (int k = 0; k < 4; k++) {
            int nr = r + dr[k];
            int nc = c + dc[k];

            if (in_board(nr, nc)) {
                count++;
                MoveTab[pos][count] = idx_of(nr, nc);
            }
        }

        MoveTab[pos][0] = count;
    }
}

void init_order_tab(void) {
    for (int a = 0; a < PIECE_TYPES; a++) {
        for (int t = 0; t < PIECE_TYPES; t++) {
            OrderTab[a][t] = can_normal_capture_piece(a, t) ? 1 : 0;
        }
    }
}

/* =========================================================
   Zobrist Hash
========================================================= */

unsigned long long splitmix64(unsigned long long x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

void init_zobrist(void) {
    unsigned long long seed = 0x123456789abcdefULL;

    for (int s = 0; s < STATE_COUNT; s++) {
        for (int pos = 0; pos < N; pos++) {
            seed = splitmix64(seed + (unsigned long long)(s * 131 + pos));
            Zobrist[s][pos] = seed;
        }
    }

    ZobristTurn[COLOR_RED] = splitmix64(seed + 17);
    ZobristTurn[COLOR_BLACK] = splitmix64(seed + 31);
}

int cell_hash_state(Cell cell) {
    if (cell.covered) return STATE_COVERED;
    if (cell.piece == EMPTY) return STATE_EMPTY;
    return cell.piece;
}

unsigned long long board_hash(const Board* b, Color turn_color) {
    unsigned long long h = 0;

    for (int i = 0; i < N; i++) {
        int s = cell_hash_state(b->cell[i]);
        h ^= Zobrist[s][i];
    }

    if (turn_color == COLOR_RED || turn_color == COLOR_BLACK) {
        h ^= ZobristTurn[turn_color];
    }

    return h;
}

/* =========================================================
   Board aux map / material
========================================================= */

void board_rebuild_aux(Board* b) {
    for (int p = 0; p < PIECE_TYPES; p++) {
        b->piece_count[p] = 0;

        for (int k = 0; k < MAX_DUP_PER_TYPE; k++) {
            b->piece_pos[p][k] = -1;
        }
    }

    b->material_red = 0;
    b->material_black = 0;

    for (int i = 0; i < N; i++) {
        Cell cell = b->cell[i];

        if (cell.covered) continue;
        if (cell.piece == EMPTY) continue;
        if (cell.piece < 0 || cell.piece >= PIECE_TYPES) continue;

        int p = cell.piece;
        int k = b->piece_count[p];

        if (k < MAX_DUP_PER_TYPE) {
            b->piece_pos[p][k] = i;
            b->piece_count[p]++;
        }

        if (piece_color(p) == COLOR_RED) {
            b->material_red += piece_value(p);
        } else if (piece_color(p) == COLOR_BLACK) {
            b->material_black += piece_value(p);
        }
    }
}

int material_of_color(const Board* b, Color color) {
    if (color == COLOR_RED) return b->material_red;
    if (color == COLOR_BLACK) return b->material_black;
    return 0;
}

/* =========================================================
   JSON helpers
========================================================= */

void get_piece_at(const char* json, int index, char* out_piece) {
    const char* board_start = strstr(json, "\"board\": [[");

    if (!board_start) {
        strcpy(out_piece, "Unknown");
        return;
    }

    const char* p = board_start + strlen("\"board\": [[");

    for (int i = 0; i <= index; i++) {
        p = strchr(p, '\"');

        if (!p) {
            strcpy(out_piece, "Unknown");
            return;
        }

        p++;

        const char* end = strchr(p, '\"');

        if (!end) {
            strcpy(out_piece, "Unknown");
            return;
        }

        if (i == index) {
            int len = (int)(end - p);

            if (len > 31) len = 31;

            strncpy(out_piece, p, len);
            out_piece[len] = '\0';
            return;
        }

        p = end + 1;
    }

    strcpy(out_piece, "Unknown");
}

void get_role_color(const char* json, const char* role, char* out_color) {
    char key[32];
    sprintf(key, "\"%s\": \"", role);

    const char* p = strstr(json, key);

    if (!p) {
        strcpy(out_color, "None");
        return;
    }

    p += strlen(key);

    const char* end = strchr(p, '\"');

    if (!end) {
        strcpy(out_color, "None");
        return;
    }

    int len = (int)(end - p);

    if (len > 15) len = 15;

    strncpy(out_color, p, len);
    out_color[len] = '\0';
}

int get_total_moves(const char* json) {
    const char* p = strstr(json, "\"total_moves\": ");

    if (!p) return -1;

    int moves = -1;
    sscanf(p + strlen("\"total_moves\": "), "%d", &moves);

    return moves;
}

int get_current_turn_role(const char* json, char* out_role) {
    const char* p = strstr(json, "\"current_turn_role\": \"");

    if (!p) {
        out_role[0] = '\0';
        return 0;
    }

    p += strlen("\"current_turn_role\": \"");

    out_role[0] = p[0];
    out_role[1] = '\0';

    return 1;
}

/* =========================================================
   Board parsing and counting
========================================================= */

void parse_visible_board(const char* json, Board* root) {
    for (int i = 0; i < N; i++) {
        char s[32];
        get_piece_at(json, i, s);

        if (strcmp(s, "Covered") == 0) {
            root->cell[i].piece = EMPTY;
            root->cell[i].covered = 1;
        } else if (strcmp(s, "Null") == 0) {
            root->cell[i].piece = EMPTY;
            root->cell[i].covered = 0;
        } else {
            root->cell[i].piece = str_to_piece(s);
            root->cell[i].covered = 0;
        }
    }

    board_rebuild_aux(root);
}

int count_covered(const Board* b) {
    int count = 0;

    for (int i = 0; i < N; i++) {
        if (b->cell[i].covered) count++;
    }

    return count;
}

int count_visible_live_pieces(const Board* b) {
    int count = 0;

    for (int i = 0; i < N; i++) {
        if (!b->cell[i].covered && b->cell[i].piece != EMPTY) count++;
    }

    return count;
}

int is_endgame_like(const Board* b) {
    int covered = count_covered(b);
    int visible = count_visible_live_pieces(b);

    return covered == 0 ||
           (covered <= ENDGAME_COVERED_LIMIT && visible <= ENDGAME_VISIBLE_LIMIT);
}

void count_visible_pieces_by_type(const Board* b, int counts[PIECE_TYPES]) {
    for (int i = 0; i < PIECE_TYPES; i++) counts[i] = 0;

    for (int i = 0; i < N; i++) {
        Cell cell = b->cell[i];

        if (!cell.covered && cell.piece >= 0 && cell.piece < PIECE_TYPES) {
            counts[cell.piece]++;
        }
    }
}

/* =========================================================
   History tracker
========================================================= */

void update_history_from_previous_board(const Board* current) {
    if (!has_previous_visible_board) {
        previous_visible_board = *current;
        has_previous_visible_board = 1;
        return;
    }

    int prev_counts[PIECE_TYPES];
    int curr_counts[PIECE_TYPES];

    count_visible_pieces_by_type(&previous_visible_board, prev_counts);
    count_visible_pieces_by_type(current, curr_counts);

    for (int p = 0; p < PIECE_TYPES; p++) {
        int disappeared = prev_counts[p] - curr_counts[p];

        if (disappeared > 0) {
            dead_counts[p] += disappeared;

            if (dead_counts[p] > TOTAL_COUNTS[p]) {
                dead_counts[p] = TOTAL_COUNTS[p];
            }

            printf("[History] Captured detected: %s x %d, dead total = %d\n",
                   PIECE_NAMES[p],
                   disappeared,
                   dead_counts[p]);
        }
    }

    previous_visible_board = *current;
}

void print_dead_counts(void) {
    printf("[History] Dead pieces:\n");

    int printed = 0;

    for (int p = 0; p < PIECE_TYPES; p++) {
        if (dead_counts[p] > 0) {
            printf("  %s: %d\n", PIECE_NAMES[p], dead_counts[p]);
            printed = 1;
        }
    }

    if (!printed) printf("  None\n");
}

/* =========================================================
   Remaining pool
========================================================= */

void get_remaining_counts(const Board* b, int remaining[PIECE_TYPES]) {
    int used[PIECE_TYPES] = {0};

    for (int i = 0; i < N; i++) {
        if (!b->cell[i].covered && b->cell[i].piece != EMPTY) {
            int p = b->cell[i].piece;

            if (p >= 0 && p < PIECE_TYPES) used[p]++;
        }
    }

    for (int p = 0; p < PIECE_TYPES; p++) {
        remaining[p] = TOTAL_COUNTS[p] - used[p] - dead_counts[p];

        if (remaining[p] < 0) remaining[p] = 0;
    }
}

int remaining_total_count(const int remaining[PIECE_TYPES]) {
    int total = 0;

    for (int p = 0; p < PIECE_TYPES; p++) total += remaining[p];

    return total;
}

/* =========================================================
   Move generation
========================================================= */

void add_move(Move moves[], int* count, int r, int c, int tr, int tc, int is_flip) {
    if (*count >= MAX_MOVES) return;

    moves[*count].r = r;
    moves[*count].c = c;
    moves[*count].tr = tr;
    moves[*count].tc = tc;
    moves[*count].is_flip = is_flip;

    (*count)++;
}

int count_between(const Board* b, int from, int to) {
    int r = from / COLS;
    int c = from % COLS;
    int tr = to / COLS;
    int tc = to % COLS;

    int count = 0;

    if (r == tr) {
        int step = (tc > c) ? 1 : -1;

        for (int x = c + step; x != tc; x += step) {
            Cell cell = b->cell[idx_of(r, x)];

            if (cell.piece != EMPTY || cell.covered) count++;
        }
    } else if (c == tc) {
        int step = (tr > r) ? 1 : -1;

        for (int y = r + step; y != tr; y += step) {
            Cell cell = b->cell[idx_of(y, c)];

            if (cell.piece != EMPTY || cell.covered) count++;
        }
    }

    return count;
}

void generate_piece_moves_from_pos(const Board* b,
                                   Color turn_color,
                                   int from,
                                   Move moves[],
                                   int* count) {
    Cell me = b->cell[from];

    if (me.covered) return;
    if (me.piece == EMPTY) return;
    if (piece_color(me.piece) != turn_color) return;

    int r = from / COLS;
    int c = from % COLS;

    /*
       Cannon capture.
    */
    if (is_cannon(me.piece)) {
        for (int to = 0; to < N; to++) {
            if (from == to) continue;

            Cell target = b->cell[to];

            if (target.covered) continue;
            if (target.piece == EMPTY) continue;
            if (piece_color(target.piece) == turn_color) continue;

            int tr = to / COLS;
            int tc = to % COLS;

            if (r != tr && c != tc) continue;

            if (count_between(b, from, to) == 1) {
                add_move(moves, count, r, c, tr, tc, 0);
            }
        }
    }

    /*
       Adjacent move / capture.
    */
    int dir_count = MoveTab[from][0];

    for (int k = 1; k <= dir_count; k++) {
        int to = MoveTab[from][k];
        Cell target = b->cell[to];

        if (target.covered) continue;

        int tr = to / COLS;
        int tc = to % COLS;

        if (target.piece == EMPTY) {
            add_move(moves, count, r, c, tr, tc, 0);
        } else if (!is_cannon(me.piece) &&
                   piece_color(target.piece) != turn_color &&
                   OrderTab[me.piece][target.piece]) {
            add_move(moves, count, r, c, tr, tc, 0);
        }
    }
}

int generate_all_legal_moves(const Board* b, Color turn_color, Move moves[]) {
    int count = 0;

    if (turn_color == COLOR_NONE) {
        for (int i = 0; i < N; i++) {
            if (b->cell[i].covered) {
                add_move(moves, &count, i / COLS, i % COLS, -1, -1, 1);
            }
        }

        return count;
    }

    /*
       Flip moves.
    */
    for (int i = 0; i < N; i++) {
        if (b->cell[i].covered) {
            add_move(moves, &count, i / COLS, i % COLS, -1, -1, 1);
        }
    }

    /*
       Non-flip moves.
    */
    int start = (turn_color == COLOR_RED) ? RED_KING : BLACK_KING;
    int end = (turn_color == COLOR_RED) ? RED_SOLDIER : BLACK_SOLDIER;

    for (int p = start; p <= end; p++) {
        for (int k = 0; k < b->piece_count[p]; k++) {
            int from = b->piece_pos[p][k];

            if (from >= 0) {
                generate_piece_moves_from_pos(b, turn_color, from, moves, &count);
            }
        }
    }

    return count;
}

int generate_non_flip_moves(const Board* b, Color turn_color, Move moves[]) {
    int count = 0;

    if (turn_color == COLOR_NONE) return 0;

    int start = (turn_color == COLOR_RED) ? RED_KING : BLACK_KING;
    int end = (turn_color == COLOR_RED) ? RED_SOLDIER : BLACK_SOLDIER;

    for (int p = start; p <= end; p++) {
        for (int k = 0; k < b->piece_count[p]; k++) {
            int from = b->piece_pos[p][k];

            if (from >= 0) {
                generate_piece_moves_from_pos(b, turn_color, from, moves, &count);
            }
        }
    }

    return count;
}

int generate_capture_moves(const Board* b, Color turn_color, Move moves[]) {
    Move all[MAX_MOVES];
    int count = generate_non_flip_moves(b, turn_color, all);

    int n = 0;

    for (int i = 0; i < count; i++) {
        int to = idx_of(all[i].tr, all[i].tc);

        if (!b->cell[to].covered && b->cell[to].piece != EMPTY) {
            moves[n++] = all[i];
        }
    }

    return n;
}

int is_legal_move(const Board* b, Color turn_color, Move m) {
    Move moves[MAX_MOVES];
    int count = generate_all_legal_moves(b, turn_color, moves);

    for (int i = 0; i < count; i++) {
        if (same_move(moves[i], m)) return 1;
    }

    return 0;
}

void apply_move(Board* b, Move m) {
    int from = idx_of(m.r, m.c);

    if (m.is_flip) {
        b->cell[from].covered = 0;
        board_rebuild_aux(b);
        return;
    }

    int to = idx_of(m.tr, m.tc);

    b->cell[to] = b->cell[from];

    b->cell[from].piece = EMPTY;
    b->cell[from].covered = 0;

    board_rebuild_aux(b);
}

/* =========================================================
   Tactical / evaluation
========================================================= */

int square_attacked_by(const Board* b, int target_idx, Color attacker_color) {
    Move caps[MAX_MOVES];
    int count = generate_capture_moves(b, attacker_color, caps);

    int tr = target_idx / COLS;
    int tc = target_idx % COLS;

    for (int i = 0; i < count; i++) {
        if (caps[i].tr == tr && caps[i].tc == tc) return 1;
    }

    return 0;
}

int covered_neighbor_count(const Board* b, int idx) {
    int count = 0;
    int dir_count = MoveTab[idx][0];

    for (int k = 1; k <= dir_count; k++) {
        int to = MoveTab[idx][k];

        if (b->cell[to].covered) count++;
    }

    return count;
}

int king_soldier_danger(const Board* b, Color my_color) {
    int penalty = 0;
    Color opp = opposite_color(my_color);

    int king = (my_color == COLOR_RED) ? RED_KING : BLACK_KING;

    for (int k = 0; k < b->piece_count[king]; k++) {
        int i = b->piece_pos[king][k];

        if (i < 0) continue;

        int dir_count = MoveTab[i][0];

        for (int d = 1; d <= dir_count; d++) {
            int to = MoveTab[i][d];
            Cell near_cell = b->cell[to];

            if (!near_cell.covered &&
                near_cell.piece != EMPTY &&
                piece_color(near_cell.piece) == opp &&
                is_soldier(near_cell.piece)) {
                penalty += KING_SOLDIER_DANGER_PENALTY;
            }
        }
    }

    return penalty;
}

int cannon_threat_score(const Board* b, Color color) {
    int score = 0;

    int cannon = (color == COLOR_RED) ? RED_CANNON : BLACK_CANNON;

    for (int k = 0; k < b->piece_count[cannon]; k++) {
        int from = b->piece_pos[cannon][k];

        if (from < 0) continue;

        int r = from / COLS;
        int c = from % COLS;

        for (int to = 0; to < N; to++) {
            if (from == to) continue;

            Cell target = b->cell[to];

            if (target.covered) continue;
            if (target.piece == EMPTY) continue;
            if (piece_color(target.piece) == color) continue;

            int tr = to / COLS;
            int tc = to % COLS;

            if (r != tr && c != tc) continue;

            if (count_between(b, from, to) == 1) {
                score += piece_value(target.piece) / 3;
            }
        }
    }

    return score;
}

/* =========================================================
   Endgame positional evaluation
========================================================= */

int cannon_setup_score(const Board* b, Color color) {
    int score = 0;

    int cannon = (color == COLOR_RED) ? RED_CANNON : BLACK_CANNON;

    for (int k = 0; k < b->piece_count[cannon]; k++) {
        int from = b->piece_pos[cannon][k];

        if (from < 0) continue;

        int r = from / COLS;
        int c = from % COLS;

        int dirs[4][2] = {
            {1, 0},
            {-1, 0},
            {0, 1},
            {0, -1}
        };

        for (int d = 0; d < 4; d++) {
            int dr = dirs[d][0];
            int dc = dirs[d][1];

            int screen_found = 0;

            int nr = r + dr;
            int nc = c + dc;

            while (in_board(nr, nc)) {
                int idx = idx_of(nr, nc);
                Cell cell = b->cell[idx];

                if (!screen_found) {
                    if (cell.covered || cell.piece != EMPTY) {
                        screen_found = 1;
                    }
                } else {
                    if (cell.covered) {
                        score += 8;
                        break;
                    }

                    if (cell.piece != EMPTY) {
                        if (piece_color(cell.piece) != color) {
                            score += 40 + piece_value(cell.piece) / 4;
                        } else {
                            score += 4;
                        }

                        break;
                    }
                }

                nr += dr;
                nc += dc;
            }
        }
    }

    return score;
}

int can_pressure_piece(int attacker, int target) {
    if (attacker == EMPTY || target == EMPTY) return 0;
    if (piece_color(attacker) == piece_color(target)) return 0;

    if (can_normal_capture_piece(attacker, target)) {
        return 1;
    }

    if (is_cannon(attacker)) {
        return 0;
    }

    return 0;
}

int surround_score(const Board* b, Color color) {
    int score = 0;
    Color opp = opposite_color(color);

    int start = (color == COLOR_RED) ? RED_KING : BLACK_KING;
    int end = (color == COLOR_RED) ? RED_SOLDIER : BLACK_SOLDIER;

    for (int p = start; p <= end; p++) {
        for (int k = 0; k < b->piece_count[p]; k++) {
            int my_idx = b->piece_pos[p][k];

            if (my_idx < 0) continue;

            for (int enemy = 0; enemy < PIECE_TYPES; enemy++) {
                if (piece_color(enemy) != opp) continue;

                for (int e = 0; e < b->piece_count[enemy]; e++) {
                    int enemy_idx = b->piece_pos[enemy][e];

                    if (enemy_idx < 0) continue;

                    int dist = manhattan_distance(my_idx, enemy_idx);

                    if (dist == 0) continue;
                    if (dist > 3) continue;

                    /*
                       兵只對將帥有強壓迫價值。
                    */
                    if (is_soldier(p) && !is_king(enemy)) {
                        continue;
                    }

                    if (can_pressure_piece(p, enemy)) {
                        if (dist == 1) {
                            score += 35 + piece_value(enemy) / 5;
                        } else if (dist == 2) {
                            score += 18 + piece_value(enemy) / 8;
                        } else {
                            score += 8;
                        }
                    }
                }
            }
        }
    }

    return score;
}

int enemy_escape_penalty_score(const Board* b, Color color) {
    Color opp = opposite_color(color);

    int score = 0;

    int start = (opp == COLOR_RED) ? RED_KING : BLACK_KING;
    int end = (opp == COLOR_RED) ? RED_SOLDIER : BLACK_SOLDIER;

    for (int p = start; p <= end; p++) {
        for (int k = 0; k < b->piece_count[p]; k++) {
            int idx = b->piece_pos[p][k];

            if (idx < 0) continue;

            int escape = 0;
            int dir_count = MoveTab[idx][0];

            for (int d = 1; d <= dir_count; d++) {
                int to = MoveTab[idx][d];
                Cell target = b->cell[to];

                if (!target.covered && target.piece == EMPTY) {
                    escape++;
                }

                if (!target.covered &&
                    target.piece != EMPTY &&
                    piece_color(target.piece) == color &&
                    can_normal_capture_piece(p, target.piece)) {
                    escape++;
                }
            }

            if (escape == 0) {
                score += 60 + piece_value(p) / 4;
            } else if (escape == 1) {
                score += 30 + piece_value(p) / 8;
            } else if (escape == 2) {
                score += 10;
            }
        }
    }

    return score;
}

int endgame_positional_score(const Board* b, Color color) {
    int score = 0;

    score += cannon_setup_score(b, color);
    score += surround_score(b, color);
    score += enemy_escape_penalty_score(b, color);

    return score;
}

int move_score_tactical(const Board* b, Move m, Color turn_color) {
    if (m.is_flip) {
        int idx = idx_of(m.r, m.c);
        int score = 6;

        if (m.r >= 1 && m.r <= 2 && m.c >= 2 && m.c <= 5) {
            score += 3;
        }

        score -= covered_neighbor_count(b, idx);
        return score;
    }

    int from = idx_of(m.r, m.c);
    int to = idx_of(m.tr, m.tc);

    int moving_piece = b->cell[from].piece;
    int target = b->cell[to].piece;

    if (moving_piece == EMPTY) return -999999;

    int score = 0;

    if (target != EMPTY) {
        score += CAPTURE_BASE_BONUS;
        score += piece_value(target);

        int trade_gain = piece_value(target) - piece_value(moving_piece);

        if (trade_gain > 0) score += trade_gain;

        if (piece_rank(moving_piece) == piece_rank(target)) {
            score += SAME_RANK_CAPTURE_BONUS;
        }
    } else {
        score += 2;
    }

    Board tmp = *b;
    apply_move(&tmp, m);

    if (square_attacked_by(&tmp, to, opposite_color(turn_color))) {
        int penalty = (int)(piece_value(moving_piece) * DANGER_PENALTY_RATIO);

        if (target != EMPTY) {
            penalty = (int)(penalty * 0.65);
        }

        score -= penalty;
    }

    if (is_king(moving_piece)) {
        int dir_count = MoveTab[to][0];

        for (int k = 1; k <= dir_count; k++) {
            int near_idx = MoveTab[to][k];
            Cell near_cell = tmp.cell[near_idx];

            if (!near_cell.covered &&
                near_cell.piece != EMPTY &&
                piece_color(near_cell.piece) == opposite_color(turn_color) &&
                is_soldier(near_cell.piece)) {
                score -= KING_SOLDIER_DANGER_PENALTY;
            }
        }
    }

    if (piece_value(moving_piece) >= 270) {
        score -= covered_neighbor_count(&tmp, to) * HIGH_VALUE_COVERED_PENALTY;
    }

    return score;
}

int count_non_flip_mobility(const Board* b, Color color) {
    Move moves[MAX_MOVES];
    return generate_non_flip_moves(b, color, moves);
}

double evaluate_board(const Board* b, Color my_color) {
    Color opp = opposite_color(my_color);

    double score = 0.0;

    score += material_of_color(b, my_color);
    score -= material_of_color(b, opp);

    for (int i = 0; i < N; i++) {
        Cell cell = b->cell[i];

        if (cell.covered) continue;
        if (cell.piece == EMPTY) continue;

        int v = piece_value(cell.piece);

        if (piece_color(cell.piece) == my_color) {
            if (square_attacked_by(b, i, opp)) {
                score -= v * SELF_DANGER_RATIO;
            }

            if (v >= 270) {
                score -= covered_neighbor_count(b, i) * HIGH_VALUE_COVERED_PENALTY;
            }
        } else if (piece_color(cell.piece) == opp) {
            if (square_attacked_by(b, i, my_color)) {
                score += v * ATTACK_BONUS_RATIO;
            }

            if (v >= 270) {
                score += covered_neighbor_count(b, i) * 8;
            }
        }
    }

    score += count_non_flip_mobility(b, my_color) * 4.0;
    score -= count_non_flip_mobility(b, opp) * 4.0;

    score += cannon_threat_score(b, my_color);
    score -= cannon_threat_score(b, opp);

    score -= king_soldier_danger(b, my_color);
    score += king_soldier_danger(b, opp) * 0.8;

    /*
       只在真正殘局或接近殘局時啟用：
       1. 全明棋
       2. Covered <= 4 且明棋數 <= 8
    */
    if (is_endgame_like(b)) {
        score += endgame_positional_score(b, my_color);
        score -= endgame_positional_score(b, opp) * 0.85;
    }

    return score;
}

/* =========================================================
   Move ordering
========================================================= */

void sort_moves_by_tactical_score(const Board* b, Move moves[], int count, Color turn_color) {
    for (int i = 0; i < count - 1; i++) {
        int best = i;
        int best_score = move_score_tactical(b, moves[i], turn_color);

        for (int j = i + 1; j < count; j++) {
            int s = move_score_tactical(b, moves[j], turn_color);

            if (s > best_score) {
                best_score = s;
                best = j;
            }
        }

        if (best != i) {
            Move tmp = moves[i];
            moves[i] = moves[best];
            moves[best] = tmp;
        }
    }
}

/* =========================================================
   Cycle pruning
========================================================= */

int path_contains(unsigned long long key) {
    for (int i = 0; i < path_len; i++) {
        if (path_keys[i] == key) return 1;
    }

    return 0;
}

void path_push(unsigned long long key) {
    if (path_len < PATH_SIZE) {
        path_keys[path_len++] = key;
    }
}

void path_pop(void) {
    if (path_len > 0) {
        path_len--;
    }
}

/* =========================================================
   Transposition Table
========================================================= */

void tt_clear(void) {
    memset(tt, 0, sizeof(tt));
}

int tt_probe_ab(unsigned long long key,
                int depth,
                double alpha,
                double beta,
                double* out_value) {
    int idx = (int)(key & (TT_SIZE - 1));

    if (!tt[idx].valid) return 0;
    if (tt[idx].key != key) return 0;
    if (tt[idx].depth < depth) return 0;

    double v = tt[idx].value;

    if (tt[idx].flag == TT_EXACT) {
        *out_value = v;
        return 1;
    }

    if (tt[idx].flag == TT_LOWER && v >= beta) {
        *out_value = v;
        return 1;
    }

    if (tt[idx].flag == TT_UPPER && v <= alpha) {
        *out_value = v;
        return 1;
    }

    return 0;
}

void tt_store_ab(unsigned long long key,
                 int depth,
                 double value,
                 double alpha_orig,
                 double beta,
                 Move best_move) {
    int idx = (int)(key & (TT_SIZE - 1));

    if (tt[idx].valid && tt[idx].depth > depth) {
        return;
    }

    tt[idx].valid = 1;
    tt[idx].key = key;
    tt[idx].depth = depth;
    tt[idx].value = value;
    tt[idx].best_move = best_move;

    if (value <= alpha_orig) {
        tt[idx].flag = TT_UPPER;
    } else if (value >= beta) {
        tt[idx].flag = TT_LOWER;
    } else {
        tt[idx].flag = TT_EXACT;
    }
}

/* =========================================================
   Quiescence Search
========================================================= */

double quiescence(Board* b,
                  Color turn_color,
                  Color my_color,
                  double alpha,
                  double beta,
                  int q_depth) {
    if (time_is_up()) {
        double eval = evaluate_board(b, my_color);

        if (turn_color != my_color) eval = -eval;

        return eval;
    }

    double stand_pat = evaluate_board(b, my_color);

    if (turn_color != my_color) {
        stand_pat = -stand_pat;
    }

    if (stand_pat >= beta) {
        return beta;
    }

    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    if (q_depth <= 0) {
        return alpha;
    }

    Move caps[MAX_MOVES];
    int count = generate_capture_moves(b, turn_color, caps);

    if (count <= 0) {
        return alpha;
    }

    sort_moves_by_tactical_score(b, caps, count, turn_color);

    if (count > CAPTURE_BRANCH_LIMIT) count = CAPTURE_BRANCH_LIMIT;

    for (int i = 0; i < count; i++) {
        if (time_is_up()) break;

        Board next = *b;
        apply_move(&next, caps[i]);

        double score = -quiescence(&next,
                                   opposite_color(turn_color),
                                   my_color,
                                   -beta,
                                   -alpha,
                                   q_depth - 1);

        if (score >= beta) return beta;

        if (score > alpha) alpha = score;
    }

    return alpha;
}

/* =========================================================
   NegaMax + Alpha-Beta
========================================================= */

double negamax_ab(Board* b,
                  Color turn_color,
                  Color my_color,
                  int depth,
                  double alpha,
                  double beta,
                  int allow_null) {
    double alpha_orig = alpha;

    if (time_is_up()) {
        double eval = evaluate_board(b, my_color);

        if (turn_color != my_color) eval = -eval;

        return eval;
    }

    unsigned long long key = board_hash(b, turn_color);

    if (path_contains(key)) {
        double eval = evaluate_board(b, my_color);

        if (turn_color != my_color) eval = -eval;

        return eval;
    }

    path_push(key);

    if (depth <= 0) {
        double result = quiescence(b, turn_color, my_color, alpha, beta, MAX_Q_DEPTH);
        path_pop();
        return result;
    }

    unsigned long long tt_key = key ^ splitmix64((unsigned long long)depth);
    double cached = 0.0;

    if (tt_probe_ab(tt_key, depth, alpha, beta, &cached)) {
        path_pop();
        return cached;
    }

    Move moves[MAX_MOVES];
    int count = generate_non_flip_moves(b, turn_color, moves);

    /*
       Null move:
       有吃子時禁止空步，符合吃子優先。
    */
    Move caps_for_null[MAX_MOVES];
    int cap_count_for_null = generate_capture_moves(b, turn_color, caps_for_null);

    if (cap_count_for_null == 0 &&
        allow_null &&
        count_covered(b) > 0 &&
        depth >= 2) {

        double null_score = -negamax_ab(b,
                                        opposite_color(turn_color),
                                        my_color,
                                        depth - 1,
                                        -beta,
                                        -alpha,
                                        0);

        if (null_score > alpha) alpha = null_score;

        if (alpha >= beta) {
            path_pop();
            return beta;
        }
    }

    if (count <= 0) {
        double eval = evaluate_board(b, my_color);

        if (turn_color != my_color) eval = -eval;

        path_pop();
        return eval;
    }

    sort_moves_by_tactical_score(b, moves, count, turn_color);

    if (count > ALPHABETA_BRANCH_LIMIT) count = ALPHABETA_BRANCH_LIMIT;

    Move best_move = moves[0];

    for (int i = 0; i < count; i++) {
        if (time_is_up()) break;

        Board next = *b;
        apply_move(&next, moves[i]);

        double score = -negamax_ab(&next,
                                   opposite_color(turn_color),
                                   my_color,
                                   depth - 1,
                                   -beta,
                                   -alpha,
                                   1);

        score += move_score_tactical(b, moves[i], turn_color) * 0.015;

        if (score > alpha) {
            alpha = score;
            best_move = moves[i];
        }

        if (alpha >= beta) break;
    }

    tt_store_ab(tt_key, depth, alpha, alpha_orig, beta, best_move);

    path_pop();

    return alpha;
}

int choose_best_nonflip_by_negamax(const Board* b,
                                   Color my_color,
                                   int depth,
                                   Move* out_move,
                                   double* out_score) {
    Move moves[MAX_MOVES];
    int count = generate_non_flip_moves(b, my_color, moves);

    if (count <= 0) return 0;

    sort_moves_by_tactical_score(b, moves, count, my_color);

    if (count > ALPHABETA_BRANCH_LIMIT) count = ALPHABETA_BRANCH_LIMIT;

    double best_score = -1e18;
    Move best_move = moves[0];

    path_len = 0;

    for (int i = 0; i < count; i++) {
        if (time_is_up()) break;

        Board next = *b;
        apply_move(&next, moves[i]);

        double score = -negamax_ab(&next,
                                   opposite_color(my_color),
                                   my_color,
                                   depth - 1,
                                   -1e18,
                                   1e18,
                                   1);

        score += move_score_tactical(b, moves[i], my_color) * 0.03;

        /*
           只在殘局或接近殘局時，獎勵炮架、包圍、限制逃路。
        */
        if (is_endgame_like(b)) {
            score += endgame_positional_score(&next, my_color) * ENDGAME_MOVE_POSITIONAL_WEIGHT;
        }

        if (score > best_score) {
            best_score = score;
            best_move = moves[i];
        }
    }

    *out_move = best_move;
    *out_score = best_score;

    return 1;
}

int choose_best_capture_by_negamax(const Board* b,
                                   Color my_color,
                                   int depth,
                                   Move* out_move,
                                   double* out_score) {
    Move caps[MAX_MOVES];
    int count = generate_capture_moves(b, my_color, caps);

    if (count <= 0) {
        return 0;
    }

    sort_moves_by_tactical_score(b, caps, count, my_color);

    if (count > CAPTURE_BRANCH_LIMIT) {
        count = CAPTURE_BRANCH_LIMIT;
    }

    double best_score = -1e18;
    Move best_move = caps[0];

    path_len = 0;

    for (int i = 0; i < count; i++) {
        if (time_is_up()) break;

        Board next = *b;
        apply_move(&next, caps[i]);

        double score = -negamax_ab(&next,
                                   opposite_color(my_color),
                                   my_color,
                                   depth - 1,
                                   -1e18,
                                   1e18,
                                   1);

        score += move_score_tactical(b, caps[i], my_color) * 0.08;

        /*
           吃完後如果形成炮架、包圍、限制逃路，殘局時額外加分。
        */
        if (is_endgame_like(b)) {
            score += endgame_positional_score(&next, my_color) * ENDGAME_CAPTURE_POSITIONAL_WEIGHT;
        }

        if (score > best_score) {
            best_score = score;
            best_move = caps[i];
        }
    }

    *out_move = best_move;
    *out_score = best_score;

    return 1;
}

/* =========================================================
   Principal Variation
========================================================= */

void print_move_short(Move m) {
    if (m.is_flip) {
        printf("FLIP(%d,%d)", m.r, m.c);
    } else {
        printf("MOVE(%d,%d->%d,%d)", m.r, m.c, m.tr, m.tc);
    }
}

int tt_get_best_move(const Board* b, Color turn_color, int depth, Move* out) {
    unsigned long long key = board_hash(b, turn_color) ^ splitmix64((unsigned long long)depth);
    int idx = (int)(key & (TT_SIZE - 1));

    if (!tt[idx].valid) return 0;
    if (tt[idx].key != key) return 0;

    *out = tt[idx].best_move;
    return 1;
}

void print_principal_variation(const Board* root, Color my_color, int depth) {
    Board b = *root;
    Color turn = my_color;

    printf("[PV] ");

    for (int ply = 0; ply < PV_MAX_LEN && depth - ply > 0; ply++) {
        Move m;

        if (!tt_get_best_move(&b, turn, depth - ply, &m)) {
            break;
        }

        if (!is_legal_move(&b, turn, m)) {
            break;
        }

        print_move_short(m);
        printf(" ");

        apply_move(&b, m);
        turn = opposite_color(turn);
    }

    printf("\n");
}

/* =========================================================
   Flip decision
========================================================= */

int generate_candidate_flips(const Board* b, Move flips[]) {
    int count = 0;

    int preferred[] = {
        idx_of(1, 3), idx_of(1, 4),
        idx_of(2, 3), idx_of(2, 4),
        idx_of(1, 2), idx_of(1, 5),
        idx_of(2, 2), idx_of(2, 5),
        idx_of(0, 0), idx_of(0, 7),
        idx_of(3, 0), idx_of(3, 7)
    };

    int preferred_count = (int)(sizeof(preferred) / sizeof(preferred[0]));

    for (int i = 0; i < preferred_count && count < FLIP_CANDIDATE_LIMIT; i++) {
        int idx = preferred[i];

        if (b->cell[idx].covered) {
            flips[count].r = idx / COLS;
            flips[count].c = idx % COLS;
            flips[count].tr = -1;
            flips[count].tc = -1;
            flips[count].is_flip = 1;
            count++;
        }
    }

    for (int i = 0; i < N && count < FLIP_CANDIDATE_LIMIT; i++) {
        if (!b->cell[i].covered) continue;

        int duplicated = 0;

        for (int k = 0; k < count; k++) {
            if (idx_of(flips[k].r, flips[k].c) == i) {
                duplicated = 1;
                break;
            }
        }

        if (!duplicated) {
            flips[count].r = i / COLS;
            flips[count].c = i % COLS;
            flips[count].tr = -1;
            flips[count].tc = -1;
            flips[count].is_flip = 1;
            count++;
        }
    }

    return count;
}

double evaluate_flip_square(const Board* b,
                            Color my_color,
                            int flip_idx,
                            double move_score_baseline,
                            int search_depth) {
    int remaining[PIECE_TYPES];
    get_remaining_counts(b, remaining);

    int total = remaining_total_count(remaining);

    if (total <= 0) return -1e18;

    int gt = 0;
    double sum = 0.0;

    for (int p = 0; p < PIECE_TYPES; p++) {
        if (time_is_up()) break;
        if (remaining[p] <= 0) continue;

        Board next = *b;
        next.cell[flip_idx].piece = p;
        next.cell[flip_idx].covered = 0;
        board_rebuild_aux(&next);

        path_len = 0;

        double score = -negamax_ab(&next,
                                   opposite_color(my_color),
                                   my_color,
                                   search_depth,
                                   -1e18,
                                   1e18,
                                   1);

        int weight = remaining[p];

        if (score > move_score_baseline) {
            gt += weight;
        } else if (score < move_score_baseline) {
            gt -= weight;
        }

        sum += (score - move_score_baseline) * weight;
    }

    return gt * FLIP_SCORE_SCALE + sum / total;
}

/* =========================================================
   First flip
========================================================= */

Move choose_first_flip(const Board* b) {
    Move flips[FLIP_CANDIDATE_LIMIT];
    int count = generate_candidate_flips(b, flips);

    if (count > 0) return flips[0];

    for (int i = 0; i < N; i++) {
        if (b->cell[i].covered) {
            Move m = {i / COLS, i % COLS, -1, -1, 1};
            return m;
        }
    }

    Move fallback = {0, 0, -1, -1, 1};
    return fallback;
}

/* =========================================================
   Paper-style AI decision
========================================================= */

Move choose_paper_ai_move(const Board* b, Color my_color) {
    int covered = count_covered(b);
    int visible = count_visible_live_pieces(b);

    printf("[PaperAI] covered=%d visible=%d\n", covered, visible);

    if (my_color == COLOR_NONE) {
        printf("[PaperAI] mode=FIRST_FLIP\n");
        return choose_first_flip(b);
    }

    int depth;

    if (covered == 0 && visible <= ENDGAME_VISIBLE_THRESHOLD) {
        depth = SEARCH_DEPTH_ENDGAME;
    } else if (covered == 0) {
        depth = SEARCH_DEPTH_REVEALED;
    } else if (covered >= 16) {
        depth = SEARCH_DEPTH_OPENING;
    } else {
        depth = SEARCH_DEPTH_MIDGAME;
    }

    search_timer_start();
    tt_clear();
    path_len = 0;

    /*
       第一優先：有吃子就直接處理吃子，不再評估翻棋。
    */
    Move best_capture;
    double best_capture_score;

    if (choose_best_capture_by_negamax(b,
                                       my_color,
                                       depth,
                                       &best_capture,
                                       &best_capture_score)) {
        printf("[PaperAI] mode=FORCED_CAPTURE_PRIORITY score=%.2f\n",
               best_capture_score);

        print_principal_variation(b, my_color, depth);

        return best_capture;
    }

    /*
       第二優先：沒有吃子才搜尋一般走棋。
    */
    Move best_move;
    double best_move_score;
    int has_nonflip = choose_best_nonflip_by_negamax(b,
                                                     my_color,
                                                     depth,
                                                     &best_move,
                                                     &best_move_score);

    if (!has_nonflip) {
        printf("[PaperAI] no non-flip move, flip forced\n");
        return choose_first_flip(b);
    }

    printf("[PaperAI] best non-flip score=%.2f depth=%d\n",
           best_move_score,
           depth);

    print_principal_variation(b, my_color, depth);

    /*
       全明棋時直接走棋，不做任何翻棋計算。
    */
    if (covered <= 0) {
        printf("[PaperAI] mode=REVEALED_NEGAMAX_ONLY\n");
        return best_move;
    }

    /*
       沒有吃子，且有暗棋，才評估翻棋。
    */
    if (time_is_up()) {
        printf("[PaperAI] time up before flip evaluation, choose move\n");
        return best_move;
    }

    Move flips[FLIP_CANDIDATE_LIMIT];
    int flip_count = generate_candidate_flips(b, flips);

    if (flip_count <= 0) {
        printf("[PaperAI] no flip candidate\n");
        return best_move;
    }

    double best_flip_score = -1e18;
    Move best_flip = flips[0];

    int flip_search_depth = depth - 2;

    if (flip_search_depth < 1) {
        flip_search_depth = 1;
    }

    for (int i = 0; i < flip_count; i++) {
        if (time_is_up()) break;

        int idx = idx_of(flips[i].r, flips[i].c);

        double flip_score = evaluate_flip_square(b,
                                                 my_color,
                                                 idx,
                                                 best_move_score,
                                                 flip_search_depth);

        printf("[PaperAI] flip candidate %d %d score=%.2f\n",
               flips[i].r,
               flips[i].c,
               flip_score);

        if (flip_score > best_flip_score) {
            best_flip_score = flip_score;
            best_flip = flips[i];
        }
    }

    if (best_flip_score > FLIP_MARGIN_IF_MOVE) {
        printf("[PaperAI] decision=FLIP flip_score=%.2f threshold=%.2f\n",
               best_flip_score,
               FLIP_MARGIN_IF_MOVE);
        return best_flip;
    }

    printf("[PaperAI] decision=MOVE move_score=%.2f flip_score=%.2f\n",
           best_move_score,
           best_flip_score);

    return best_move;
}

/* =========================================================
   Send move
========================================================= */

void make_move(const char* json, const char* my_role_ab) {
    Board visible;
    parse_visible_board(json, &visible);

    char color_str[16];
    get_role_color(json, my_role_ab, color_str);

    Color my_color = str_to_color(color_str);

    Move best = choose_paper_ai_move(&visible, my_color);

    if (my_color != COLOR_NONE) {
        if (!is_legal_move(&visible, my_color, best)) {
            printf("[Warning] selected illegal move. Fallback to first legal.\n");

            Move legal[MAX_MOVES];
            int count = generate_all_legal_moves(&visible, my_color, legal);

            if (count <= 0) {
                printf("[Error] no legal move.\n");
                return;
            }

            sort_moves_by_tactical_score(&visible, legal, count, my_color);
            best = legal[0];
        }
    }

    char action[64];

    if (best.is_flip) {
        sprintf(action, "%d %d\n", best.r, best.c);
        printf("Action: FLIP %d %d\n", best.r, best.c);
    } else {
        sprintf(action, "%d %d %d %d\n",
                best.r,
                best.c,
                best.tr,
                best.tc);

        printf("Action: MOVE %d %d -> %d %d\n",
               best.r,
               best.c,
               best.tr,
               best.tc);
    }

    Sleep(ACTION_DELAY_MS);
    send_action(action);
}

/* =========================================================
   Program entry
========================================================= */

int main(void) {
    char board_data[12000];
    int last_total_moves = -1;

    srand((unsigned int)time(NULL));

    init_move_tab();
    init_order_tab();
    init_zobrist();
    tt_clear();

    if (init_connection() != 0) {
        printf("Connection failed.\n");
        return 1;
    }

    auto_join_room();

    char my_role_ab[4] = "";

    if (strcmp(_assigned_role, "first") == 0) {
        strcpy(my_role_ab, "A");
    } else if (strcmp(_assigned_role, "second") == 0) {
        strcpy(my_role_ab, "B");
    } else {
        printf("Unknown role: %s\n", _assigned_role);
        close_connection();
        return 1;
    }

    printf("My role: %s\n", my_role_ab);

    while (1) {
        receive_update(board_data, sizeof(board_data));

        if (strlen(board_data) == 0) {
            printf("Disconnected.\n");
            break;
        }

        printf("\n%s\n", board_data);

        if (!strstr(board_data, "UPDATE")) {
            continue;
        }

        if (!strstr(board_data, "\"state\": \"playing\"")) {
            continue;
        }

        Board current_visible;
        parse_visible_board(board_data, &current_visible);
        update_history_from_previous_board(&current_visible);

        int current_total_moves = get_total_moves(board_data);

        char current_turn_role[4];

        if (!get_current_turn_role(board_data, current_turn_role)) {
            continue;
        }

        if (strcmp(current_turn_role, my_role_ab) == 0 &&
            current_total_moves != last_total_moves) {

            printf("It's my turn. Role = %s, total_moves = %d\n",
                   my_role_ab,
                   current_total_moves);

            print_dead_counts();

            make_move(board_data, my_role_ab);

            last_total_moves = current_total_moves;
        }
    }

    close_connection();

    return 0;
}
#ifndef GAME_H
#define GAME_H

#include "board.h"
#include "ai.h"

typedef enum {
    PHASE_CHOOSE_ORDER,
    PHASE_PLAYING,
    PHASE_GAME_OVER
} GamePhase;

typedef enum {
    TURN_PLAYER,
    TURN_COMPUTER_THINKING,
    TURN_COMPUTER_SHOWING
} Turn;

typedef enum {
    WINNER_NONE,
    WINNER_PLAYER,
    WINNER_COMPUTER,
    WINNER_DRAW
} Winner;

typedef struct {
    Board board;

    GamePhase phase;
    Turn turn;
    Winner winner;

    int game_over;

    int player_goes_first;
    int sides_assigned;
    PieceSide player_side;
    PieceSide computer_side;

    int player_steps;
    int computer_steps;
    int max_steps;

    int selected_row;
    int selected_col;

    float ai_delay_timer;
    float highlight_timer;

    int last_flip_row;
    int last_flip_col;

    char status_text[128];
} Game;

void init_game(Game *game);
void update_game(Game *game);
void handle_player_click(Game *game, int mouseX, int mouseY);
int screen_to_board(int mouseX, int mouseY, int *row, int *col);

#endif
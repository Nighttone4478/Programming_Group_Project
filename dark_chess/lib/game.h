#ifndef GAME_H
#define GAME_H

#include "board.h"

typedef enum {
    TURN_PLAYER,
    TURN_COMPUTER_THINKING,
    TURN_COMPUTER_SHOWING
} Turn;

typedef struct {
    Board board;
    Turn turn;
    int game_over;

    float ai_delay_timer;      // 電腦等待翻牌
    float highlight_timer;     // 翻牌後高光顯示時間

    int last_flip_row;
    int last_flip_col;
} Game;

void init_game(Game *game);
void update_game(Game *game);
void handle_player_click(Game *game, int mouseX, int mouseY);
int screen_to_board(int mouseX, int mouseY, int *row, int *col);

#endif
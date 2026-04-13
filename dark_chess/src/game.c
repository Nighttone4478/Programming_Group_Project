#include "game.h"
#include "ai.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>

#define CELL_SIZE 110
#define BOARD_OFFSET_X 50
#define BOARD_OFFSET_Y 150

#define AI_THINK_DELAY 0.75f
#define AI_SHOW_DELAY  0.60f

#define ORDER_BTN_W 220
#define ORDER_BTN_H 70
#define ORDER_BTN_Y 35
#define ORDER_BTN_PLAYER_X 230
#define ORDER_BTN_COMPUTER_X 530

static Rectangle get_player_first_button(void)
{
    return (Rectangle){ ORDER_BTN_PLAYER_X, ORDER_BTN_Y, ORDER_BTN_W, ORDER_BTN_H };
}

static Rectangle get_computer_first_button(void)
{
    return (Rectangle){ ORDER_BTN_COMPUTER_X, ORDER_BTN_Y, ORDER_BTN_W, ORDER_BTN_H };
}

static void set_status(Game *game, const char *text)
{
    snprintf(game->status_text, sizeof(game->status_text), "%s", text);
}

static void assign_sides_after_flip(Game *game, Piece revealedPiece, int currentActorIsPlayer)
{
    if (game->sides_assigned || is_empty_piece(revealedPiece)) {
        return;
    }

    if (currentActorIsPlayer) {
        game->player_side = revealedPiece.side;
        game->computer_side = (revealedPiece.side == SIDE_RED) ? SIDE_BLACK : SIDE_RED;
    } else {
        game->computer_side = revealedPiece.side;
        game->player_side = (revealedPiece.side == SIDE_RED) ? SIDE_BLACK : SIDE_RED;
    }

    game->sides_assigned = 1;
}

static void decide_winner(Game *game)
{
    int playerScore = 0;
    int computerScore = 0;

    if (game->sides_assigned) {
        playerScore = board_material_score(&game->board, game->player_side);
        computerScore = board_material_score(&game->board, game->computer_side);
    }

    if (playerScore > computerScore) {
        game->winner = WINNER_PLAYER;
        set_status(game, "遊戲結束：玩家獲勝");
    } else if (computerScore > playerScore) {
        game->winner = WINNER_COMPUTER;
        set_status(game, "遊戲結束：電腦獲勝");
    } else {
        game->winner = WINNER_DRAW;
        set_status(game, "遊戲結束：平手");
    }

    game->phase = PHASE_GAME_OVER;
    game->game_over = 1;
    game->turn = TURN_PLAYER;
    game->selected_row = -1;
    game->selected_col = -1;
}
static int count_side_pieces(const Board *board, PieceSide side)
{
    int count = 0;
    for (int r = 0; r < BOARD_ROWS; r++) {
        for (int c = 0; c < BOARD_COLS; c++) {
            Piece p = board->cells[r][c];
            if (!is_empty_piece(p) && p.side == side) {
                count++;
            }
        }
    }
    return count;
}

static void check_real_game_over(Game *game)
{
    if (!game->sides_assigned) {
        return;
    }

    int playerCount = count_side_pieces(&game->board, game->player_side);
    int computerCount = count_side_pieces(&game->board, game->computer_side);

    if (playerCount == 0) {
        game->winner = WINNER_COMPUTER;
        game->phase = PHASE_GAME_OVER;
        game->game_over = 1;
        set_status(game, "遊戲結束：電腦獲勝");
        return;
    }

    if (computerCount == 0) {
        game->winner = WINNER_PLAYER;
        game->phase = PHASE_GAME_OVER;
        game->game_over = 1;
        set_status(game, "遊戲結束：玩家獲勝");
        return;
    }

    if (count_unrevealed(&game->board) == 0) {
        int playerCan = has_any_legal_action(&game->board, game->player_side);
        int computerCan = has_any_legal_action(&game->board, game->computer_side);

        if (!playerCan && !computerCan) {
            decide_winner(game);
        }
    }
}

static void advance_turn(Game *game, int actorIsPlayer)
{
    if (actorIsPlayer) {
        game->player_steps++;
    } else {
        game->computer_steps++;
    }

    game->selected_row = -1;
    game->selected_col = -1;

    check_real_game_over(game);
    if (game->game_over) {
        return;
    }

    if (actorIsPlayer) {
        game->turn = TURN_COMPUTER_THINKING;
        game->ai_delay_timer = AI_THINK_DELAY;
        set_status(game, "電腦思考中");
    } else {
        game->turn = TURN_PLAYER;
        set_status(game, "玩家回合");
    }
}

static void perform_player_flip(Game *game, int row, int col)
{
    if (!flip_piece(&game->board, row, col)) {
        return;
    }

    game->last_flip_row = row;
    game->last_flip_col = col;

    assign_sides_after_flip(game, game->board.cells[row][col], 1);

    if (!game->sides_assigned) {
        set_status(game, "玩家翻棋");
    } else {
        set_status(game, "玩家翻棋完成");
    }

    advance_turn(game, 1);
}

static void perform_player_move_or_capture(Game *game, int fromRow, int fromCol, int toRow, int toCol)
{
    if (can_move_piece(&game->board, fromRow, fromCol, toRow, toCol)) {
        move_piece(&game->board, fromRow, fromCol, toRow, toCol);
        set_status(game, "玩家移動棋子");
        advance_turn(game, 1);
        return;
    }

    if (can_capture_piece(&game->board, fromRow, fromCol, toRow, toCol)) {
        capture_piece(&game->board, fromRow, fromCol, toRow, toCol);
        set_status(game, "玩家吃子成功");
        advance_turn(game, 1);
        return;
    }
}

void init_game(Game *game)
{
    init_board(&game->board);

    game->phase = PHASE_CHOOSE_ORDER;
    game->turn = TURN_PLAYER;
    game->winner = WINNER_NONE;
    game->game_over = 0;

    game->player_goes_first = 1;
    game->sides_assigned = 0;
    game->player_side = SIDE_NONE;
    game->computer_side = SIDE_NONE;

    game->player_steps = 0;
    game->computer_steps = 0;
    game->max_steps = 999999;

    game->selected_row = -1;
    game->selected_col = -1;

    game->ai_delay_timer = 0.0f;
    game->highlight_timer = 0.0f;

    game->last_flip_row = -1;
    game->last_flip_col = -1;

    set_status(game, "請先選擇玩家先手或電腦先手");
}

int screen_to_board(int mouseX, int mouseY, int *row, int *col)
{
    int localX = mouseX - BOARD_OFFSET_X;
    int localY = mouseY - BOARD_OFFSET_Y;

    if (localX < 0 || localY < 0) {
        return 0;
    }

    *col = localX / CELL_SIZE;
    *row = localY / CELL_SIZE;

    if (!is_valid_position(*row, *col)) {
        return 0;
    }

    return 1;
}

void handle_player_click(Game *game, int mouseX, int mouseY)
{
    if (game->phase == PHASE_GAME_OVER) {
        return;
    }

    if (game->phase == PHASE_CHOOSE_ORDER) {
        Vector2 mouse = { (float)mouseX, (float)mouseY };

        if (CheckCollisionPointRec(mouse, get_player_first_button())) {
            game->player_goes_first = 1;
            game->phase = PHASE_PLAYING;
            game->turn = TURN_PLAYER;
            set_status(game, "玩家先手，請先翻棋或行動");
            return;
        }

        if (CheckCollisionPointRec(mouse, get_computer_first_button())) {
            game->player_goes_first = 0;
            game->phase = PHASE_PLAYING;
            game->turn = TURN_COMPUTER_THINKING;
            game->ai_delay_timer = AI_THINK_DELAY;
            set_status(game, "電腦先手，電腦思考中");
            return;
        }

        return;
    }

    if (game->game_over) {
        return;
    }

    if (game->turn != TURN_PLAYER) {
        return;
    }

    int row, col;
    if (!screen_to_board(mouseX, mouseY, &row, &col)) {
        game->selected_row = -1;
        game->selected_col = -1;
        return;
    }

    Piece clicked = game->board.cells[row][col];

    if (!game->sides_assigned) {
        if (!is_empty_piece(clicked) && !clicked.revealed) {
            perform_player_flip(game, row, col);
        }
        return;
    }

    if (!is_empty_piece(clicked) && !clicked.revealed) {
        game->selected_row = -1;
        game->selected_col = -1;
        perform_player_flip(game, row, col);
        return;
    }

    if (game->selected_row == -1 || game->selected_col == -1) {
        if (!is_empty_piece(clicked) && clicked.revealed && clicked.side == game->player_side) {
            game->selected_row = row;
            game->selected_col = col;
            set_status(game, "已選取玩家棋子");
        }
        return;
    }

    if (game->selected_row == row && game->selected_col == col) {
        game->selected_row = -1;
        game->selected_col = -1;
        set_status(game, "已取消選取");
        return;
    }

    if (!is_empty_piece(clicked) && clicked.revealed && clicked.side == game->player_side) {
        game->selected_row = row;
        game->selected_col = col;
        set_status(game, "已重新選取玩家棋子");
        return;
    }

    perform_player_move_or_capture(
        game,
        game->selected_row, game->selected_col,
        row, col
    );
}

void update_game(Game *game)
{
    if (game->phase != PHASE_PLAYING || game->game_over) {
        return;
    }

    if (game->turn == TURN_COMPUTER_THINKING) {
        game->ai_delay_timer -= GetFrameTime();

        if (game->ai_delay_timer <= 0.0f) {
            AIMove move;
            int ok = ai_choose_move(&game->board, game->computer_side, game->player_side, &move);

            if (!game->sides_assigned) {
                int hiddenExists = count_unrevealed(&game->board) > 0;
                if (hiddenExists) {
                    int r = -1, c = -1;

                    for (int rr = 0; rr < BOARD_ROWS && r == -1; rr++) {
                        for (int cc = 0; cc < BOARD_COLS; cc++) {
                            if (!is_empty_piece(game->board.cells[rr][cc]) &&
                                !game->board.cells[rr][cc].revealed) {
                                r = rr;
                                c = cc;
                                break;
                            }
                        }
                    }

                    if (r != -1) {
                        flip_piece(&game->board, r, c);
                        game->last_flip_row = r;
                        game->last_flip_col = c;
                        assign_sides_after_flip(game, game->board.cells[r][c], 0);
                        set_status(game, "電腦翻棋完成");
                        game->turn = TURN_COMPUTER_SHOWING;
                        game->highlight_timer = AI_SHOW_DELAY;
                        return;
                    }
                }
            }

            if (!ok || move.action == AI_ACTION_NONE) {
                decide_winner(game);
                return;
            }

            if (move.action == AI_ACTION_FLIP) {
                flip_piece(&game->board, move.to_row, move.to_col);
                game->last_flip_row = move.to_row;
                game->last_flip_col = move.to_col;
                assign_sides_after_flip(game, game->board.cells[move.to_row][move.to_col], 0);
                set_status(game, "電腦翻棋完成");
            }
            else if (move.action == AI_ACTION_MOVE) {
                move_piece(&game->board, move.from_row, move.from_col, move.to_row, move.to_col);
                game->last_flip_row = move.to_row;
                game->last_flip_col = move.to_col;
                set_status(game, "電腦移動棋子");
            }
            else if (move.action == AI_ACTION_CAPTURE) {
                capture_piece(&game->board, move.from_row, move.from_col, move.to_row, move.to_col);
                game->last_flip_row = move.to_row;
                game->last_flip_col = move.to_col;
                set_status(game, "電腦吃子成功");
            }

            game->turn = TURN_COMPUTER_SHOWING;
            game->highlight_timer = AI_SHOW_DELAY;
        }
    }
    else if (game->turn == TURN_COMPUTER_SHOWING) {
        game->highlight_timer -= GetFrameTime();

        if (game->highlight_timer <= 0.0f) {
            advance_turn(game, 0);
        }
    }
}
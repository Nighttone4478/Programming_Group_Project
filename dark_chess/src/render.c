#include "render.h"
#include "piece.h"
#include "raylib.h"
#include <stdio.h>
#include <math.h>

#define SCREEN_WIDTH   980
#define SCREEN_HEIGHT  760

#define CELL_SIZE      110
#define BOARD_OFFSET_X 50
#define BOARD_OFFSET_Y 150
#define PIECE_RADIUS   41.0f

#define ORDER_BTN_W 220
#define ORDER_BTN_H 70
#define ORDER_BTN_Y 35
#define ORDER_BTN_PLAYER_X 230
#define ORDER_BTN_COMPUTER_X 530

#define COLOR_BOARD_BG (Color){222, 205, 170, 255}
#define COLOR_BOARD_BORDER (Color){80, 58, 35, 255}
#define COLOR_BOARD_MARK (Color){110, 88, 60, 255}
#define COLOR_RIVER_BG (Color){214, 196, 160, 255}

#define COLOR_BACK_PIECE (Color){145, 175, 95, 255}
#define COLOR_BACK_RING (Color){110, 135, 70, 255}

#define COLOR_FRONT_PIECE (Color){245, 236, 220, 255}
#define COLOR_FRONT_RING (Color){168, 140, 110, 255}

#define COLOR_RED_TEXT (Color){170, 50, 50, 255}
#define COLOR_BLACK_TEXT (Color){60, 60, 60, 255}

#define COLOR_HOVER (Color){255, 220, 120, 120}
#define COLOR_LAST_FLIP (Color){255, 235, 120, 170}
#define COLOR_SELECT (Color){80, 170, 255, 180}
#define COLOR_MOVE_HINT (Color){80, 180, 120, 120}
#define COLOR_CAPTURE_HINT (Color){220, 70, 70, 140}

static Rectangle get_player_first_button(void)
{
    return (Rectangle){ORDER_BTN_PLAYER_X, ORDER_BTN_Y, ORDER_BTN_W, ORDER_BTN_H};
}

static Rectangle get_computer_first_button(void)
{
    return (Rectangle){ORDER_BTN_COMPUTER_X, ORDER_BTN_Y, ORDER_BTN_W, ORDER_BTN_H};
}

static const char *side_text(PieceSide side)
{
    if (side == SIDE_RED)
        return "紅";
    if (side == SIDE_BLACK)
        return "黑";
    return "未定";
}

static Color side_color(PieceSide side)
{
    if (side == SIDE_RED) {
        return (Color){170, 50, 50, 255};
    }
    if (side == SIDE_BLACK) {
        return (Color){60, 60, 60, 255};
    }
    return COLOR_BOARD_BORDER;
}

static void draw_text_center(Font font, const char *text, Vector2 center, float fontSize, Color color)
{
    Vector2 size = MeasureTextEx(font, text, fontSize, 1.0f);
    Vector2 pos = {
        center.x - size.x / 2.0f,
        center.y - size.y / 2.0f};
    DrawTextEx(font, text, pos, fontSize, 1.0f, color);
}

static void draw_board_texture_noise(int width, int height)
{
    for (int i = 0; i < 650; i++)
    {
        int x = GetRandomValue(BOARD_OFFSET_X - 18, BOARD_OFFSET_X + width + 18);
        int y = GetRandomValue(BOARD_OFFSET_Y - 18, BOARD_OFFSET_Y + height + 18);
        int a = GetRandomValue(8, 18);
        DrawCircle(x, y, GetRandomValue(1, 2), (Color){100, 80, 50, a});
    }

    for (int i = 0; i < 120; i++)
    {
        int x1 = GetRandomValue(BOARD_OFFSET_X - 10, BOARD_OFFSET_X + width + 10);
        int y1 = GetRandomValue(BOARD_OFFSET_Y - 10, BOARD_OFFSET_Y + height + 10);
        int len = GetRandomValue(8, 24);

        DrawLineEx(
            (Vector2){(float)x1, (float)y1},
            (Vector2){(float)(x1 + len), (float)(y1 + GetRandomValue(-2, 2))},
            1.0f,
            (Color){120, 95, 65, 12});
    }
}

static void draw_board_background(void)
{
    int boardWidth = BOARD_COLS * CELL_SIZE;
    int boardHeight = BOARD_ROWS * CELL_SIZE;

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){235, 225, 205, 255});

    DrawRectangle(
        BOARD_OFFSET_X - 20,
        BOARD_OFFSET_Y - 20,
        boardWidth + 40,
        boardHeight + 40,
        COLOR_BOARD_BG);

    DrawRectangleGradientV(
        BOARD_OFFSET_X - 20,
        BOARD_OFFSET_Y - 20,
        boardWidth + 40,
        boardHeight + 40,
        Fade(WHITE, 0.08f),
        Fade(BLACK, 0.04f));

    draw_board_texture_noise(boardWidth, boardHeight);

    DrawRectangleLinesEx(
        (Rectangle){
            BOARD_OFFSET_X - 20,
            BOARD_OFFSET_Y - 20,
            boardWidth + 40,
            boardHeight + 40},
        3.0f,
        COLOR_BOARD_BORDER);
}

static void draw_board_grid(void)
{
    int boardWidth = BOARD_COLS * CELL_SIZE;
    int boardHeight = BOARD_ROWS * CELL_SIZE;

    for (int r = 0; r <= BOARD_ROWS; r++)
    {
        float y = BOARD_OFFSET_Y + r * CELL_SIZE;
        DrawLineEx(
            (Vector2){BOARD_OFFSET_X, y},
            (Vector2){BOARD_OFFSET_X + boardWidth, y},
            2.0f,
            COLOR_BOARD_BORDER);
    }

    for (int c = 0; c <= BOARD_COLS; c++)
    {
        float x = BOARD_OFFSET_X + c * CELL_SIZE;
        DrawLineEx(
            (Vector2){x, BOARD_OFFSET_Y},
            (Vector2){x, BOARD_OFFSET_Y + boardHeight},
            2.0f,
            COLOR_BOARD_BORDER);
    }
}

static void draw_corner_mark(float x, float y)
{
    float s = 7.0f;

    DrawLineEx((Vector2){x - s, y - 2}, (Vector2){x - 2, y - 2}, 1.5f, COLOR_BOARD_MARK);
    DrawLineEx((Vector2){x - 2, y - 2}, (Vector2){x - 2, y - s}, 1.5f, COLOR_BOARD_MARK);

    DrawLineEx((Vector2){x + s, y - 2}, (Vector2){x + 2, y - 2}, 1.5f, COLOR_BOARD_MARK);
    DrawLineEx((Vector2){x + 2, y - 2}, (Vector2){x + 2, y - s}, 1.5f, COLOR_BOARD_MARK);

    DrawLineEx((Vector2){x - s, y + 2}, (Vector2){x - 2, y + 2}, 1.5f, COLOR_BOARD_MARK);
    DrawLineEx((Vector2){x - 2, y + 2}, (Vector2){x - 2, y + s}, 1.5f, COLOR_BOARD_MARK);

    DrawLineEx((Vector2){x + s, y + 2}, (Vector2){x + 2, y + 2}, 1.5f, COLOR_BOARD_MARK);
    DrawLineEx((Vector2){x + 2, y + 2}, (Vector2){x + 2, y + s}, 1.5f, COLOR_BOARD_MARK);
}

static void draw_board_decorations(void)
{
    int boardWidth = BOARD_COLS * CELL_SIZE;
    int boardHeight = BOARD_ROWS * CELL_SIZE;

    Rectangle river = {
        BOARD_OFFSET_X + boardWidth * 0.32f,
        BOARD_OFFSET_Y - 8,
        boardWidth * 0.36f,
        boardHeight + 16};

    DrawRectangleRec(river, Fade(COLOR_RIVER_BG, 0.42f));

    for (int r = 0; r < BOARD_ROWS; r++)
    {
        for (int c = 0; c < BOARD_COLS; c++)
        {
            float cx = BOARD_OFFSET_X + c * CELL_SIZE + CELL_SIZE / 2.0f;
            float cy = BOARD_OFFSET_Y + r * CELL_SIZE + CELL_SIZE / 2.0f;
            draw_corner_mark(cx, cy);
        }
    }
}

static void draw_piece_shadow(Vector2 center)
{
    DrawEllipse((int)center.x + 4, (int)center.y + 8, 29, 11, Fade(BLACK, 0.18f));
    DrawEllipse((int)center.x + 3, (int)center.y + 9, 24, 8, Fade(BLACK, 0.10f));
}

static void draw_piece_highlight(Vector2 center)
{
    DrawCircleSector(
        (Vector2){center.x - 9, center.y - 10},
        12.0f,
        210.0f,
        25.0f,
        18,
        Fade(WHITE, 0.30f));
}

static void draw_piece_back(Vector2 center)
{
    draw_piece_shadow(center);

    DrawCircleV((Vector2){center.x, center.y + 2.0f}, PIECE_RADIUS + 1.0f, Fade(COLOR_BACK_RING, 0.85f));
    DrawCircleV(center, PIECE_RADIUS + 2.0f, COLOR_BACK_RING);
    DrawCircleV(center, PIECE_RADIUS, COLOR_BACK_PIECE);
    DrawCircleLines((int)center.x, (int)center.y, PIECE_RADIUS - 4.0f, Fade(WHITE, 0.22f));

    draw_piece_highlight(center);
}

static void draw_piece_front(Vector2 center, Piece piece, Font font)
{
    Color textColor = (piece.side == SIDE_RED) ? COLOR_RED_TEXT : COLOR_BLACK_TEXT;

    draw_piece_shadow(center);

    DrawCircleV((Vector2){center.x, center.y + 2.0f}, PIECE_RADIUS + 1.0f, Fade(COLOR_FRONT_RING, 0.85f));
    DrawCircleV(center, PIECE_RADIUS + 2.0f, COLOR_FRONT_RING);
    DrawCircleV(center, PIECE_RADIUS, COLOR_FRONT_PIECE);
    DrawCircleLines((int)center.x, (int)center.y, PIECE_RADIUS - 6.0f, Fade(COLOR_FRONT_RING, 0.75f));

    draw_piece_highlight(center);
    draw_text_center(font, get_piece_code(piece), center, 34.0f, textColor);
}

static void draw_ring_hint(Vector2 center, Color color, float inner, float outer)
{
    DrawRing(center, inner, outer, 0.0f, 360.0f, 64, color);
}

static void draw_hover_effect(Vector2 center)
{
    float pulse = 3.0f + 2.0f * sinf((float)GetTime() * 4.0f);

    DrawRing(center, PIECE_RADIUS + 8.0f, PIECE_RADIUS + 12.0f + pulse, 0.0f, 360.0f, 64, Fade(COLOR_LAST_FLIP, 0.75f));
    DrawRing(center, PIECE_RADIUS + 2.0f, PIECE_RADIUS + 5.0f, 0.0f, 360.0f, 64, Fade(COLOR_LAST_FLIP, 0.40f));
}

static void draw_last_flip_effect(Vector2 center)
{
    float pulse = 3.0f + 2.0f * sinf((float)GetTime() * 4.0f);

    DrawRing(center, PIECE_RADIUS + 8.0f, PIECE_RADIUS + 12.0f + pulse, 0.0f, 360.0f, 64, Fade(COLOR_LAST_FLIP, 0.75f));
    DrawRing(center, PIECE_RADIUS + 2.0f, PIECE_RADIUS + 5.0f, 0.0f, 360.0f, 64, Fade(COLOR_LAST_FLIP, 0.40f));
}

static void draw_order_buttons(const Assets *assets)
{
    Rectangle a = get_player_first_button();
    Rectangle b = get_computer_first_button();

    DrawRectangleRounded(a, 0.20f, 16, (Color){242, 235, 220, 255});
    DrawRectangleRoundedLinesEx(a, 0.20f, 16, 3.0f, COLOR_BOARD_BORDER);
    draw_text_center(assets->chineseFontBold, "玩家先手", (Vector2){a.x + a.width / 2, a.y + a.height / 2}, 28, DARKBLUE);

    DrawRectangleRounded(b, 0.20f, 16, (Color){242, 235, 220, 255});
    DrawRectangleRoundedLinesEx(b, 0.20f, 16, 3.0f, COLOR_BOARD_BORDER);
    draw_text_center(assets->chineseFontBold, "電腦先手", (Vector2){b.x + b.width / 2, b.y + b.height / 2}, 28, MAROON);
}

static void draw_piece_cell(
    int row,
    int col,
    Piece piece,
    Font font,
    const Game *game,
    int hoverRow,
    int hoverCol)
{
    Vector2 center = {
        BOARD_OFFSET_X + col * CELL_SIZE + CELL_SIZE / 2.0f,
        BOARD_OFFSET_Y + row * CELL_SIZE + CELL_SIZE / 2.0f};

    if (game->turn == TURN_COMPUTER_SHOWING &&
        row == game->last_flip_row &&
        col == game->last_flip_col)
    {
        draw_last_flip_effect(center);
    }

    if (!is_empty_piece(piece) &&
        game->turn == TURN_PLAYER &&
        row == hoverRow &&
        col == hoverCol)
    {
        draw_hover_effect(center);
    }

    if (game->selected_row == row && game->selected_col == col)
    {
        draw_ring_hint(center, COLOR_SELECT, PIECE_RADIUS + 6.0f, PIECE_RADIUS + 11.0f);
    }

    if (!piece.revealed && !is_empty_piece(piece))
    {
        draw_piece_back(center);
    }
    else if (!is_empty_piece(piece))
    {
        draw_piece_front(center, piece, font);
    }
}

static void draw_selected_hints(const Game *game)
{
    if (game->selected_row < 0 || game->selected_col < 0)
    {
        return;
    }

    for (int r = 0; r < BOARD_ROWS; r++)
    {
        for (int c = 0; c < BOARD_COLS; c++)
        {
            Vector2 center = {
                BOARD_OFFSET_X + c * CELL_SIZE + CELL_SIZE / 2.0f,
                BOARD_OFFSET_Y + r * CELL_SIZE + CELL_SIZE / 2.0f};

            if (can_move_piece(&game->board, game->selected_row, game->selected_col, r, c))
            {
                DrawCircleV(center, 11.0f, COLOR_MOVE_HINT);
            }
            else if (can_capture_piece(&game->board, game->selected_row, game->selected_col, r, c))
            {
                draw_ring_hint(center, COLOR_CAPTURE_HINT, PIECE_RADIUS + 4.0f, PIECE_RADIUS + 8.0f);
            }
        }
    }
}

void draw_game(const Game *game, const Assets *assets)
{
    ClearBackground(RAYWHITE);

    draw_board_background();
    draw_board_grid();
    draw_board_decorations();

    DrawTextEx(
        assets->chineseFontBold,
        "暗棋",
        (Vector2){40, 18},
        30,
        1.0f,
        COLOR_BOARD_BORDER);

    if (game->phase == PHASE_CHOOSE_ORDER)
    {
        draw_order_buttons(assets);
    }

    if (game->phase == PHASE_CHOOSE_ORDER) {
    draw_text_center(
        assets->chineseFont,
        game->status_text,
        (Vector2){SCREEN_WIDTH / 2.0f, 118.0f},
        24.0f,
        COLOR_BOARD_BORDER
    );
    } else {
        DrawTextEx(
            assets->chineseFont,
            game->status_text,
            (Vector2){50, 110},
            24,
            1.0f,
            COLOR_BOARD_BORDER
        );
    }

    Color playerInfoColor = side_color(game->player_side);
    Color computerInfoColor = side_color(game->computer_side);

    char stepText[64];
    snprintf(stepText, sizeof(stepText), "玩家步數：%d", game->player_steps);
    DrawTextEx(assets->chineseFont, stepText, (Vector2){320, 655}, 24, 1.0f, playerInfoColor);

    snprintf(stepText, sizeof(stepText), "電腦步數：%d", game->computer_steps);
    DrawTextEx(assets->chineseFont, stepText, (Vector2){320, 690}, 24, 1.0f, computerInfoColor);

    char sideInfo[64];
    snprintf(sideInfo, sizeof(sideInfo), "玩家顏色：%s", side_text(game->player_side));
    DrawTextEx(assets->chineseFont, sideInfo, (Vector2){620, 655}, 24, 1.0f, playerInfoColor);

    snprintf(sideInfo, sizeof(sideInfo), "電腦顏色：%s", side_text(game->computer_side));
    DrawTextEx(assets->chineseFont, sideInfo, (Vector2){620, 690}, 24, 1.0f, computerInfoColor);

    int hoverRow = -1;
    int hoverCol = -1;

    if (game->phase == PHASE_PLAYING && game->turn == TURN_PLAYER)
    {
        Vector2 mouse = GetMousePosition();
        int row, col;
        if (screen_to_board((int)mouse.x, (int)mouse.y, &row, &col))
        {
            Piece hoverPiece = game->board.cells[row][col];
            if (!is_empty_piece(hoverPiece))
            {
                hoverRow = row;
                hoverCol = col;
            }
        }
    }

    draw_selected_hints(game);

    for (int r = 0; r < BOARD_ROWS; r++)
    {
        for (int c = 0; c < BOARD_COLS; c++)
        {
            draw_piece_cell(
                r,
                c,
                game->board.cells[r][c],
                assets->chineseFontBold,
                game,
                hoverRow,
                hoverCol);
        }
    }
}
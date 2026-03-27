#include "assets.h"
#include "raylib.h"
#include <stdio.h>

int load_assets(Assets *assets)
{   
    const char *FontPath = "assets/fonts/NotoSansTC-Bold.ttf";

    const char *allText =
        "暗棋翻牌狀態玩家電腦回合遊戲結束"
        "剩餘未翻棋子背楚河漢界"
        "將士象車馬包卒"
        "帥仕相俥傌炮兵"
        "點擊一顆尚未翻開的棋子"
        "：0123456789 "
        "狀態電腦翻牌思考中";

    int codepointCount = 0;
    int *codepoints = LoadCodepoints(allText, &codepointCount);

    assets->chineseFont = LoadFontEx(
        FontPath,
        40,
        codepoints,
        codepointCount
    );

    assets->chineseFontBold = LoadFontEx(
        FontPath,
        52,
        codepoints,
        codepointCount
    );

    UnloadCodepoints(codepoints);

    if (assets->chineseFont.texture.id == 0) {
        printf("Failed to load chineseFont\n");
        return 0;
    }

    if (assets->chineseFontBold.texture.id == 0) {
        printf("Failed to load chineseFontBold\n");
        return 0;
    }

    SetTextureFilter(assets->chineseFont.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(assets->chineseFontBold.texture, TEXTURE_FILTER_BILINEAR);

    return 1;
}

void unload_assets(Assets *assets)
{
    UnloadFont(assets->chineseFont);
    UnloadFont(assets->chineseFontBold);
}
#include "assets.h"
#include "raylib.h"
#include <stdio.h>

int load_assets(Assets *assets)
{
    const char *FontPath = "assets/fonts/NotoSansTC-Bold.ttf";

    const char *allText =
        "暗棋玩家電腦回合遊戲結束平手獲勝失敗"
        "剩餘未翻棋子先手後手請先選擇"
        "玩家先手電腦先手"
        "狀態思考中翻牌完成移動棋子吃子成功"
        "已選取重新取消"
        "紅黑帥將仕士相象俥車傌馬炮包兵卒"
        "十步已完成輪到"
        "雙方顏色未定"
        "玩家顏色電腦顏色"
        "玩家步數電腦步數"
        "：0123456789 /"
        "請先選擇玩家先手或電腦先手"
        "玩家先手，請先翻棋或行動";

    int codepointCount = 0;
    int *codepoints = LoadCodepoints(allText, &codepointCount);

    assets->chineseFont = LoadFontEx(FontPath, 34, codepoints, codepointCount);
    assets->chineseFontBold = LoadFontEx(FontPath, 48, codepoints, codepointCount);

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
#ifndef ASSETS_H
#define ASSETS_H

#include "raylib.h"

typedef struct {
    Font chineseFont;
    Font chineseFontBold;
} Assets;

int load_assets(Assets *assets);
void unload_assets(Assets *assets);

#endif
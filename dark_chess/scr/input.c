#include <stdio.h>
#include "input.h"

int get_player_input(int *row, int *col)
{
    printf("Enter row and col (example: 1 3): ");

    if (scanf("%d %d", row, col) != 2) {
        return 0;
    }

    return 1;
}
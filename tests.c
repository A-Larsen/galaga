#include "game.h"

int main(int argc, char **argv)
{

    if (argc > 2) {
        if (strcmp(argv[1], "pickFormationPosition") == 0) {
            Grid grid;
            srand(time(NULL));

            uint8_t i = 0;
            while(i < atoi(argv[2])) {
                uint8_t a = pickFormationPosition(ENEMY_BEE, grid.formation);
                printf("%d\n", a);
                i++;
            }
            printf("SUCCESS\n");
            return 0;
        }

    }

    return 0;
}

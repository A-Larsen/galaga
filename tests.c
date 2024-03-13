#include "game.h"

int main(int argc, char **argv)
{

    if (argc > 2) {
        if (strcmp(argv[1], "pickFormationPosition") == 0) {
            Grid grid;
            srand(time(NULL));

            uint16_t size = atoll(argv[2]);
            uint8_t i = 0;
            uint8_t *picked = malloc(sizeof(uint8_t) * size);
            bool pass = true;

            while(i < size) {
                printf("%d: ", i);
                int a = -1;
                a = pickFormationPosition(ENEMY_BEE, grid.formation);

                printf("%d\n", a);

                if (picked[a] == 1) {
                    pass = false;
                    break;
                }

                picked[a] = 1;

                /* for (uint16_t j = 0; j < size; ++j) { */
                /*     if ((j != i) && (picked[i] == picked[j])) { */
                /*         pass = false; */
                /*         break; */
                /*     } */
                /* } */
                i++;
            }

            if (pass) {
                printf("SUCCESS\n");
                return 0;

            } else {
                printf("FAIL\n");
                return 1;
            }

            free(picked);
        }

    }

    return 0;
}

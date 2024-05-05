/* 
 * Copyright (C) 2024  Austin Larsen
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "game.h"

int main(int argc, char **argv)
{

    if (argc > 1) {
        if (strcmp(argv[1], "pickFormationPosition") == 0) {
            Grid grid;
            srand(time(NULL));

            uint16_t size = 20;

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
        } else {
            fprintf(stderr, "not an argument");
            return 1 ;
        }

    }

    return 0;
}

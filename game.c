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

void interpolate(float *x, int y, int s, SDL_Point p1, SDL_Point p2) {
    if ((p2.y - p1.y) == 0) return;
    *x = s + (float)((p2.x - p1.x) * (y - p1.y)) / (float)(p2.y - p1.y);
}

void
setColor(SDL_Renderer *renderer, uint8_t color)
{
    const SDL_Color colors[] = {
        [COLOR_RED] = {.r = 217, .g = 100, .b = 89, .a = 255},
        [COLOR_WHITE] = {.r = 255, .g = 255, .b = 255, .a = 255},
        [COLOR_GREEN] = {.r = 88, .g = 140, .b = 126, .a = 255},
        [COLOR_BLUE] = {.r = 146, .g = 161, .b = 185, .a = 255},
        [COLOR_ORANGE] = {.r = 242, .g = 174, .b = 114, .a = 255},
        [COLOR_GREY] = {.r = 89, .g = 89, .b = 89, .a = 89},
        [COLOR_BLACK] = {.r = 0, .g = 0, .b = 0, .a = 0},
    };

    SDL_SetRenderDrawColor(renderer, colors[color].r, colors[color].g,
                           colors[color].b, colors[color].a);
}

void
updateGridPosition(Grid *grid, uint64_t frame)
{
    uint64_t update_rate = 450;

    grid->space = 50 + sin(((frame % update_rate) /
                           (float)update_rate) * TAU) * 4;

    float width = ((float)grid->space * (float)FORMATION_WIDTH);
    uint8_t height = ((ENEMY_SIZE_PX + grid->space) * FORMATION_HEIGHT);

    grid->start.x = grid->space + ((grid->space - ENEMY_SIZE_PX) / 2.0f) +
              ((float)SCREEN_WIDTH_PX / 2.0f) - (width / 2.0f) -grid->space;

    grid->start.y = 10;
}

void
getGridPosition(SDL_Point *point, Grid grid,  uint8_t x, uint8_t y)
{
    point->x = grid.start.x + x * grid.space;
    point->y = grid.start.y + y * grid.space;
}

void
drawFormationGrid(SDL_Renderer *renderer, Grid grid, uint64_t frame)
{

    for (uint8_t y = 0; y < FORMATION_HEIGHT; ++y) {
        for (uint8_t x = 0; x < FORMATION_WIDTH; ++x) {
            SDL_Point point;

            getGridPosition(&point, grid, x, y);

            SDL_Rect rect = {
                .x = point.x,
                .y = point.y,
                .w = ENEMY_SIZE_PX,
                .h = ENEMY_SIZE_PX,
            };
            
            SDL_RenderDrawRect(renderer, &rect);
        }
    }
}

void
enemyToFormation(FPosition *point, Grid *grid, uint64_t frame, FPosition source,
                 bool *enteredFormation, uint8_t pickedPosition,
                 SDL_Point destination)
{
    SDL_Point a;

    getGridPosition(&a, *grid, destination.x, destination.y);

    if (!(*enteredFormation)) {
        SDL_Point b = {
            .x = source.x,
            .y = source.y
        };

        interpolate(&point->x, point->y, source.x, b, a);

        point->y -= 1;

        if (point->y < a.y) {
            *enteredFormation = true;
            grid->formation[pickedPosition] = 1;
        }
        return;
    }

    point->y = a.y;
    point->x = a.x;
}

void
drawFighter(SDL_Renderer *renderer, SDL_Point point)
{
    SDL_Rect rect = {
        .x = point.x,
        .y = point.y,
        .w = FIGHTER_WIDTH_PX,
        .h = FIGHTER_HEIGHT_PX
    };

    setColor(renderer, COLOR_GREEN);
    SDL_RenderFillRect(renderer, &rect);
}

void
drawEnemy(SDL_Renderer *renderer, FPosition point)
{
    SDL_Rect rect = {
        .x = point.x,
        .y = point.y,
        .w = ENEMY_SIZE_PX,
        .h = ENEMY_SIZE_PX
    };

    setColor(renderer, COLOR_BLUE);
    SDL_RenderFillRect(renderer, &rect);
}

void
drawNoiseCircle(SDL_Renderer *renderer, SDL_Point center,
           uint8_t px_size, uint16_t radius, float accuracy)
{
    for(float i = 0; i <= TAU + accuracy; i += accuracy) {
        float n = (float)rand() / (float)RAND_MAX;

        float x = sinf(n * TAU);
        float y = cosf(n * TAU);

        if (!(i > 0)) continue;

        SDL_Rect rect = {
            .x = x * radius + center.x,
            .y = y * radius + center.y,
            .w = px_size,
            .h = px_size
        };

        SDL_RenderFillRect(renderer, &rect);
    }
}

bool
drawExplosion(SDL_Renderer *renderer, uint64_t frame)
{
    static float i = 0;
    SDL_Point center = {.x = 400, .y = 400};

    if (i >= 55) return false;

    uint8_t gap = 3;

    setColor(renderer, (int)i % 4 ? COLOR_WHITE : COLOR_RED);

    for (uint8_t j = 0; j < i; ++j) {
        drawNoiseCircle(renderer, center, 3, j, 4);
    }

    i += .9f;
    return true;
}


void
enemyMove(FPosition *point, bool invert_x, bool invert_y, float radians)
{
    point->x += sinf(radians) * (invert_x ? -1 : 1);
    point->y += cosf(radians) * (invert_y ? -1 : 1);
}

bool
enemyEntrance(uint8_t p1, uint8_t p2, uint64_t frame, FPosition *rect)
{
    const int padding = 50;

    if (rect->init) {

        switch(p1) {
            case BOTTOM: rect->radians = M_PI_2; break;
            case TOP: rect->radians = 0; break;
        }

        rect->start_radians = rect->radians;

        if (p1 == BOTTOM && p2 == LEFT) {
            rect->x = -padding - ENEMY_SIZE_PX;
            rect->y = (float)SCREEN_HEIGHT_PX - ENEMY_SIZE_PX - padding;
        }

       if (p1 == BOTTOM && p2 == RIGHT) {
            rect->x = SCREEN_WIDTH_PX + padding;
            rect->y = (float)SCREEN_HEIGHT_PX - ENEMY_SIZE_PX - padding;
       }

       if (p1 == TOP && p2 == CENTER_LEFT) {
            rect->x = ((float)SCREEN_WIDTH_PX / 2.0f) - ENEMY_SIZE_PX - 30;
            rect->y = -padding - ENEMY_SIZE_PX;
       }

        rect->init = false;
    }


    switch(p1) {
        case BOTTOM: {
            enemyMove(rect, p2 == LEFT ? 0 : 1, 0, rect->radians);

            float r2 = 0;

            if (rect->radians - rect->start_radians < M_PI_2) {
                rect->radians += .003f;
                r2 = rect->radians;
            } else{
                rect->radians += .016f;
                if (rect->radians - r2 >= M_3PI) {
                    rect->init = true;
                    return false;
                }
            }
            break;
        }
        case TOP: {
            enemyMove(rect, p2 == CENTER_LEFT ? 1 : 0, 0, rect->radians);

            float r2 = 0;

            if (rect->radians - rect->start_radians > -M_PI_4) {
                rect->radians -= .002f;
                r2 = rect->radians;
            } else {
                rect->radians -= .016f;
                if (rect->radians - r2 <= -TAU) {
                    rect->init = true;
                    return false;
                }
            }
            break;
        }
    }

    return true;
}

uint8_t
pickFormationPosition(uint8_t type, bool *formation)
{

    switch(type) {
        case ENEMY_BEE: {
            int i = -1;

            while (true) {
                float r = (float)rand() / (float)RAND_MAX;
                /* i = r * ((FORMATION_WIDTH * 2) - 1) + (FORMATION_WIDTH * 3); */
                i = r * ((FORMATION_WIDTH * 2)) + (FORMATION_WIDTH * 3);
                if (!formation[i]) break;
            }

            formation[i] = 1;
            return i;
        }
    }
    return 0;
}

FPosition *
EnemyEnter(uint8_t id, uint8_t type, Grid *grid, uint8_t enter,
           uint64_t frame) {

    static EnemyInfo info[FORMATION_WIDTH] = {[0 ... FORMATION_WIDTH - 1] = {
        .pickedPosition = 0,
        .enteredFormation = false,
        .entering = true,
        .source = {0},
        .position = {
            .x = 0,
            .y = 0,
            .radians = 0,
            .init = true
        },
        .formation = {0}
    }};

    EnemyInfo *infop = &info[id];

    if (infop->entering) {
        infop->entering = enemyEntrance(BOTTOM, enter, frame,
                                        &infop->position);
    } else if (!infop->pickedPosition) {
        SDL_Point p;
        uint8_t i = pickFormationPosition(type, grid->formation);
        infop->formation.x = i % FORMATION_WIDTH;
        infop->formation.y = floor((float)i / (float)FORMATION_WIDTH);
        printf("i: %d\n", i);
        printf("x: %d, y: %d\n", infop->formation.x, infop->formation.y);
        memcpy(&infop->source, &infop->position, sizeof(FPosition));
        infop->pickedPosition = i;
    } else{
        enemyToFormation(&infop->position, grid, frame, infop->source,
                         &infop->enteredFormation, infop->pickedPosition,
                         infop->formation);
    }
    return &infop->position;
}

uint8_t
updateMain(Game *game, uint64_t frame, SDL_KeyCode key, bool keydown)
{
    static bool init = true;

    if (init) {
        srand(time(NULL));
        init = false;
    }

    updateGridPosition(&game->grid, frame);

    static SDL_Point fighter_pos = {
        .x = 10,
        .y = SCREEN_HEIGHT_PX - FIGHTER_HEIGHT_PX - 10
    };

    // make sure left and right enemy do not choose the same spot
    FPosition *position1 = EnemyEnter(0, ENEMY_BEE, &game->grid, RIGHT, frame);
    FPosition *position2 = EnemyEnter(1, ENEMY_BEE, &game->grid, LEFT, frame);

    drawExplosion(game->renderer, frame);
    drawFighter(game->renderer, fighter_pos);
    drawEnemy(game->renderer, *position1);
    drawEnemy(game->renderer, *position2);
    SDL_RenderDrawLine(game->renderer, SCREEN_WIDTH_PX / 2, 0,
                       SCREEN_WIDTH_PX / 2, SCREEN_HEIGHT_PX);
    drawFormationGrid(game->renderer, game->grid, frame);

    return UPDATE_MAIN;
}

void
GameInit(Game *game)
{
    memset(game, 0, sizeof(Game));

    END(SDL_Init(SDL_INIT_VIDEO) != 0, "Could not create texture",
        SDL_GetError());

    END(TTF_Init() != 0, "Could not initialize TTF", TTF_GetError());

    END(IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG, "Could not initialize PNG",
                 TTF_GetError());

    game->window = SDL_CreateWindow("galaga", SDL_WINDOWPOS_UNDEFINED, 
                     SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH_PX, 
                     SCREEN_HEIGHT_PX, SDL_WINDOW_SHOWN);

    END(game->window == NULL, "Could not create window", SDL_GetError());

    game->renderer = SDL_CreateRenderer(game->window, 0,
                                        SDL_RENDERER_SOFTWARE);

    END(game->renderer == NULL, "Could not create renderer", SDL_GetError());
}

void
GameUpdate(Game *game, const uint16_t lps, const uint16_t fps)
{
    uint64_t frame = 0;
    bool keydown = false;
    uint8_t update_id = 0;
    Update_callback update;
    float lsp_mspd = (1.0f / (float)lps) * 1000.0f;

    SDL_Rect background_rect = {
        .x = 0,
        .y = 0,
        .w = SCREEN_WIDTH_PX,
        .h = SCREEN_HEIGHT_PX
    };

    while (!game->quit) {
        uint32_t start = SDL_GetTicks();

        switch (update_id) {
            case UPDATE_MAIN: update = updateMain; break;
        }

        setColor(game->renderer, COLOR_GREY);
        SDL_RenderClear(game->renderer);
        setColor(game->renderer, COLOR_BLACK);
        SDL_RenderFillRect(game->renderer, &background_rect);

        SDL_Event event;
        SDL_KeyCode key = 0;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYDOWN: {
                    if (event.key.repeat == 0) {
                      key = event.key.keysym.sym;
                      keydown = true;
                    }

                    break;
                }

                case SDL_KEYUP: keydown = false; break;
                case SDL_QUIT: game->quit = true; break;
            }
        }

        update_id = update(game, frame, key, keydown);

        uint32_t end = SDL_GetTicks();
        uint32_t elapsed_time = end - start;

        if (elapsed_time < lsp_mspd) SDL_Delay(lsp_mspd - elapsed_time);

        if (frame % (lps / fps) == 0) SDL_RenderPresent(game->renderer);

        frame++;
    }
}

void
GameQuit(Game *game)
{
    SDL_DestroyWindow(game->window);
    SDL_DestroyRenderer(game->renderer);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

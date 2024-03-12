#include <stdio.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>

#define TAU ((float)(M_PI * 2.0f))
#define M_3PI ((float)(M_PI * 3.0f))
#define SCREEN_WIDTH_PX 600U
#define SCREEN_HEIGHT_PX 800U
#define FIGHTER_WIDTH_PX 45
#define FIGHTER_HEIGHT_PX 58
#define ENEMY_SIZE_PX 30
#define UP TOP
#define DOWN BOTTOM
#define FORMATION_WIDTH 10
#define FORMATION_HEIGHT 5
#define FORMATION_SIZE 50
#define ENEMY_BOSS_START 0
#define ENEMY_BOSS_END 9
#define ENEMY_BUTTERFLY_START 10
#define ENEMY_BUTTERFLY_END 29
#define ENEMY_BEE_START 30
#define ENEMY_BEE_END 39

#define END(check, str1, str2) \
    if (check) { \
        assert(check); \
        fprintf(stderr, "%s\n%s", str1, str2); \
        exit(1); \
    } \

enum {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_ORANGE, COLOR_GREY,
      COLOR_WHITE, COLOR_BLACK, COLOR_SIZE};

enum {TOP, BOTTOM, LEFT, RIGHT, CENTER_LEFT, CENTER_RIGHT};

enum {ENEMY_BEE, ENEMY_BUTTERFLY, ENEMY_BOSS};

enum {UPDATE_MAIN};

typedef struct _Game {
    uint8_t level;
    uint64_t score;
    SDL_Renderer *renderer;
    SDL_Window *window;
    bool quit;
    bool canDraw;
} Game;

typedef struct _FPoint {
    float x;
    float y;
    float radians;
    float start_radians;
    bool init;
} FPoint;

typedef struct _Bee {
    FPoint position;
    bool entering;
    SDL_Point formation;
    FPoint source;
    bool pickedPosition;
} Bee;

typedef uint8_t (*Update_callback)(Game *game, uint64_t frame, SDL_KeyCode key,
                                   bool keydown);

static bool formation[FORMATION_SIZE]; // 10 x 5
// row 1  (0-9)    boss
// row 2  (10-19)  butterfly
// row 3  (20-29)  butterfly
// row 4  (30-39)  bees
// row 5: (40-49)  bees

void interpolate(float *x, int y, int s, SDL_Point p1, SDL_Point p2) {
    if ((p2.y - p1.y) == 0) return;
    *x = s + (float)((p2.x - p1.x) * (y - p1.y)) / (float)(p2.y - p1.y);
}

void
BeeInit(Bee *bee) {
    bee->position.x = 0;
    bee->position.y = 0;
    bee->position.radians = 0;
    bee->position.init = true;
    bee->entering = true;
    bee->pickedPosition = false;
    memset(&bee->formation, 0, sizeof(SDL_Point));
    memset(&bee->source, 0, sizeof(FPoint));
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
getGridPosition(bool *formation, uint64_t frame, SDL_Point *point, uint8_t x,
                uint8_t y)
{
    static uint8_t space = 50;
    static float i = 0;
    static SDL_Point start = {0};
    uint64_t update_rate = 30000;

    space = 50 + sin(((frame % update_rate) / (float)update_rate) * TAU) * 4;

    float width = ((float)space * (float)FORMATION_WIDTH);
    uint8_t height = ((ENEMY_SIZE_PX + space) * FORMATION_HEIGHT);

    start.x = space + ((space - ENEMY_SIZE_PX) / 2.0f) +
              ((float)SCREEN_WIDTH_PX / 2.0f) - (width / 2.0f) -space;

    start.y = 10;

    point->x = start.x + x * space;
    point->y = start.y + y * space;
}

void
drawFormationGrid(SDL_Renderer *renderer, uint64_t frame, bool *formation)
{

    for (uint8_t y = 0; y < FORMATION_HEIGHT; ++y) {
        for (uint8_t x = 0; x < FORMATION_WIDTH; ++x) {
            SDL_Point point;
            getGridPosition(formation, frame, &point, x, y);
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
enemyToFormation(FPoint *point, uint64_t frame, FPoint source,
                 SDL_Point destination)
{

    SDL_Point a;
    SDL_Point b = {
        .x = source.x,
        .y = source.y
    };

    getGridPosition(formation, frame, &a, destination.x, destination.y);
    interpolate(&point->x, point->y, source.x, b, a);
    
    point->y += (point->y > a.y) ? -0.01f : 0.01f;
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
drawEnemy(SDL_Renderer *renderer, FPoint point)
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

    i += .1f;
    return true;
}


void
enemyMove(FPoint *point, bool invert_x, bool invert_y, float radians)
{
    point->x += sinf(radians) * (invert_x ? -1 : 1);
    point->y += cosf(radians) * (invert_y ? -1 : 1);
}

bool
enemyEntrance(uint8_t p1, uint8_t p2, uint64_t frame, FPoint *rect)
{
    const int padding = 50;

    if (!(frame % 30 == 0)) return true;
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
pickFormationPosition(uint8_t type)
{
    float r = (float)rand() / (float)RAND_MAX;

    switch(type) {
        case ENEMY_BEE: {
            return r * ((FORMATION_WIDTH * 2) - 1) + (FORMATION_WIDTH * 3);
        }
    }
    return 0;
}

void
BeeEnter(Bee *bee, uint8_t enter, uint64_t frame) {
    if (bee->entering) {
        bee->entering = enemyEntrance(BOTTOM, enter, frame, &bee->position);
    } else if (!bee->pickedPosition) {
        SDL_Point p;
        uint8_t position = pickFormationPosition(ENEMY_BEE);
        bee->formation.x = position % FORMATION_WIDTH;
        bee->formation.y = floor((float)position / (float)FORMATION_WIDTH);
        printf("position: %d\n", position);
        printf("x: %d, y: %d\n", bee->formation.x, bee->formation.y);
        memcpy(&bee->source, &bee->position, sizeof(FPoint));
        bee->pickedPosition = true;
    } else{
        enemyToFormation(&bee->position, frame, bee->source, bee->formation);
    }
}

static uint8_t
updateMain(Game *game, uint64_t frame, SDL_KeyCode key, bool keydown)
{
    static bool init = true;
    static Bee bee1;
    static Bee bee2;

    if (init) {
        srand(time(NULL));
        init = false;
        BeeInit(&bee1);
        BeeInit(&bee2);
    }

    static SDL_Point fighter_pos = {
        .x = 10,
        .y = SCREEN_HEIGHT_PX - FIGHTER_HEIGHT_PX - 10
    };

    // make sure left and right enemy do not choose the same spot
    BeeEnter(&bee1, RIGHT, frame);
    BeeEnter(&bee2, LEFT, frame);

    if (game->canDraw) {
        drawExplosion(game->renderer, frame);
        drawFighter(game->renderer, fighter_pos);

        drawEnemy(game->renderer, bee1.position);
        drawEnemy(game->renderer, bee2.position);

        SDL_RenderDrawLine(game->renderer, SCREEN_WIDTH_PX / 2, 0,
                           SCREEN_WIDTH_PX / 2, SCREEN_HEIGHT_PX);
        drawFormationGrid(game->renderer, frame, formation);
    }

    return UPDATE_MAIN;
}

void
Game_Init(Game *game)
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
Game_Update(Game *game, const uint32_t lps, const uint32_t fps)
{
    uint64_t frame = 0;
    bool keydown = false;
    uint8_t update_id = 0;
    Update_callback update;
    float mspd = (1.0f / (float)lps) * 1000.0f;

    SDL_Rect background_rect = {
        .x = 0,
        .y = 0,
        .w = SCREEN_WIDTH_PX,
        .h = SCREEN_HEIGHT_PX
    };

    while (!game->quit) {
        uint32_t start = SDL_GetTicks();
        game->canDraw = frame % (lps / fps) == 0;

        switch (update_id) {
            case UPDATE_MAIN: update = updateMain; break;
        }

        if (game->canDraw) {
            setColor(game->renderer, COLOR_GREY);
            SDL_RenderClear(game->renderer);
            setColor(game->renderer, COLOR_BLACK);
            SDL_RenderFillRect(game->renderer, &background_rect);

        }

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

        if (elapsed_time < mspd) {
            elapsed_time = mspd - elapsed_time;
            SDL_Delay(elapsed_time);
        } 


        if (game->canDraw)
            SDL_RenderPresent(game->renderer);

        frame++;
    }
}

void
Game_Quit(Game *game)
{
    SDL_DestroyWindow(game->window);
    SDL_DestroyRenderer(game->renderer);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

int main(void)
{
    Game game;
    Game_Init(&game);
    Game_Update(&game, 1200, 60);
    Game_Quit(&game);
}

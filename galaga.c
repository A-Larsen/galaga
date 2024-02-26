#include <stdio.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <stdbool.h>

#define TAU ((float)(M_PI * 2.0f))
#define M_3PI ((float)(M_PI * 3.0f))
#define SCREEN_WIDTH_PX 600U
#define SCREEN_HEIGHT_PX 800U
#define FIGHTER_WIDTH_PX 45
#define FIGHTER_HEIGHT_PX 58
#define BEE_WIDTH_PX 40
#define BEE_HEIGHT_PX 40
#define UP TOP
#define DOWN BOTTOM

#define END(check, str1, str2) \
    if (check) { \
        assert(check); \
        fprintf(stderr, "%s\n%s", str1, str2); \
        exit(1); \
    } \

enum {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_ORANGE, COLOR_GREY,
      COLOR_WHITE, COLOR_BLACK, COLOR_SIZE};

enum {TOP, BOTTOM, LEFT, RIGHT, CENTER_LEFT, CENTER_RIGHT};


enum {UPDATE_MAIN};

typedef struct _Game {
    uint8_t level;
    uint64_t score;
    SDL_Renderer *renderer;
    SDL_Window *window;
    bool quit;
    bool canDraw;
} Game;

typedef struct _FRect {
    float x;
    float y;
    int w;
    int h;
    float radians;
    float start_radians;
    bool init;
} FRect;

typedef uint8_t (*Update_callback)(Game *game, uint64_t frame, SDL_KeyCode key,
                                   bool keydown);

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
drawBee(SDL_Renderer *renderer, FRect point)
{

    SDL_Rect rect = {
        .x = point.x,
        .y = point.y,
        .w = BEE_WIDTH_PX,
        .h = BEE_HEIGHT_PX
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
enemyMove(FRect *point, bool invert_x, bool invert_y, float radians)
{
    point->x += sinf(radians) * (invert_x ? -1 : 1);
    point->y += cosf(radians) * (invert_y ? -1 : 1);
}

bool
enemyEntrance(uint8_t p1, uint8_t p2, uint64_t frame, FRect *rect)
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
            rect->x = -padding - rect->w;
            rect->y = (float)SCREEN_HEIGHT_PX - rect->w - padding;
        }

       if (p1 == BOTTOM && p2 == RIGHT) {
            rect->x = SCREEN_WIDTH_PX + padding;
            rect->y = (float)SCREEN_HEIGHT_PX - rect->w - padding;
       }

       if (p1 == TOP && p2 == CENTER_LEFT) {
            rect->x = ((float)SCREEN_WIDTH_PX / 2.0f) - rect->w - 30;
            rect->y = -padding - rect->h;
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

static uint8_t
updateMain(Game *game, uint64_t frame, SDL_KeyCode key, bool keydown)
{
    static bool b1IsEntering = true;
    static bool b2IsEntering = true;


    static FRect bee1_pos = {.x = 0, .y = 0, .w = BEE_WIDTH_PX,
                            .h = BEE_HEIGHT_PX, .radians = 0, .init = true};

    static FRect bee2_pos = {.x = 0, .y = 0, .w = BEE_WIDTH_PX,
                            .h = BEE_HEIGHT_PX, .radians = 0, .init = true};

    static SDL_Point fighter_pos = {
        .x = 10,
        .y = SCREEN_HEIGHT_PX - FIGHTER_HEIGHT_PX - 10
    };


    if (b1IsEntering) {
        b1IsEntering = enemyEntrance(TOP, CENTER_LEFT, frame, &bee1_pos);
    }

    if (b2IsEntering) {
        b2IsEntering = enemyEntrance(BOTTOM, LEFT, frame, &bee2_pos);
    }


    if (game->canDraw) {
        drawExplosion(game->renderer, frame);
        drawFighter(game->renderer, fighter_pos);
        drawBee(game->renderer, bee1_pos);
        drawBee(game->renderer, bee2_pos);
        SDL_RenderDrawLine(game->renderer, SCREEN_WIDTH_PX / 2, 0,
                           SCREEN_WIDTH_PX / 2, SCREEN_HEIGHT_PX);
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

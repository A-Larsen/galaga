#include <stdio.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <stdbool.h>

#define TAU ((float)(M_PI * 2.0f))
#define SCREEN_WIDTH_PX 1000U
#define SCREEN_HEIGHT_PX 800U
#define FIGHTER_WIDTH_PX 45
#define FIGHTER_HEIGHT_PX 58
#define BEE_WIDTH_PX 40
#define BEE_HEIGHT_PX 40

#define END(check, str1, str2) \
    if (check) { \
        assert(check); \
        fprintf(stderr, "%s\n%s", str1, str2); \
        exit(1); \
    } \

enum {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_ORANGE, COLOR_GREY,
      COLOR_WHITE, COLOR_BLACK, COLOR_SIZE};

/* enum {ENTER_BOTTOM_LEFT, ENTER_BOTTOM_RIGHT}; */

enum {UP, DOWN};

enum {LEFT, RIGHT};

enum {UPDATE_MAIN};


typedef struct _Game {
    uint8_t level;
    uint64_t score;
    SDL_Renderer *renderer;
    SDL_Window *window;
} Game;

typedef struct _FPoint {
    float x;
    float y;
} FPoint;

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
    setColor(renderer , COLOR_GREEN);

    SDL_Rect rect = {
        .x = point.x,
        .y = point.y,
        .w = FIGHTER_WIDTH_PX,
        .h = FIGHTER_HEIGHT_PX
    };

    SDL_RenderFillRect(renderer, &rect);
}

void
drawBee(SDL_Renderer *renderer, FPoint point)
{
    setColor(renderer , COLOR_BLUE);

    SDL_Rect rect = {
        .x = point.x,
        .y = point.y,
        .w = BEE_WIDTH_PX,
        .h = BEE_HEIGHT_PX
    };

    SDL_RenderFillRect(renderer, &rect);
}

bool
getCirclePoint(SDL_Point *point, SDL_Point center,
               uint16_t radius, uint8_t i)
{
    float accuracy = .02f;

    point->x = sinf(i * accuracy);
    point->y = cosf(i * accuracy);

    if (i <= TAU + accuracy) return false;

    return true;
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

    setColor(renderer , (int)i % 4 ? COLOR_WHITE : COLOR_RED);

    for (uint8_t j = 0; j < i; ++j) {
        drawNoiseCircle(renderer, center, 3, j, 4);
    }

    i += .18f;
    return true;
}


void
enemyMove(FPoint *point, uint8_t p1, uint8_t p2, float radians)
{
    point->x += sinf(radians) * ((p1 == RIGHT) ? 1 : -1);
    point->y += cosf(radians) * ((p2 == UP)  ? 1 : -1);
}

bool
enemyEntrance(uint8_t p1, uint8_t p2, uint64_t frame, FPoint *point)
{
    const float deg90 = (float)90 / (180.0f / M_PI);
    const int padding = 10;
    static bool init = true;

    if (init) {
        if (p1 == LEFT && p2 == UP) {
            point->x = -50;
            point->y = (float)SCREEN_HEIGHT_PX - BEE_HEIGHT_PX - padding;
        }

       if (p1 == RIGHT && p2 == UP) {
            point->x = SCREEN_WIDTH_PX - BEE_WIDTH_PX - 10;
            point->y = (float)SCREEN_HEIGHT_PX - BEE_WIDTH_PX - padding;
        }

        init = false;
    }


    if (p2 == UP) {

        static float radians = deg90;

        if (frame % 4 == 0) {

            enemyMove(point, p1 == RIGHT ? LEFT : RIGHT, p2, radians);

            if (radians < (M_PI / 2) + deg90) {
                radians += .003f;
            } else{
                radians += .016f;
                if (radians >= (2.5 * M_PI) + deg90) {
                    init = true;
                    return false;
                }
            }
        }
    }

    return true;
}

static uint8_t
updateMain(Game *game, uint64_t frame, SDL_KeyCode key, bool keydown)
{
    static bool isEntering = true;

    drawExplosion(game->renderer, frame);

    static FPoint bee_pos = {0};

    static SDL_Point fighter_pos = {
        .x = 10,
        .y = SCREEN_HEIGHT_PX - FIGHTER_HEIGHT_PX - 10
    };

    drawFighter(game->renderer, fighter_pos);

    if (isEntering) 
        isEntering = enemyEntrance(LEFT, UP, frame, &bee_pos);

    drawBee(game->renderer, bee_pos);


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
Game_Update(Game *game, const uint32_t fps)
{
    uint64_t frame = 0;
    bool quit = false;
    bool keydown = false;
    uint8_t update_id = 0;
    Update_callback update;
    float mspd = (1.0f / (float)fps) * 1000.0f;

    SDL_Rect background_rect = {
        .x = 0,
        .y = 0,
        .w = SCREEN_WIDTH_PX,
        .h = SCREEN_HEIGHT_PX
    };

    while (!quit) {
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
                case SDL_QUIT: quit = true; break;
            }
        }

        update_id = update(game, frame, key, keydown);

        uint32_t end = SDL_GetTicks();
        uint32_t elapsed_time = end - start;

        if (elapsed_time < mspd) {
            elapsed_time = mspd - elapsed_time;
            SDL_Delay(elapsed_time);
        } 

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
    Game_Update(&game, 1000);
    Game_Quit(&game);
}

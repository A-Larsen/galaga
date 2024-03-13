#ifndef _GAME_H_
#define _GAME_H_

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

enum /* color */ {COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_ORANGE, COLOR_GREY, COLOR_WHITE, COLOR_BLACK, COLOR_SIZE};
enum /* cardinal */ {TOP, BOTTOM, LEFT, RIGHT, CENTER_LEFT, CENTER_RIGHT};
enum /*enemy types */ {ENEMY_BEE, ENEMY_BUTTERFLY, ENEMY_BOSS};
enum /* update callback ids */ {UPDATE_MAIN};

typedef struct _FPoint {
    bool init;
    float x;
    float y;
    float radians;
    float start_radians;
} FPosition;

typedef struct _Grid {
    bool formation[FORMATION_SIZE]; // 10 x 5
    SDL_Point start;
    uint8_t space;
    // row 1  (0-9)    boss
    // row 2  (10-19)  butterfly
    // row 3  (20-29)  butterfly
    // row 4  (30-39)  bees
    // row 5: (40-49)  bees
} Grid;

typedef struct _Game {
    bool quit;
    bool canDraw;
    uint8_t level;
    uint64_t score;
    Grid grid;
    SDL_Renderer *renderer;
    SDL_Window *window;
} Game;

typedef struct _EnemyInfo {
    bool enteredFormation;
    bool entering;
    SDL_Point formation;
    FPosition source;
    FPosition position;
    uint8_t pickedPosition;
} EnemyInfo;

typedef uint8_t (*Update_callback)(Game *game, uint64_t frame, SDL_KeyCode key, bool keydown);

void interpolate(float *x, int y, int s, SDL_Point p1, SDL_Point p2);
void setColor(SDL_Renderer *renderer, uint8_t color);
void updateGridPosition(Grid *grid, uint64_t frame);
void getGridPosition(SDL_Point *point, Grid grid,  uint8_t x, uint8_t y);
void drawFormationGrid(SDL_Renderer *renderer, Grid grid, uint64_t frame);
void enemyToFormation(FPosition *point, Grid *grid, uint64_t frame, FPosition source, bool *enteredFormation, uint8_t pickedPosition, SDL_Point destination);
void drawFighter(SDL_Renderer *renderer, SDL_Point point);
void drawEnemy(SDL_Renderer *renderer, FPosition point);
void drawNoiseCircle(SDL_Renderer *renderer, SDL_Point center, uint8_t px_size, uint16_t radius, float accuracy);
bool drawExplosion(SDL_Renderer *renderer, uint64_t frame);
void enemyMove(FPosition *point, bool invert_x, bool invert_y, float radians);
bool enemyEntrance(uint8_t p1, uint8_t p2, uint64_t frame, FPosition *rect);
uint8_t pickFormationPosition(uint8_t type, bool *formation);
FPosition * EnemyEnter(uint8_t id, uint8_t type, Grid *grid, uint8_t enter, uint64_t frame);
uint8_t updateMain(Game *game, uint64_t frame, SDL_KeyCode key, bool keydown);
void GameInit(Game *game);
void GameUpdate(Game *game, const uint16_t lps, const uint16_t fps);
void GameQuit(Game *game);

#endif // _GAME_H_

#include "game.h"

int main(void)
{
    Game game;
    GameInit(&game);
    GameUpdate(&game, 220, 60);
    GameQuit(&game);
}

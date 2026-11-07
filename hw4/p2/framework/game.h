#ifndef HW4_FLAPPY_GAME_H
#define HW4_FLAPPY_GAME_H
#include "../flappy.h"
void gameInit(Game *game);
void gameRestart(Game *game);
void gameStep(Game *game, Input input, float dt);
Input gameDemoInput(const Game *game);
#endif

#ifndef HW4_FLAPPY_H
#define HW4_FLAPPY_H
#include <stdbool.h>

/* A small reproducible generator; all arithmetic fits a 32-bit unsigned int. */
enum {
    RANDOM_INITIAL_SEED = 100,
    RANDOM_MULTIPLIER = 109,
    RANDOM_INCREMENT = 1021,
    RANDOM_MODULUS = 32768
};
/* seed > 0 resets the state before taking one step; seed == 0 continues.
 * Return (RANDOM_MULTIPLIER * state + RANDOM_INCREMENT) % RANDOM_MODULUS.
 * Store the state in one function-local static unsigned int. */
unsigned int nextRandom(unsigned int seed);

/* World coordinates; use these names instead of repeating numeric values. */
enum {
    VIEW_W = 768,
    VIEW_H = 432,
    FLOOR_Y = 384,
    BIRD_X = 192,
    BIRD_WIDTH = 28,
    BIRD_HEIGHT = 22,
    PIPE_COUNT = 4,
    PIPE_WIDTH = 64,
    PIPE_GAP_HEIGHT = 156,
    PIPE_SPACING = 256,
    FIRST_PIPE_X = 576,
    PHYSICS_HZ = 120
};
#define STEP (1.0f / PHYSICS_HZ)
#define PIPE_SPEED 140.0f

typedef enum { READY, PLAYING, DEAD } Status;
typedef struct {
    float m_x;
    float m_y;
    float m_width;
    float m_height;
} Box;
typedef struct {
    float m_x;
    float m_gap_y;
    bool m_scored;
} Pipe;
typedef struct {
    bool m_flap_pressed;
} Input;
typedef struct {
    Pipe m_pipes[PIPE_COUNT];
    float m_bird_y;
    float m_vy;
    float m_elapsed;
    int m_score;
    int m_best;
    Status m_status;
} Game;

/* Three student functions. No dynamic allocation or input handling is needed. */
void advancePipes(Game *game, float dt);
bool birdHitsObstacle(const Game *game);
int scorePassedPipes(Game *game);

/* Provided helpers. recyclePipe places the slot after the rightmost pipe,
 * chooses a valid gap, and clears its scored flag. index must be in range. */
void recyclePipe(Game *game, int index);
Box gameBirdBox(const Game *game);
bool boxesOverlap(Box left, Box right);
#endif

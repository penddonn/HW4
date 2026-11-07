#ifndef HW4_SNAKE_H
#define HW4_SNAKE_H
/* Portable C17 game model. No terminal or operating-system headers here. */
#include <stdbool.h>
#include <stddef.h>

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

enum { MIN_BOARD_SIZE = 6, BOARD_WALL_THICKNESS = 1 };

typedef struct {
    int m_row;
    int m_col;
} Coord;
typedef enum { DIR_UP, DIR_RIGHT, DIR_DOWN, DIR_LEFT, DIRECTION_COUNT } Direction;
typedef struct SnakeNode {
    Coord m_pos;
    struct SnakeNode *m_next;
} SnakeNode;
typedef struct {
    SnakeNode *m_head;
    size_t m_length;
    Direction m_direction;
    int m_rows;
    int m_cols;
} Snake;
typedef enum { STEP_OK, STEP_WALL, STEP_SELF, STEP_NO_MEMORY } StepResult;

/* Nonempty ring: m_head->m_next is the tail. Next links run from tail to head.
 * The outer row/column of the board is a wall. Init starts facing right.
 * Pass a zero-initialized Snake to snakeInit; it stays empty on failure. */
bool snakeInit(Snake *snake, int rows, int cols, size_t length);
/* Opposite direction is ignored when length > 1. Does not change the snake. */
Coord snakeNextPosition(const Snake *snake, Direction requested);
bool snakeContains(const Snake *snake, Coord pos);
/* Failure leaves the snake unchanged. Normal move reuses the old tail;
 * growth inserts one new head. The caller decides whether food is eaten. */
StepResult snakeStep(Snake *snake, Direction requested, bool grow);
/* Release all nodes and reset to an empty snake. Safe on an empty snake. */
void snakeDestroy(Snake *snake);
#endif

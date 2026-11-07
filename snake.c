#include "snake.h"
#include <stdlib.h>

/* Row and column change for one step in each direction, indexed by Direction. */
static const Coord DIRECTION_DELTA[DIRECTION_COUNT] = {
    [DIR_UP] = {-1, 0},
    [DIR_RIGHT] = {0, 1},
    [DIR_DOWN] = {1, 0},
    [DIR_LEFT] = {0, -1},
};

/* Free every node of a ring that holds exactly `count` nodes, starting anywhere. */
static void freeRing(SnakeNode *start, size_t count) {
    SnakeNode *node = start;
    for (size_t i = 0; i < count; ++i) {
        SnakeNode *next = node->m_next;
        free(node);
        node = next;
    }
}

/* Decide which direction a step really uses. Invalid values and a direct
 * reverse of a snake longer than one node keep the current direction. */
static Direction effectiveDirection(const Snake *snake, Direction requested) {
    bool valid = (int)requested >= 0 && (int)requested < DIRECTION_COUNT;
    if (!valid) {
        return snake->m_direction;
    }
    int opposite = ((int)snake->m_direction + DIRECTION_COUNT / 2) % DIRECTION_COUNT;
    bool reverse = (int)requested == opposite;
    if (reverse && snake->m_length > 1) {
        return snake->m_direction;
    }
    return requested;
}

static Coord stepFrom(Coord pos, Direction direction) {
    Coord delta = DIRECTION_DELTA[direction];
    return (Coord){pos.m_row + delta.m_row, pos.m_col + delta.m_col};
}

/* The outer BOARD_WALL_THICKNESS rows and columns are wall cells. */
static bool isWall(const Snake *snake, Coord pos) {
    return pos.m_row < BOARD_WALL_THICKNESS || pos.m_row >= snake->m_rows - BOARD_WALL_THICKNESS ||
           pos.m_col < BOARD_WALL_THICKNESS || pos.m_col >= snake->m_cols - BOARD_WALL_THICKNESS;
}

static bool sameCell(Coord left, Coord right) {
    return left.m_row == right.m_row && left.m_col == right.m_col;
}

/* Like snakeContains, but skips the tail, which leaves its cell on a normal move. */
static bool bodyWithoutTailContains(const Snake *snake, Coord pos) {
    const SnakeNode *tail = snake->m_head->m_next;
    const SnakeNode *node = tail->m_next;
    for (size_t i = 1; i < snake->m_length; ++i, node = node->m_next) {
        if (sameCell(node->m_pos, pos)) {
            return true;
        }
    }
    return false;
}

bool snakeInit(Snake *snake, int rows, int cols, size_t length) {
    *snake = (Snake){0};
    if (rows < MIN_BOARD_SIZE || cols < MIN_BOARD_SIZE || length < 1 ||
        length > (size_t)(cols - 2 * BOARD_WALL_THICKNESS)) {
        return false;
    }
    int headRow = rows / 2;
    int headCol = (size_t)(cols / 2) > length ? cols / 2 : (int)length;
    /* Build from tail to head: each new node is linked after the previous one,
     * so the last node created is the head and its m_next closes the ring. */
    SnakeNode *tail = NULL;
    SnakeNode *last = NULL;
    for (size_t i = 0; i < length; ++i) {
        SnakeNode *node = malloc(sizeof *node);
        if (!node) {
            if (tail) {
                last->m_next = tail;
                freeRing(tail, i);
            }
            return false;
        }
        node->m_pos = (Coord){headRow, headCol - (int)(length - 1 - i)};
        node->m_next = NULL;
        if (!tail) {
            tail = node;
        } else {
            last->m_next = node;
        }
        last = node;
    }
    last->m_next = tail;
    snake->m_head = last;
    snake->m_length = length;
    snake->m_direction = DIR_RIGHT;
    snake->m_rows = rows;
    snake->m_cols = cols;
    return true;
}

Coord snakeNextPosition(const Snake *snake, Direction requested) {
    return stepFrom(snake->m_head->m_pos, effectiveDirection(snake, requested));
}

bool snakeContains(const Snake *snake, Coord pos) {
    if (!snake->m_head || snake->m_length == 0) {
        return false;
    }
    /* Visit exactly m_length nodes; the ring never reaches NULL. */
    const SnakeNode *node = snake->m_head;
    for (size_t i = 0; i < snake->m_length; ++i, node = node->m_next) {
        if (sameCell(node->m_pos, pos)) {
            return true;
        }
    }
    return false;
}

StepResult snakeStep(Snake *snake, Direction requested, bool grow) {
    /* All checks happen before any field or link changes. */
    Direction direction = effectiveDirection(snake, requested);
    Coord next = stepFrom(snake->m_head->m_pos, direction);
    if (isWall(snake, next)) {
        return STEP_WALL;
    }
    bool hitsBody = grow ? snakeContains(snake, next) : bodyWithoutTailContains(snake, next);
    if (hitsBody) {
        return STEP_SELF;
    }
    if (grow) {
        SnakeNode *node = malloc(sizeof *node);
        if (!node) {
            return STEP_NO_MEMORY;
        }
        /* Insert between old head and tail: old head -> new head -> tail. */
        node->m_pos = next;
        node->m_next = snake->m_head->m_next;
        snake->m_head->m_next = node;
        snake->m_head = node;
        ++snake->m_length;
    } else {
        /* The old tail becomes the new head; its m_next is already the new tail. */
        SnakeNode *oldTail = snake->m_head->m_next;
        oldTail->m_pos = next;
        snake->m_head = oldTail;
    }
    snake->m_direction = direction;
    return STEP_OK;
}

void snakeDestroy(Snake *snake) {
    if (snake->m_head) {
        freeRing(snake->m_head, snake->m_length);
    }
    *snake = (Snake){0};
}

unsigned int nextRandom(unsigned int seed) {
    static unsigned int state = RANDOM_INITIAL_SEED;
    if (seed > 0) {
        state = seed % RANDOM_MODULUS;
    }
    state = (RANDOM_MULTIPLIER * state + RANDOM_INCREMENT) % RANDOM_MODULUS;
    return state;
}

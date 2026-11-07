#include "flappy.h"

/* The upper pipe runs from the ceiling down to the top edge of the gap. */
static Box upperPipeBox(const Pipe *pipe) {
    return (Box){pipe->m_x, 0.0f, PIPE_WIDTH, pipe->m_gap_y};
}

/* The lower pipe runs from the bottom edge of the gap down to the floor. */
static Box lowerPipeBox(const Pipe *pipe) {
    float top = pipe->m_gap_y + PIPE_GAP_HEIGHT;
    return (Box){pipe->m_x, top, PIPE_WIDTH, FLOOR_Y - top};
}

void advancePipes(Game *game, float dt) {
    /* Move every pipe first, so recyclePipe sees the updated rightmost pipe. */
    for (int i = 0; i < PIPE_COUNT; ++i) {
        game->m_pipes[i].m_x -= PIPE_SPEED * dt;
    }
    for (int i = 0; i < PIPE_COUNT; ++i) {
        bool offScreen = game->m_pipes[i].m_x + PIPE_WIDTH <= 0.0f;
        if (offScreen) {
            recyclePipe(game, i);
        }
    }
}

bool birdHitsObstacle(const Game *game) {
    Box bird = gameBirdBox(game);
    /* Touching the ceiling or floor exactly is still inside [0, FLOOR_Y]. */
    if (bird.m_y < 0.0f || bird.m_y + bird.m_height > FLOOR_Y) {
        return true;
    }
    for (int i = 0; i < PIPE_COUNT; ++i) {
        const Pipe *pipe = &game->m_pipes[i];
        if (boxesOverlap(bird, upperPipeBox(pipe)) || boxesOverlap(bird, lowerPipeBox(pipe))) {
            return true;
        }
    }
    return false;
}

int scorePassedPipes(Game *game) {
    int earned = 0;
    for (int i = 0; i < PIPE_COUNT; ++i) {
        Pipe *pipe = &game->m_pipes[i];
        /* Strictly left of BIRD_X; m_scored blocks a second point until recycling. */
        bool passed = pipe->m_x + PIPE_WIDTH < BIRD_X;
        if (passed && !pipe->m_scored) {
            pipe->m_scored = true;
            ++earned;
        }
    }
    game->m_score += earned;
    return earned;
}

unsigned int nextRandom(unsigned int seed) {
    static unsigned int state = RANDOM_INITIAL_SEED;
    if (seed > 0) {
        state = seed % RANDOM_MODULUS;
    }
    state = (RANDOM_MULTIPLIER * state + RANDOM_INCREMENT) % RANDOM_MODULUS;
    return state;
}

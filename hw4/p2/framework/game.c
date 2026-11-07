#include "game.h"
#include <math.h>
#include <stddef.h>

enum { GAP_MARGIN = 56, GAP_VARIATION = 40 };
static const float GRAVITY = 880.0f;
static const float FLAP_VELOCITY = -315.0f;
static const float MAX_FALL_SPEED = 480.0f;
static const float DEMO_FLAP_MARGIN = 12.0f;

bool boxesOverlap(Box left, Box right) {
    return left.m_x < right.m_x + right.m_width && left.m_x + left.m_width > right.m_x &&
           left.m_y < right.m_y + right.m_height && left.m_y + left.m_height > right.m_y;
}

Box gameBirdBox(const Game *game) { return (Box){BIRD_X, game->m_bird_y, BIRD_WIDTH, BIRD_HEIGHT}; }

static float nextGap(float previous) {
    int offset = (int)(nextRandom(0) % (2 * GAP_VARIATION + 1)) - GAP_VARIATION;
    return fminf(FLOOR_Y - PIPE_GAP_HEIGHT - GAP_MARGIN, fmaxf(GAP_MARGIN, previous + offset));
}

void recyclePipe(Game *game, int index) {
    if (index < 0 || index >= PIPE_COUNT) {
        return;
    }
    float rightmost = VIEW_W;
    float gap = (FLOOR_Y - PIPE_GAP_HEIGHT) / 2.0f;
    for (int i = 0; i < PIPE_COUNT; ++i) {
        if (game->m_pipes[i].m_x > rightmost) {
            rightmost = game->m_pipes[i].m_x;
            gap = game->m_pipes[i].m_gap_y;
        }
    }
    game->m_pipes[index] = (Pipe){rightmost + PIPE_SPACING, nextGap(gap), false};
}

void gameInit(Game *game) {
    *game = (Game){.m_bird_y = (FLOOR_Y - BIRD_HEIGHT) / 2.0f, .m_status = READY};
    float gap = (FLOOR_Y - PIPE_GAP_HEIGHT) / 2.0f;
    for (int i = 0; i < PIPE_COUNT; ++i) {
        game->m_pipes[i] = (Pipe){FIRST_PIPE_X + i * PIPE_SPACING, gap, false};
        gap = nextGap(gap);
    }
}

void gameRestart(Game *game) {
    int best = game->m_best;
    gameInit(game);
    game->m_best = best;
}

void gameStep(Game *game, Input input, float dt) {
    if (game->m_status == DEAD) {
        if (!input.m_flap_pressed) {
            return;
        }
        gameRestart(game);
    }
    if (game->m_status == READY) {
        if (!input.m_flap_pressed) {
            return;
        }
        game->m_status = PLAYING;
    }
    if (input.m_flap_pressed) {
        game->m_vy = FLAP_VELOCITY;
    }
    game->m_elapsed += dt;
    game->m_vy = fminf(game->m_vy + GRAVITY * dt, MAX_FALL_SPEED);
    game->m_bird_y += game->m_vy * dt;
    advancePipes(game, dt);
    if (birdHitsObstacle(game)) {
        game->m_status = DEAD;
        return;
    }
    scorePassedPipes(game);
    if (game->m_score > game->m_best) {
        game->m_best = game->m_score;
    }
}

Input gameDemoInput(const Game *game) {
    if (game->m_status == READY) {
        return (Input){.m_flap_pressed = true};
    }
    const Pipe *next = NULL;
    for (int i = 0; i < PIPE_COUNT; ++i) {
        const Pipe *pipe = &game->m_pipes[i];
        if (pipe->m_x + PIPE_WIDTH >= BIRD_X && (next == NULL || pipe->m_x < next->m_x)) {
            next = pipe;
        }
    }
    float target = next ? next->m_gap_y + (PIPE_GAP_HEIGHT - BIRD_HEIGHT) / 2.0f
                        : (FLOOR_Y - BIRD_HEIGHT) / 2.0f;
    return (Input){.m_flap_pressed = game->m_bird_y > target + DEMO_FLAP_MARGIN && game->m_vy > 0};
}

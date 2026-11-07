#include "snake.h"
#include "terminal.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const double DEFAULT_CELL_ASPECT = 2.0;
static const double MIN_CELL_ASPECT = 1.0;
static const double MAX_CELL_ASPECT = 4.0;
static const double CELL_ASPECT_STEP = 0.05;
static const char UI_CONFIG_PATH[] = ".snake-ui.conf";

enum {
    BOARD_ROWS = 12,
    BOARD_COLS = 24,
    INITIAL_LENGTH = 3,
    WIN_SCORE = 10,
    DEFAULT_TICK_MS = 180,
    MIN_TICK_MS = 40,
    MAX_TICK_MS = 2000,
    INPUT_POLL_MS = 100,
    DECIMAL_BASE = 10
};
static bool placeFood(const Snake *snake, Coord *food) {
    unsigned count = 0;
    *food = (Coord){-1, -1};
    /* Reservoir sampling ends even when the board is nearly full. */
    for (int r = 1; r < snake->m_rows - 1; ++r) {
        for (int c = 1; c < snake->m_cols - 1; ++c) {
            Coord pos = {r, c};
            if (!snakeContains(snake, pos) && nextRandom(0) % ++count == 0) {
                *food = pos;
            }
        }
    }
    return count != 0;
}

static double loadAspect(void) {
    double value = DEFAULT_CELL_ASPECT;
    FILE *config = fopen(UI_CONFIG_PATH, "r");
    if (config) {
        if (fscanf(config, "%lf", &value) != 1 ||
            !(value >= MIN_CELL_ASPECT && value <= MAX_CELL_ASPECT)) {
            value = DEFAULT_CELL_ASPECT;
        }
        fclose(config);
    }
    return value;
}

static int calibrate(double value) {
    if (!terminalOpen(value)) {
        return 1;
    }
    for (;;) {
        terminalCalibration(value);
        int key = terminalReadKey(INPUT_POLL_MS);
        if (key == 'q' || key == TERMINAL_INTERRUPTED) {
            terminalClose();
            return 0;
        }
        if (key == '+' || key == '=') {
            value += CELL_ASPECT_STEP;
            if (value > MAX_CELL_ASPECT) {
                value = MAX_CELL_ASPECT;
            }
        }
        if (key == '-') {
            value -= CELL_ASPECT_STEP;
            if (value < MIN_CELL_ASPECT) {
                value = MIN_CELL_ASPECT;
            }
        }
        if (key == '\n' || key == '\r') {
            terminalClose();
            FILE *config = fopen(UI_CONFIG_PATH, "w");
            if (!config) {
                perror(UI_CONFIG_PATH);
                return 1;
            }
            bool ok = fprintf(config, "%.4f\n", value) > 0;
            if (fclose(config) != 0) {
                ok = false;
            }
            if (!ok) {
                fputs("Could not save calibration.\n", stderr);
                return 1;
            }
            printf("Saved character aspect %.2f for this folder.\n", value);
            return 0;
        }
    }
}

static bool parseLong(const char *text, long min, long max, long *out) {
    char *end;
    errno = 0;
    long value = strtol(text, &end, DECIMAL_BASE);
    if (errno || end == text || *end || value < min || value > max) {
        return false;
    }
    *out = value;
    return true;
}

int main(int argc, char **argv) {
    int tickMs = DEFAULT_TICK_MS;
    double aspect = loadAspect();
    bool calibration = false;
    unsigned int seed = (unsigned int)(time(NULL) % RANDOM_MODULUS) + 1;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--help")) {
            puts("Usage: snake [--calibrate] [--cell-aspect 1..4] [--seed N] [--tick-ms 40..2000]\n"
                 "WASD/arrows: move; P: pause; R: restart; Q: quit.\n"
                 "If the grid looks stretched, run snake --calibrate once.");
            return 0;
        }
        if (!strcmp(argv[i], "--calibrate")) {
            calibration = true;
            continue;
        }
        if (!strcmp(argv[i], "--cell-aspect") && i + 1 < argc) {
            char *end;
            errno = 0;
            aspect = strtod(argv[++i], &end);
            if (!errno && *end == '\0' && aspect >= MIN_CELL_ASPECT && aspect <= MAX_CELL_ASPECT) {
                continue;
            }
        } else if (!strcmp(argv[i], "--seed") && i + 1 < argc) {
            long n;
            if (parseLong(argv[++i], 1, UINT_MAX, &n)) {
                seed = (unsigned int)n;
                continue;
            }
        } else if (!strcmp(argv[i], "--tick-ms") && i + 1 < argc) {
            long n;
            if (parseLong(argv[++i], MIN_TICK_MS, MAX_TICK_MS, &n)) {
                tickMs = (int)n;
                continue;
            }
        }
        fputs("Invalid option. Run with --help.\n", stderr);
        return 1;
    }
    nextRandom(seed);
    if (calibration) {
        return calibrate(aspect);
    }
    Snake snake = {0};
    if (!snakeInit(&snake, BOARD_ROWS, BOARD_COLS, INITIAL_LENGTH)) {
        fputs("Could not create the snake. Complete snakeInit in snake.c first.\n", stderr);
        return 1;
    }
    Coord food;
    if (!placeFood(&snake, &food)) {
        snakeDestroy(&snake);
        return 1;
    }
    if (!terminalOpen(aspect)) {
        snakeDestroy(&snake);
        return 1;
    }
    int score = 0;
    bool paused = false, ended = false, pending = false, wasFit = false;
    Direction requested = snake.m_direction;
    const char *message = "Eat the yellow food. Reach 10 points to win.";
    int64_t deadline = terminalNowMs() + tickMs;
    for (;;) {
        bool fits = terminalFits(&snake);
        if (!fits || !wasFit) {
            deadline = terminalNowMs() + tickMs;
        }
        wasFit = fits;
        terminalPresent(&snake, food, score, paused ? "Paused. P: continue" : message);
        int64_t remaining = deadline - terminalNowMs();
        int waitMs = (!fits || paused || ended)  ? INPUT_POLL_MS
                     : remaining < 0             ? 0
                     : remaining > INPUT_POLL_MS ? INPUT_POLL_MS
                                                 : (int)remaining;
        int key = terminalReadKey(waitMs);
        if (key == 'q' || key == TERMINAL_INTERRUPTED) {
            break;
        }
        if (key == 'r') {
            snakeDestroy(&snake);
            if (!snakeInit(&snake, BOARD_ROWS, BOARD_COLS, INITIAL_LENGTH)) {
                terminalClose();
                fputs("Could not restart the snake.\n", stderr);
                return 1;
            }
            score = 0;
            paused = ended = pending = false;
            requested = snake.m_direction;
            placeFood(&snake, &food);
            message = "Eat the yellow food. Reach 10 points to win.";
            deadline = terminalNowMs() + tickMs;
            continue;
        }
        if (key == 'p' && !ended) {
            paused = !paused;
            deadline = terminalNowMs() + tickMs;
        }
        if (!paused && !ended && fits && !pending) {
            Direction candidate = snake.m_direction;
            if (key == 'w') {
                candidate = DIR_UP;
            }
            if (key == 'd') {
                candidate = DIR_RIGHT;
            }
            if (key == 's') {
                candidate = DIR_DOWN;
            }
            if (key == 'a') {
                candidate = DIR_LEFT;
            }
            if (candidate != snake.m_direction &&
                (snake.m_length <= 1 || ((int)candidate + DIRECTION_COUNT / 2) % DIRECTION_COUNT !=
                                            (int)snake.m_direction)) {
                requested = candidate;
                pending = true;
            }
        }
        if (!fits || paused || ended || terminalNowMs() < deadline) {
            continue;
        }
        Coord next = snakeNextPosition(&snake, requested);
        bool grow = next.m_row == food.m_row && next.m_col == food.m_col;
        StepResult result = snakeStep(&snake, requested, grow);
        if (result == STEP_OK) {
            if (grow) {
                ++score;
                if (score >= WIN_SCORE || !placeFood(&snake, &food)) {
                    ended = true;
                    food = (Coord){-1, -1};
                    message = "You win! R: restart   Q: quit";
                }
            }
        } else {
            ended = true;
            message = result == STEP_NO_MEMORY ? "Out of memory. R: restart   Q: quit"
                                               : "Game over. R: restart   Q: quit";
        }
        requested = snake.m_direction;
        pending = false;
        deadline = terminalNowMs() + tickMs;
    }
    terminalClose();
    snakeDestroy(&snake);
    return 0;
}

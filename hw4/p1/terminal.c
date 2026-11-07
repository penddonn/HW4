#define _POSIX_C_SOURCE 200809L
#include "terminal.h"
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/* SBTerminal's half-block idea: foreground = top pixel, background = bottom.
 * One logical game cell is a 3x3 raster square, not a pair of text columns.
 * Sampling accounts for terminal character height / width. */
enum {
    SCALE = 3,
    MAX_W = 120,
    MAX_H = 144,
    HALF_PIXELS_PER_CELL = 2,
    RESERVED_ROWS = 4,
    GAME_FIRST_ROW = 3,
    TITLE_CAPACITY = 128,
    ESCAPE_KEY = 27,
    ESCAPE_WAIT_MS = 15,
    SIGNAL_EXIT_BASE = 128,
    MS_PER_SECOND = 1000,
    NS_PER_MS = 1000000
};
enum { RGB_RED, RGB_GREEN, RGB_BLUE, RGB_CHANNELS };
/* Palette entries are RGB data, not game rules. */
enum { C_FLOOR, C_WALL, C_BODY, C_HEAD, C_FOOD, C_GRID, C_INK };
static const unsigned char rgb[][RGB_CHANNELS] = {
    [C_FLOOR] = {15, 23, 42},  [C_WALL] = {59, 80, 121},  [C_BODY] = {34, 197, 94},
    [C_HEAD] = {34, 211, 238}, [C_FOOD] = {250, 204, 21}, [C_GRID] = {29, 41, 61},
    [C_INK] = {15, 23, 42}};
typedef struct {
    int m_width, m_height;
    unsigned char m_pixels[MAX_H][MAX_W];
} Frame;
static Frame previous;
static bool active;
static struct termios saved;
static volatile sig_atomic_t interrupted;
static double aspect;
static int lastRows, lastCols;
static const char *savedMessage;

static void stopSignal(int sig) {
    (void)sig;
    interrupted = 1;
}
/* Do not leave a student's terminal in raw input mode after a C crash.
 * tcsetattr, write and _exit are async-signal-safe POSIX operations. */
static void crashSignal(int sig) {
    static const char reset[] = "\033[0m\033[?25h\033[?1049l";
    static const char error[] = "Game crashed. Check snake.c; terminal input was restored.\n";
    (void)tcsetattr(STDIN_FILENO, TCSANOW, &saved);
    ssize_t written = write(STDOUT_FILENO, reset, sizeof reset - 1);
    (void)written;
    written = write(STDERR_FILENO, error, sizeof error - 1);
    (void)written;
    _exit(SIGNAL_EXIT_BASE + sig);
}

int64_t terminalNowMs(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (int64_t)ts.tv_sec * MS_PER_SECOND + ts.tv_nsec / NS_PER_MS;
}

void terminalClose(void) {
    if (!active) {
        return;
    }
    active = false;
    (void)tcsetattr(STDIN_FILENO, TCSANOW, &saved);
    fputs("\033[0m\033[?25h\033[?1049l", stdout);
    fflush(stdout);
}

bool terminalOpen(double cellAspect) {
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        fputs("Run the game in a macOS or Linux terminal.\n", stderr);
        return false;
    }
    const char *term = getenv("TERM");
    if (term && !strcmp(term, "dumb")) {
        fputs("Use a UTF-8 terminal with ANSI color support.\n", stderr);
        return false;
    }
    if (tcgetattr(STDIN_FILENO, &saved) != 0) {
        return false;
    }
    struct termios input = saved;
    input.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
    input.c_iflag &= (tcflag_t) ~(IXON | ICRNL);
    input.c_cc[VMIN] = 0;
    input.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &input) != 0) {
        return false;
    }
    active = true;
    interrupted = 0;
    aspect = cellAspect;
    struct sigaction action;
    memset(&action, 0, sizeof action);
    action.sa_handler = stopSignal;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGHUP, &action, NULL);
    action.sa_handler = crashSignal;
    sigaction(SIGSEGV, &action, NULL);
    sigaction(SIGABRT, &action, NULL);
    sigaction(SIGBUS, &action, NULL);
    sigaction(SIGFPE, &action, NULL);
    sigaction(SIGILL, &action, NULL);
    atexit(terminalClose);
    fputs("\033[?1049h\033[?25l\033[0m\033[2J\033[H", stdout);
    fflush(stdout);
    return true;
}

static int readByte(int timeoutMs) {
    if (interrupted) {
        return TERMINAL_INTERRUPTED;
    }
    struct pollfd fd = {STDIN_FILENO, POLLIN, 0};
    int result = poll(&fd, 1, timeoutMs);
    if (interrupted) {
        return TERMINAL_INTERRUPTED;
    }
    if (result < 0) {
        return errno == EINTR ? TERMINAL_NO_KEY : TERMINAL_INTERRUPTED;
    }
    if (!result) {
        return TERMINAL_NO_KEY;
    }
    if (fd.revents & (POLLHUP | POLLERR | POLLNVAL)) {
        return TERMINAL_INTERRUPTED;
    }
    unsigned char byte;
    return read(STDIN_FILENO, &byte, 1) == 1 ? byte : TERMINAL_NO_KEY;
}

int terminalReadKey(int timeoutMs) {
    int key = readByte(timeoutMs);
    if (key == ESCAPE_KEY) {
        int next = readByte(ESCAPE_WAIT_MS);
        if (next == '[' || next == 'O') {
            switch (readByte(ESCAPE_WAIT_MS)) {
            case 'A':
                return 'w';
            case 'B':
                return 's';
            case 'C':
                return 'd';
            case 'D':
                return 'a';
            default:
                return TERMINAL_NO_KEY;
            }
        }
        return TERMINAL_NO_KEY;
    }
    if (key >= 'A' && key <= 'Z') {
        key += 'a' - 'A';
    }
    return key;
}

static int sampleHeight(int rows) {
    return (int)(rows * SCALE * HALF_PIXELS_PER_CELL / aspect + 0.5);
}

static void windowSize(int *rows, int *cols) {
    struct winsize size = {0};
    (void)ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    *rows = size.ws_row;
    *cols = size.ws_col;
}

bool terminalFits(const Snake *snake) {
    int rows, cols;
    windowSize(&rows, &cols);
    return snake->m_cols * SCALE <= MAX_W && sampleHeight(snake->m_rows) <= MAX_H &&
           cols >= snake->m_cols * SCALE &&
           rows >= (sampleHeight(snake->m_rows) + HALF_PIXELS_PER_CELL - 1) / HALF_PIXELS_PER_CELL +
                       RESERVED_ROWS;
}

static void textLine(int row, const char *text) { printf("\033[0m\033[%d;1H\033[2K%s", row, text); }

static void presentPixels(const Frame *frame) {
    int rows, cols;
    windowSize(&rows, &cols);
    if (rows != lastRows || cols != lastCols || frame->m_width != previous.m_width ||
        frame->m_height != previous.m_height) {
        fputs("\033[2J", stdout);
        previous.m_width = 0;
        lastRows = rows;
        lastCols = cols;
    }
    int fg = -1, bg = -1;
    for (int y = 0; y < frame->m_height; y += HALF_PIXELS_PER_CELL) {
        for (int x = 0; x < frame->m_width; ++x) {
            int top = frame->m_pixels[y][x];
            int bottom = y + 1 < frame->m_height ? frame->m_pixels[y + 1][x] : C_FLOOR;
            bool same = previous.m_width && previous.m_pixels[y][x] == top &&
                        (y + 1 >= frame->m_height || previous.m_pixels[y + 1][x] == bottom);
            if (same) {
                continue;
            }
            printf("\033[%d;%dH", GAME_FIRST_ROW + y / HALF_PIXELS_PER_CELL, x + 1);
            if (fg != top || bg != bottom) {
                printf("\033[38;2;%d;%d;%d;48;2;%d;%d;%dm", rgb[top][RGB_RED], rgb[top][RGB_GREEN],
                       rgb[top][RGB_BLUE], rgb[bottom][RGB_RED], rgb[bottom][RGB_GREEN],
                       rgb[bottom][RGB_BLUE]);
                fg = top;
                bg = bottom;
            }
            fputs("▀", stdout);
        }
    }
    previous = *frame;
}

static void buildSource(Frame *source, const Snake *snake, Coord food) {
    source->m_width = snake->m_cols * SCALE;
    source->m_height = snake->m_rows * SCALE;
    for (int y = 0; y < source->m_height; ++y) {
        for (int x = 0; x < source->m_width; ++x) {
            int row = y / SCALE, col = x / SCALE;
            source->m_pixels[y][x] =
                row == 0 || col == 0 || row == snake->m_rows - 1 || col == snake->m_cols - 1
                    ? C_WALL
                    : C_FLOOR;
        }
    }
    if (food.m_row >= 0 && food.m_row < snake->m_rows && food.m_col >= 0 &&
        food.m_col < snake->m_cols) {
        for (int y = 0; y < SCALE; ++y) {
            for (int x = 0; x < SCALE; ++x) {
                source->m_pixels[food.m_row * SCALE + y][food.m_col * SCALE + x] = C_FOOD;
            }
        }
    }
    const SnakeNode *node = snake->m_head;
    for (size_t i = 0; i < snake->m_length; ++i, node = node->m_next) {
        for (int y = 0; y < SCALE; ++y) {
            for (int x = 0; x < SCALE; ++x) {
                bool eye = node == snake->m_head &&
                           ((snake->m_direction == DIR_RIGHT && x == SCALE - 1 && y != SCALE / 2) ||
                            (snake->m_direction == DIR_LEFT && x == 0 && y != SCALE / 2) ||
                            (snake->m_direction == DIR_UP && y == 0 && x != SCALE / 2) ||
                            (snake->m_direction == DIR_DOWN && y == SCALE - 1 && x != SCALE / 2));
                source->m_pixels[node->m_pos.m_row * SCALE + y][node->m_pos.m_col * SCALE + x] =
                    eye                     ? C_INK
                    : node == snake->m_head ? C_HEAD
                                            : C_BODY;
            }
        }
    }
}

void terminalPresent(const Snake *snake, Coord food, int score, const char *message) {
    if (!terminalFits(snake)) {
        if (previous.m_width || savedMessage != message) {
            fputs("\033[2J", stdout);
        }
        previous.m_width = 0;
        savedMessage = message;
        textLine(1, "Window too small. Enlarge it; the game is paused. Q: quit");
        fflush(stdout);
        return;
    }
    Frame source;
    buildSource(&source, snake, food);
    Frame frame = {.m_width = snake->m_cols * SCALE, .m_height = sampleHeight(snake->m_rows)};
    for (int y = 0; y < frame.m_height; ++y) {
        int sourceY = (int)((y + 0.5) * snake->m_rows * SCALE / frame.m_height);
        for (int x = 0; x < frame.m_width; ++x) {
            frame.m_pixels[y][x] = source.m_pixels[sourceY][x];
        }
    }
    presentPixels(&frame);
    char title[TITLE_CAPACITY];
    snprintf(title, sizeof title, "CS100 Snake | Score: %d/10 | Length: %zu", score,
             snake->m_length);
    textLine(1, title);
    textLine(2, "WASD / arrows: move   P: pause   R: restart   Q: quit");
    textLine(GAME_FIRST_ROW + (frame.m_height + HALF_PIXELS_PER_CELL - 1) / HALF_PIXELS_PER_CELL,
             message);
    savedMessage = message;
    fflush(stdout);
}

void terminalCalibration(double value) {
    aspect = value;
    enum { SQUARE_SIZE = 18, CANVAS_SIZE = SQUARE_SIZE * 2, SQUARE_ORIGIN = SQUARE_SIZE / 2 };
    Frame frame = {.m_width = CANVAS_SIZE,
                   .m_height = (int)(CANVAS_SIZE * HALF_PIXELS_PER_CELL / aspect + 0.5)};
    for (int y = 0; y < frame.m_height; ++y) {
        int sourceY = (int)((y + 0.5) * CANVAS_SIZE / frame.m_height);
        for (int x = 0; x < frame.m_width; ++x) {
            frame.m_pixels[y][x] = x >= SQUARE_ORIGIN && x < SQUARE_ORIGIN + SQUARE_SIZE &&
                                           sourceY >= SQUARE_ORIGIN &&
                                           sourceY < SQUARE_ORIGIN + SQUARE_SIZE
                                       ? C_HEAD
                                       : C_FLOOR;
        }
    }
    int rows, cols;
    windowSize(&rows, &cols);
    if (rows < (frame.m_height + HALF_PIXELS_PER_CELL - 1) / HALF_PIXELS_PER_CELL + RESERVED_ROWS ||
        cols < frame.m_width) {
        textLine(1, "Enlarge this window to see the square. Q: cancel");
        fflush(stdout);
        return;
    }
    presentPixels(&frame);
    char title[TITLE_CAPACITY];
    snprintf(title, sizeof title, "Make the cyan shape square. Character aspect: %.2f", value);
    textLine(1, title);
    textLine(2, "+/-: adjust   Enter: save   Q: cancel");
    textLine(GAME_FIRST_ROW + (frame.m_height + HALF_PIXELS_PER_CELL - 1) / HALF_PIXELS_PER_CELL,
             "This setting changes the image sampling, not your terminal font.");
    fflush(stdout);
}

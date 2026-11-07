#define _POSIX_C_SOURCE 200809L
#include "game.h"
#include "keyboard.h"
#include "output.h"
#include "render.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

enum {
    DEFAULT_COLS = 96,
    DEFAULT_ROWS = 32,
    MIN_COLS = 60,
    MIN_ROWS = 20,
    DRAW_FPS = 30,
    INPUT_BUFFER_CAPACITY = 128,
    EVENT_POLL_MS = 8
};
static const double NANOSECONDS_PER_SECOND = 1e9;
static const double MAX_FRAME_SECONDS = 0.1;

static Game *liveGame;
static void cleanupGame(void) {
    if (liveGame) {
        *liveGame = (Game){0};
    }
}

static struct termios savedTerm;
static bool terminalOpen;
static volatile sig_atomic_t stopping, resized;
static void signalStop(int sig) {
    (void)sig;
    stopping = 1;
}

static void signalResize(int sig) {
    (void)sig;
    resized = 1;
}

static double now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / NANOSECONDS_PER_SECOND;
}

static void restore(void) {
    if (!terminalOpen) {
        return;
    }
    outputCancel();
    outputControl("\033[?2026l\033[?1004l\033[0m\033[?7h\033[?25h\033[?1049l");
    tcsetattr(STDIN_FILENO, TCSANOW, &savedTerm);
    outputClose();
    terminalOpen = false;
}

static bool prepare(void) {
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        return false;
    }
    if (tcgetattr(STDIN_FILENO, &savedTerm) < 0) {
        return false;
    }
    if (!outputOpen()) {
        return false;
    }
    struct termios raw = savedTerm;
    raw.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) < 0) {
        outputClose();
        return false;
    }
    terminalOpen = true;
    atexit(restore);
    struct sigaction sa = {0};
    sa.sa_handler = signalStop;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sa.sa_handler = signalResize;
    sigaction(SIGWINCH, &sa, NULL);
    /* Enter the alternate screen; extended key reporting is optional. */
    outputControl("\033[?1049h\033[?25l\033[?7l\033[?6l\033[r\033[2J\033[H");
    outputControl("\033[?1004h");
    return true;
}

static void size(int *cols, int *rows) {
    struct winsize w = {0};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col && w.ws_row) {
        *cols = w.ws_col;
        *rows = w.ws_row;
    } else {
        *cols = DEFAULT_COLS;
        *rows = DEFAULT_ROWS;
    }
    if (*cols > MAX_VIEW_COLS) {
        *cols = MAX_VIEW_COLS;
    }
    if (*rows > MAX_VIEW_PIXELS / HALF_PIXELS_PER_CELL + RESERVED_ROWS) {
        *rows = MAX_VIEW_PIXELS / HALF_PIXELS_PER_CELL + RESERVED_ROWS;
    }
}

int main(int argc, char **argv) {
    if (argc != 1) {
        fprintf(stderr, "Usage: %s\nSpace: flap/start/retry; P: pause; R: restart; Q: quit.\n",
                argv[0]);
        return argc == 2 && strcmp(argv[1], "--help") == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    int cols = DEFAULT_COLS, rows = DEFAULT_ROWS;
    static Game game;
    liveGame = &game;
    atexit(cleanupGame);
    nextRandom((unsigned int)(time(NULL) % RANDOM_MODULUS) + 1);
    gameInit(&game);
    static Frame frame, previous;
    if (!prepare()) {
        fputs("Run in a UTF-8 terminal with ANSI truecolor support.\n", stderr);
        return EXIT_FAILURE;
    }
    size(&cols, &rows);
    Keyboard keyboard = {0};
    if (stopping) {
        restore();
        return EXIT_SUCCESS;
    }
    keyboardClear(&keyboard);
    bool paused = false, havePrevious = false;
    double last = now(), lastDraw = 0;
    float accumulator = 0;
    while (!stopping) {
        struct pollfd pfds[] = {{STDIN_FILENO, POLLIN, 0},
                                {outputPending() ? outputFd() : -1, POLLOUT, 0}};
        poll(pfds, sizeof pfds / sizeof pfds[0], EVENT_POLL_MS);
        unsigned char bytes[INPUT_BUFFER_CAPACITY];
        ssize_t n = read(STDIN_FILENO, bytes, sizeof bytes);
        if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            stopping = 1;
            break;
        }
        if (n == 0 && (pfds[0].revents & (POLLHUP | POLLERR))) {
            stopping = 1;
            break;
        }
        if (n > 0) {
            keyboardFeed(&keyboard, bytes, (size_t)n);
        }
        if (keyboard.m_frame_ack) {
            outputAck();
            keyboard.m_frame_ack = false;
        }
        if (keyboard.m_quit_pressed) {
            stopping = 1;
            break;
        }
        if (keyboard.m_focus_lost) {
            if (game.m_status == PLAYING) {
                paused = true;
            }
            keyboardClear(&keyboard);
        }
        if (keyboard.m_pause_pressed && game.m_status == PLAYING) {
            paused = !paused;
            keyboardClear(&keyboard);
        }
        if (keyboard.m_restart_pressed) {
            gameRestart(&game);
            paused = false;
            keyboardClear(&keyboard);
        }
        double t = now(), dt = t - last;
        last = t;
        if (dt > MAX_FRAME_SECONDS) {
            dt = MAX_FRAME_SECONDS;
        }
        accumulator += (float)dt;
        while (accumulator >= STEP) {
            if (!paused && cols >= MIN_COLS && rows >= MIN_ROWS) {
                Status before = game.m_status;
                gameStep(&game, keyboardInput(&keyboard), STEP);
                if (before == PLAYING && game.m_status != PLAYING) {
                    keyboardClear(&keyboard);
                }
            }
            keyboardConsume(&keyboard);
            accumulator -= STEP;
        }
        if (resized) {
            size(&cols, &rows);
            resized = 0;
            havePrevious = false;
            outputCancel();
        }
        if (!outputPump(t)) {
            stopping = 1;
            break;
        }
        if (t - lastDraw >= 1.0 / DRAW_FPS && outputReady(t)) {
            if (cols < MIN_COLS || rows < MIN_ROWS) {
                outputControl("\033[H\033[0mPlease enlarge the terminal to at least 60 columns x "
                              "20 rows.\033[K");
                havePrevious = false;
            } else {
                renderGame(&frame, &game, cols, rows, paused);
                char *data = NULL;
                size_t length = 0;
                FILE *frameStream = open_memstream(&data, &length);
                if (frameStream == NULL) {
                    stopping = 1;
                    break;
                }
                if (!havePrevious) {
                    fputs("\033[?2026h\033[2J\033[H", frameStream);
                }
                renderAnsi(frameStream, &frame, havePrevious ? &previous : NULL);
                if (length > 0) {
                    fputs("\033[6n", frameStream);
                }
                fclose(frameStream);
                if (length && outputSubmit(data, length, t)) {
                    previous = frame;
                    havePrevious = true;
                    if (!outputPump(t)) {
                        stopping = 1;
                        break;
                    }
                } else {
                    free(data);
                }
            }
            lastDraw = t;
        }
    }
    restore();
    return EXIT_SUCCESS;
}

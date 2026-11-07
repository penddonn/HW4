#ifndef HW4_TERMINAL_H
#define HW4_TERMINAL_H
/* macOS/Linux terminal interface. OS headers and implementation stay in terminal.c. */
#include "snake.h"
#include <stdbool.h>
#include <stdint.h>

enum { TERMINAL_NO_KEY = -1, TERMINAL_INTERRUPTED = -2 };

bool terminalOpen(double cellAspect);
void terminalClose(void);
/* Returns a key, TERMINAL_NO_KEY, or TERMINAL_INTERRUPTED. Arrows return w/a/s/d. */
int terminalReadKey(int timeoutMs);
bool terminalFits(const Snake *snake);
void terminalPresent(const Snake *snake, Coord food, int score, const char *message);
void terminalCalibration(double cellAspect);
int64_t terminalNowMs(void);
#endif

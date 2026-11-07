#ifndef LITTLE_LEAP_OUTPUT_H
#define LITTLE_LEAP_OUTPUT_H
#include <stdbool.h>
#include <stddef.h>
/* Owns a single pending frame. Uses an independently opened TTY descriptor. */
bool outputOpen(void);
bool outputAttach(int fd); /* takes ownership; used by transport tests */
int outputFd(void);
bool outputPending(void);
bool outputReady(double now);
bool outputSubmit(char *data, size_t length, double now);
bool outputPump(double now);
/* Enable ACK pacing only after observing a real cursor-position response. */
void outputAck(void);
void outputCancel(void);
void outputControl(const char *data);
void outputClose(void);
#endif

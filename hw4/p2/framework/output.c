#define _POSIX_C_SOURCE 200809L
#include "output.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
enum { TTY_PATH_CAPACITY = 256, CONTROL_WRITE_ATTEMPTS = 40, CONTROL_POLL_MS = 10 };
static const double FRAME_ACK_TIMEOUT_SECONDS = 1.0;

/* Opening the TTY separately avoids making stdin/stdout share O_NONBLOCK. */
static int ttyFd = -1;
static char *pending;
static size_t length, offset;
static bool awaitingAck, useFrameAcks;
static double submitted;
bool outputAttach(int fd) {
    if (ttyFd >= 0 || fd < 0) {
        return false;
    }
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        return false;
    }
    ttyFd = fd;
    return true;
}

bool outputOpen(void) {
    char path[TTY_PATH_CAPACITY];
    if (ttyname_r(STDOUT_FILENO, path, sizeof path) != 0) {
        return false;
    }
    int fd = open(path, O_WRONLY | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        return false;
    }
    if (!outputAttach(fd)) {
        close(fd);
        return false;
    }
    return true;
}

int outputFd(void) { return ttyFd; }

bool outputPending(void) { return pending != NULL; }

bool outputReady(double now) {
    if (pending) {
        return false;
    }
    if (awaitingAck && now - submitted > FRAME_ACK_TIMEOUT_SECONDS) {
        awaitingAck = false;
        useFrameAcks = false;
    }
    return !awaitingAck;
}

bool outputSubmit(char *data, size_t size, double now) {
    if (!outputReady(now) || !data || !size) {
        return false;
    }
    pending = data;
    length = size;
    offset = 0;
    submitted = now;
    return true;
}

bool outputPump(double now) {
    while (pending && offset < length) {
        ssize_t n = write(ttyFd, pending + offset, length - offset);
        if (n > 0) {
            offset += (size_t)n;
            continue;
        }
        if (n < 0 && errno == EINTR) {
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return true;
        }
        return false;
    }
    if (pending) {
        free(pending);
        pending = NULL;
        length = offset = 0;
        awaitingAck = useFrameAcks;
        submitted = now;
    }
    return true;
}

void outputAck(void) {
    useFrameAcks = true;
    awaitingAck = false;
}

void outputCancel(void) {
    free(pending);
    pending = NULL;
    length = offset = 0;
    awaitingAck = false;
}

void outputControl(const char *data) {
    if (ttyFd < 0) {
        return;
    }
    size_t n = strlen(data), done = 0;
    int tries = 0;
    while (done < n && tries++ < CONTROL_WRITE_ATTEMPTS) {
        ssize_t result = write(ttyFd, data + done, n - done);
        if (result > 0) {
            done += (size_t)result;
            continue;
        }
        if (result < 0 && errno == EINTR) {
            continue;
        }
        if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            struct pollfd p = {ttyFd, POLLOUT, 0};
            poll(&p, 1, CONTROL_POLL_MS);
            continue;
        }
        break;
    }
}

void outputClose(void) {
    outputCancel();
    if (ttyFd >= 0) {
        close(ttyFd);
    }
    ttyFd = -1;
    useFrameAcks = false;
}

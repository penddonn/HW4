# Use the system C compiler: Apple Clang on macOS, GCC/Clang on Linux.
CC ?= cc
CFLAGS ?= -std=c17 -Wall -Wextra -Wpedantic -O2
SNAKE_SRC ?= snake.c
TARGET ?= build/snake
DEBUG_FLAGS = -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all

.PHONY: all release debug run clean
all: release
release: $(TARGET)

$(TARGET): Makefile game.c terminal.c $(SNAKE_SRC) snake.h terminal.h
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I. game.c terminal.c $(SNAKE_SRC) -o $@

run: $(TARGET)
	./$(TARGET)

debug:
	$(MAKE) build/snake-debug TARGET=build/snake-debug CFLAGS="$(filter-out -O%,$(CFLAGS)) $(DEBUG_FLAGS)"

clean:
	rm -rf build

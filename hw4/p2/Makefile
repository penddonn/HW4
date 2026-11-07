CC ?= cc
CFLAGS ?= -std=c17 -Wall -Wextra -Wpedantic -O2
STUDENT_SRC ?= flappy.c
TARGET ?= build/flappy
DEBUG_FLAGS = -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all
FRAMEWORK = framework/main.c framework/game.c framework/render.c framework/keyboard.c framework/output.c
LIBS = -lm
.PHONY: all release debug run clean
all: release
release: $(TARGET)
$(TARGET): Makefile $(FRAMEWORK) $(STUDENT_SRC) flappy.h framework/game.h framework/render.h framework/keyboard.h framework/output.h
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I. -Iframework $(FRAMEWORK) $(STUDENT_SRC) -o $@ $(LIBS)
run: $(TARGET)
	./$(TARGET)
debug:
	$(MAKE) build/flappy-debug TARGET=build/flappy-debug CFLAGS="$(filter-out -O%,$(CFLAGS)) $(DEBUG_FLAGS)"
clean:
	rm -rf build

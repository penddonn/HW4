#ifndef HW4_FLAPPY_RENDER_H
#define HW4_FLAPPY_RENDER_H
#include "game.h"
#include <stdint.h>
#include <stdio.h>
enum {
    MAX_VIEW_COLS = 180,
    MAX_VIEW_PIXELS = 64,
    HALF_PIXELS_PER_CELL = 2,
    RESERVED_ROWS = 4,
    FRAME_TEXT_CAPACITY = 256,
    OVERLAY_TEXT_CAPACITY = 128,
    MAX_OVERLAYS = 8
};
typedef struct {
    int m_row;
    int m_col;
    uint32_t m_color;
    char m_text[OVERLAY_TEXT_CAPACITY];
} Overlay;
typedef struct {
    int m_width;
    int m_height;
    uint32_t m_pixels[MAX_VIEW_PIXELS][MAX_VIEW_COLS];
    char m_title[FRAME_TEXT_CAPACITY];
    char m_status[FRAME_TEXT_CAPACITY];
    char m_footer[FRAME_TEXT_CAPACITY];
    Overlay m_overlays[MAX_OVERLAYS];
    int m_overlay_count;
} Frame;
void renderGame(Frame *frame, const Game *game, int cols, int rows, bool paused);
void renderAnsi(FILE *out, const Frame *frame, const Frame *previous);
#endif

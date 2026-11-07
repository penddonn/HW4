#include "render.h"
#include <math.h>
#include <string.h>

/* RGB colors and pixel sizes below are artwork/layout data, not gameplay rules. */
enum {
    COLOR_SKY = 0xb9e6e7,
    COLOR_CLOUD = 0xf2fff8,
    COLOR_GROUND = 0xb2864e,
    COLOR_GRASS = 0x7dad55,
    COLOR_PIPE = 0x38a55b,
    COLOR_PIPE_LIGHT = 0x8ad77c,
    COLOR_PIPE_SHADOW = 0x247648,
    COLOR_BIRD = 0xffda65,
    COLOR_WING = 0xfff2b1,
    COLOR_BEAK = 0xf58c42,
    COLOR_EYE = 0x203640,
    COLOR_PANEL = 0x1e343b,
    COLOR_TEXT = 0xe7eed6,
    COLOR_ACCENT = 0xffda65,
    RGB_CHANNEL_BITS = 8,
    RGB_RED_SHIFT = 2 * RGB_CHANNEL_BITS,
    RGB_CHANNEL_MASK = 255,
    UI_FIRST_ROW = 1,
    UI_STATUS_ROW = 2,
    GAME_FIRST_ROW = 3,
    PANEL_WIDTH = 60,
    PANEL_HEIGHT = 18,
    PANEL_MARGIN = 3,
    PANEL_TEXT_COL = 3,
    CLOUD_COUNT = 3,
    CLOUD_SPACING = 256,
    CLOUD_W = 80,
    CLOUD_H = 18,
    CLOUD_Y = 48,
    PIPE_RIM_HEIGHT = 8
};
static const uint32_t NO_EMITTED_COLOR = UINT32_MAX;

static void box(Frame *frame, int x, int y, int width, int height, uint32_t color) {
    for (int row = y; row < y + height; ++row) {
        for (int col = x; col < x + width; ++col) {
            if (row >= 0 && row < frame->m_height && col >= 0 && col < frame->m_width) {
                frame->m_pixels[row][col] = color;
            }
        }
    }
}

static void worldBox(Frame *frame, float x, float y, float width, float height, uint32_t color) {
    float sx = (float)frame->m_width / VIEW_W, sy = (float)frame->m_height / VIEW_H;
    box(frame, (int)floorf(x * sx), (int)floorf(y * sy), (int)ceilf(width * sx),
        (int)ceilf(height * sy), color);
}

static void panelLine(Frame *frame, int row, int col, const char *text) {
    Overlay *overlay = &frame->m_overlays[frame->m_overlay_count++];
    overlay->m_row = row;
    overlay->m_col = col;
    overlay->m_color = COLOR_TEXT;
    snprintf(overlay->m_text, sizeof overlay->m_text, "%s", text);
}

void renderGame(Frame *frame, const Game *game, int cols, int rows, bool paused) {
    memset(frame, 0, sizeof *frame);
    int available = (rows - RESERVED_ROWS) * HALF_PIXELS_PER_CELL;
    if (available > MAX_VIEW_PIXELS) {
        available = MAX_VIEW_PIXELS;
    }
    frame->m_width = cols < MAX_VIEW_COLS ? cols : MAX_VIEW_COLS;
    if (frame->m_width > available * VIEW_W / VIEW_H) {
        frame->m_width = available * VIEW_W / VIEW_H;
    }
    frame->m_height = frame->m_width * VIEW_H / VIEW_W;
    frame->m_height -= frame->m_height % HALF_PIXELS_PER_CELL;
    box(frame, 0, 0, frame->m_width, frame->m_height, COLOR_SKY);
    for (int i = 0; i < CLOUD_COUNT; ++i) {
        worldBox(frame, i * CLOUD_SPACING + CLOUD_W, CLOUD_Y + i * CLOUD_H, CLOUD_W, CLOUD_H,
                 COLOR_CLOUD);
    }
    for (int i = 0; i < PIPE_COUNT; ++i) {
        const Pipe *pipe = &game->m_pipes[i];
        float bottom = pipe->m_gap_y + PIPE_GAP_HEIGHT;
        worldBox(frame, pipe->m_x, 0, PIPE_WIDTH, pipe->m_gap_y, COLOR_PIPE_SHADOW);
        worldBox(frame, pipe->m_x + PIPE_RIM_HEIGHT, 0, PIPE_WIDTH - PIPE_RIM_HEIGHT, pipe->m_gap_y,
                 COLOR_PIPE);
        worldBox(frame, pipe->m_x, pipe->m_gap_y - PIPE_RIM_HEIGHT, PIPE_WIDTH, PIPE_RIM_HEIGHT,
                 COLOR_PIPE_LIGHT);
        worldBox(frame, pipe->m_x, bottom, PIPE_WIDTH, FLOOR_Y - bottom, COLOR_PIPE_SHADOW);
        worldBox(frame, pipe->m_x + PIPE_RIM_HEIGHT, bottom, PIPE_WIDTH - PIPE_RIM_HEIGHT,
                 FLOOR_Y - bottom, COLOR_PIPE);
        worldBox(frame, pipe->m_x, bottom, PIPE_WIDTH, PIPE_RIM_HEIGHT, COLOR_PIPE_LIGHT);
    }
    worldBox(frame, 0, FLOOR_Y, VIEW_W, VIEW_H - FLOOR_Y, COLOR_GROUND);
    worldBox(frame, 0, FLOOR_Y, VIEW_W, PIPE_RIM_HEIGHT, COLOR_GRASS);
    int bx = BIRD_X * frame->m_width / VIEW_W;
    int by = (int)(game->m_bird_y * frame->m_height / VIEW_H);
    int bw = (int)ceilf((float)BIRD_WIDTH * frame->m_width / VIEW_W);
    int bh = (int)ceilf((float)BIRD_HEIGHT * frame->m_height / VIEW_H);
    box(frame, bx, by, bw, bh, COLOR_BIRD);
    box(frame, bx + bw - 1, by, 1, 1, COLOR_EYE);
    box(frame, bx + bw, by + bh / 2, 1, 1, COLOR_BEAK);
    box(frame, bx, by + bh - 1, bw / 2, 1, COLOR_WING);
    snprintf(frame->m_title, sizeof frame->m_title, " FLAPPY BIRD  |  SCORE %d  |  BEST %d",
             game->m_score, game->m_best);
    snprintf(frame->m_status, sizeof frame->m_status, " %s",
             game->m_status == READY  ? "Press SPACE to start."
             : game->m_status == DEAD ? "Run ended. Press SPACE to retry."
                                      : "Tap SPACE to flap. Pass between the pipes.");
    snprintf(frame->m_footer, sizeof frame->m_footer, " SPACE FLAP   P PAUSE   R RESTART   Q QUIT");
    if (paused || game->m_status != PLAYING) {
        int width = frame->m_width - 2 * PANEL_MARGIN;
        if (width > PANEL_WIDTH) {
            width = PANEL_WIDTH;
        }
        int x = (frame->m_width - width) / 2;
        int y = (frame->m_height - PANEL_HEIGHT) / 2;
        y -= y % HALF_PIXELS_PER_CELL;
        box(frame, x, y, width, PANEL_HEIGHT, COLOR_PANEL);
        int row = y / HALF_PIXELS_PER_CELL + GAME_FIRST_ROW;
        panelLine(frame, row + 1, x + PANEL_TEXT_COL, paused ? "PAUSED" : "FLAPPY BIRD");
        char score[OVERLAY_TEXT_CAPACITY];
        snprintf(score, sizeof score, "Score: %d     Best: %d", game->m_score, game->m_best);
        panelLine(frame, row + 3, x + PANEL_TEXT_COL, score);
        panelLine(frame, row + 5, x + PANEL_TEXT_COL,
                  paused ? "P: resume" : "SPACE: flap and start/retry");
        panelLine(frame, row + 7, x + PANEL_TEXT_COL, "Q: quit");
    }
}
static void textLine(FILE *out, int row, int cols, const char *s) {
    fprintf(out, "\033[%d;1H\033[0m\033[38;2;239;242;218m\033[48;2;30;52;59m", row);
    int n = (int)strlen(s);
    if (n > cols) {
        n = cols;
    }
    fwrite(s, 1, (size_t)n, out);
    for (int i = n; i < cols; ++i) {
        fputc(' ', out);
    }
}

void renderAnsi(FILE *out, const Frame *frame, const Frame *previous) {
    if (previous &&
        (frame->m_width != previous->m_width || frame->m_height != previous->m_height)) {
        previous = NULL;
    }
    bool overlayChanged =
        !previous || frame->m_overlay_count != previous->m_overlay_count ||
        memcmp(frame->m_overlays, previous->m_overlays, sizeof frame->m_overlays) != 0;
    if (previous && overlayChanged) {
        previous = NULL;
    }
    bool changed = !previous || strcmp(frame->m_title, previous->m_title) ||
                   strcmp(frame->m_status, previous->m_status) ||
                   strcmp(frame->m_footer, previous->m_footer);
    if (!changed) {
        for (int y = 0; y < frame->m_height; ++y) {
            if (memcmp(frame->m_pixels[y], previous->m_pixels[y],
                       (size_t)frame->m_width * sizeof(uint32_t))) {
                changed = true;
                break;
            }
        }
    }
    if (!changed) {
        return;
    }
    /* VS Code/xterm supports synchronized output: present complete frames. */
    fputs("\033[?2026h", out);
    if (!previous || strcmp(frame->m_title, previous->m_title)) {
        textLine(out, UI_FIRST_ROW, frame->m_width, frame->m_title);
    }
    if (!previous || strcmp(frame->m_status, previous->m_status)) {
        textLine(out, UI_STATUS_ROW, frame->m_width, frame->m_status);
    }
    uint32_t emittedFg = NO_EMITTED_COLOR, emittedBg = NO_EMITTED_COLOR;
    for (int y = 0; y < frame->m_height; y += HALF_PIXELS_PER_CELL) {
        int x = 0;
        while (x < frame->m_width) {
            uint32_t top = frame->m_pixels[y][x], bottom = frame->m_pixels[y + 1][x];
            bool dirty = !previous || top != previous->m_pixels[y][x] ||
                         bottom != previous->m_pixels[y + 1][x];
            if (!dirty) {
                ++x;
                continue;
            }
            int end = x + 1;
            while (end < frame->m_width && frame->m_pixels[y][end] == top &&
                   frame->m_pixels[y + 1][end] == bottom) {
                ++end;
            }
            fprintf(out, "\033[%d;%dH", y / HALF_PIXELS_PER_CELL + GAME_FIRST_ROW, x + 1);
            if (bottom != emittedBg) {
                fprintf(out, "\033[48;2;%u;%u;%um", bottom >> RGB_RED_SHIFT,
                        (bottom >> RGB_CHANNEL_BITS) & RGB_CHANNEL_MASK, bottom & RGB_CHANNEL_MASK);
                emittedBg = bottom;
            }
            if (top != bottom && top != emittedFg) {
                fprintf(out, "\033[38;2;%u;%u;%um", top >> RGB_RED_SHIFT,
                        (top >> RGB_CHANNEL_BITS) & RGB_CHANNEL_MASK, top & RGB_CHANNEL_MASK);
                emittedFg = top;
            }
            const char *glyph = top == bottom ? " " : "\342\226\200";
            for (int col = x; col < end; ++col) {
                fputs(glyph, out);
            }
            x = end;
        }
    }
    if (!previous || strcmp(frame->m_footer, previous->m_footer)) {
        textLine(out, frame->m_height / HALF_PIXELS_PER_CELL + GAME_FIRST_ROW, frame->m_width,
                 frame->m_footer);
    }
    if (overlayChanged) {
        for (int i = 0; i < frame->m_overlay_count; ++i) {
            const Overlay *overlayText = &frame->m_overlays[i];
            fprintf(out, "\033[%d;%dH\033[38;2;%u;%u;%um\033[48;2;30;52;59m%s", overlayText->m_row,
                    overlayText->m_col, overlayText->m_color >> RGB_RED_SHIFT,
                    (overlayText->m_color >> RGB_CHANNEL_BITS) & RGB_CHANNEL_MASK,
                    overlayText->m_color & RGB_CHANNEL_MASK, overlayText->m_text);
        }
    }
    fputs("\033[0m\033[?2026l", out);
    fflush(out);
}

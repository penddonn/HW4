#ifndef HW4_FLAPPY_KEYBOARD_H
#define HW4_FLAPPY_KEYBOARD_H
#include "game.h"
#include <stddef.h>
enum { KEY_SEQUENCE_CAPACITY = 128 };
typedef enum { INPUT_TEXT, INPUT_ESCAPE, INPUT_CSI } InputState;
typedef struct {
    bool m_flap_pressed;
    bool m_pause_pressed;
    bool m_restart_pressed;
    bool m_quit_pressed;
    bool m_focus_lost;
    bool m_frame_ack;
    char m_sequence[KEY_SEQUENCE_CAPACITY];
    size_t m_sequence_length;
    InputState m_state;
} Keyboard;
void keyboardFeed(Keyboard *keyboard, const unsigned char *bytes, size_t length);
void keyboardClear(Keyboard *keyboard);
void keyboardConsume(Keyboard *keyboard);
Input keyboardInput(const Keyboard *keyboard);
#endif

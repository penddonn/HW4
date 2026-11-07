#include "keyboard.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

enum {
    KEY_ESCAPE = 27,
    KEY_CONTROL_C = 3,
    KEY_PRESS = 1,
    CSI_FINAL_FIRST = 0x40,
    CSI_FINAL_LAST = 0x7e,
    DECIMAL_BASE = 10
};

static void keyPress(Keyboard *keyboard, unsigned code) {
    if (code >= 'A' && code <= 'Z') {
        code = (unsigned)tolower((int)code);
    }
    switch (code) {
    case ' ':
        keyboard->m_flap_pressed = true;
        break;
    case 'p':
        keyboard->m_pause_pressed = true;
        break;
    case 'r':
        keyboard->m_restart_pressed = true;
        break;
    case 'q':
    case KEY_CONTROL_C:
        keyboard->m_quit_pressed = true;
        break;
    default:
        break;
    }
}

static void sequenceEnd(Keyboard *keyboard, char final) {
    char *sequence = keyboard->m_sequence;
    if (final == 'R' && strchr(sequence, ';')) {
        keyboard->m_frame_ack = true;
    } else if (final == 'O' && !*sequence) {
        keyboardClear(keyboard);
        keyboard->m_focus_lost = true;
    } else if (final == 'u' && *sequence != '?') {
        /* Accept an already-enabled extended terminal too; no mode query is needed. */
        char *event = strchr(sequence, ':');
        if (!event || strtoul(event + 1, NULL, DECIMAL_BASE) == KEY_PRESS) {
            keyPress(keyboard, (unsigned)strtoul(sequence, NULL, DECIMAL_BASE));
        }
    }
}

void keyboardFeed(Keyboard *keyboard, const unsigned char *bytes, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        unsigned char byte = bytes[i];
        if (byte == KEY_ESCAPE) {
            keyboard->m_state = INPUT_ESCAPE;
            keyboard->m_sequence_length = 0;
        } else if (keyboard->m_state == INPUT_ESCAPE) {
            keyboard->m_state = byte == '[' ? INPUT_CSI : INPUT_TEXT;
        } else if (keyboard->m_state == INPUT_CSI) {
            if (byte >= CSI_FINAL_FIRST && byte <= CSI_FINAL_LAST) {
                keyboard->m_sequence[keyboard->m_sequence_length] = '\0';
                sequenceEnd(keyboard, (char)byte);
                keyboard->m_sequence_length = 0;
                keyboard->m_state = INPUT_TEXT;
            } else if (keyboard->m_sequence_length + 1 < KEY_SEQUENCE_CAPACITY) {
                keyboard->m_sequence[keyboard->m_sequence_length++] = (char)byte;
            } else {
                keyboard->m_sequence_length = 0;
                keyboard->m_state = INPUT_TEXT;
            }
        } else {
            keyPress(keyboard, byte);
        }
    }
}

void keyboardConsume(Keyboard *keyboard) {
    keyboard->m_flap_pressed = false;
    keyboard->m_pause_pressed = false;
    keyboard->m_restart_pressed = false;
    keyboard->m_quit_pressed = false;
    keyboard->m_focus_lost = false;
}

void keyboardClear(Keyboard *keyboard) { keyboardConsume(keyboard); }

Input keyboardInput(const Keyboard *keyboard) {
    return (Input){.m_flap_pressed = keyboard->m_flap_pressed};
}

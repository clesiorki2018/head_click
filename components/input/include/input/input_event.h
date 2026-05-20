#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    INPUT_EVENT_MOUSE_MOVE = 0,
    INPUT_EVENT_MOUSE_BUTTON,
    INPUT_EVENT_KEYBOARD_KEY,
    INPUT_EVENT_SYSTEM,
} input_event_type_t;

typedef struct {
    int16_t dx;
    int16_t dy;
} input_mouse_move_t;

typedef struct {
    uint8_t button;
    bool pressed;
} input_mouse_button_t;

typedef struct {
    uint8_t keycode;
    bool pressed;
} input_keyboard_key_t;

typedef struct {
    uint32_t code;
} input_system_event_t;

typedef struct {
    input_event_type_t type;
    union {
        input_mouse_move_t mouse_move;
        input_mouse_button_t mouse_button;
        input_keyboard_key_t keyboard_key;
        input_system_event_t system;
    } data;
} input_event_t;

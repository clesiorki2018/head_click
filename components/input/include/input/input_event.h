/*
 * head_click
 *
 * Initial ESP32-S3 USB HID receiver architecture for accessibility.
 *
 * Copyright (c) 2026 clesiorki2018
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

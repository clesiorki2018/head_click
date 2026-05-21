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

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    INPUT_PROTOCOL_OPCODE_MOUSE_MOVE = 0x01,
    INPUT_PROTOCOL_OPCODE_MOUSE_BUTTON = 0x02,
    INPUT_PROTOCOL_OPCODE_KEYBOARD_KEY = 0x03,
    INPUT_PROTOCOL_OPCODE_JOYSTICK_AXIS = 0x04,
    INPUT_PROTOCOL_OPCODE_JOYSTICK_BUTTON = 0x05,
} input_protocol_opcode_t;

enum {
    INPUT_PROTOCOL_MOUSE_MOVE_SIZE = 5,
    INPUT_PROTOCOL_MOUSE_BUTTON_SIZE = 3,
    INPUT_PROTOCOL_KEYBOARD_KEY_SIZE = 3,
    INPUT_PROTOCOL_JOYSTICK_AXIS_SIZE = 9,
    INPUT_PROTOCOL_JOYSTICK_BUTTON_SIZE = 3,
};

#ifdef __cplusplus
}
#endif

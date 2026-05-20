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

#include "esp_log.h"
#include "input/input_mapper.h"

static const char *TAG = "input_mapper";

input_event_t input_mapper_map_from_espnow(const uint8_t *payload, size_t payload_size)
{
    input_event_t event = {
        .type = INPUT_EVENT_SYSTEM,
        .data.system = {.code = 0},
    };

    if (payload == NULL || payload_size == 0) {
        ESP_LOGW(TAG, "Received empty ESP-NOW payload");
        return event;
    }

    uint8_t opcode = payload[0];
    switch (opcode) {
    case 0x01:
        if (payload_size >= 5) {
            event.type = INPUT_EVENT_MOUSE_MOVE;
            event.data.mouse_move.dx = (int16_t)(payload[1] | (payload[2] << 8));
            event.data.mouse_move.dy = (int16_t)(payload[3] | (payload[4] << 8));
        }
        break;
    case 0x02:
        if (payload_size >= 3) {
            event.type = INPUT_EVENT_MOUSE_BUTTON;
            event.data.mouse_button.button = payload[1];
            event.data.mouse_button.pressed = payload[2] != 0;
        }
        break;
    case 0x03:
        if (payload_size >= 3) {
            event.type = INPUT_EVENT_KEYBOARD_KEY;
            event.data.keyboard_key.keycode = payload[1];
            event.data.keyboard_key.pressed = payload[2] != 0;
        }
        break;
    default:
        event.type = INPUT_EVENT_SYSTEM;
        event.data.system.code = opcode;
        break;
    }

    return event;
}

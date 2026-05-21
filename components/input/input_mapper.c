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

static int16_t read_i16_le(const uint8_t *data)
{
    return (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
}

static esp_err_t require_payload_size(size_t actual_size, size_t expected_size, uint8_t opcode)
{
    if (actual_size != expected_size) {
        ESP_LOGW(TAG, "Invalid ESP-NOW payload size opcode=0x%02x size=%u expected=%u",
                 opcode, (unsigned int)actual_size, (unsigned int)expected_size);
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

esp_err_t input_mapper_map_from_espnow(const uint8_t *payload, size_t payload_size, input_event_t *event)
{
    if (event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (payload == NULL || payload_size == 0) {
        ESP_LOGW(TAG, "Received empty ESP-NOW payload");
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t opcode = payload[0];
    switch (opcode) {
    case 0x01:
        if (require_payload_size(payload_size, 5, opcode) != ESP_OK) {
            return ESP_ERR_INVALID_SIZE;
        }

        event->type = INPUT_EVENT_MOUSE_MOVE;
        event->data.mouse_move.dx = read_i16_le(&payload[1]);
        event->data.mouse_move.dy = read_i16_le(&payload[3]);
        return ESP_OK;
    case 0x02:
        if (require_payload_size(payload_size, 3, opcode) != ESP_OK) {
            return ESP_ERR_INVALID_SIZE;
        }

        event->type = INPUT_EVENT_MOUSE_BUTTON;
        event->data.mouse_button.button = payload[1];
        event->data.mouse_button.pressed = payload[2] != 0;
        return ESP_OK;
    case 0x03:
        if (require_payload_size(payload_size, 3, opcode) != ESP_OK) {
            return ESP_ERR_INVALID_SIZE;
        }

        event->type = INPUT_EVENT_KEYBOARD_KEY;
        event->data.keyboard_key.keycode = payload[1];
        event->data.keyboard_key.pressed = payload[2] != 0;
        return ESP_OK;
    default:
        ESP_LOGW(TAG, "Unknown ESP-NOW opcode=0x%02x", opcode);
        return ESP_ERR_NOT_SUPPORTED;
    }
}

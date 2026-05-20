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
#include "hid/hid_service.h"

static const char *TAG = "hid_service";

esp_err_t hid_service_init(void)
{
    ESP_LOGI(TAG, "Initializing HID service (stub)");
    return ESP_OK;
}

esp_err_t hid_mouse_move(int16_t dx, int16_t dy)
{
    ESP_LOGI(TAG, "HID mouse move dx=%d dy=%d", dx, dy);
    return ESP_OK;
}

esp_err_t hid_mouse_button(uint8_t button, bool pressed)
{
    ESP_LOGI(TAG, "HID mouse button button=%u pressed=%d", button, pressed);
    return ESP_OK;
}

esp_err_t hid_keyboard_key(uint8_t keycode, bool pressed)
{
    ESP_LOGI(TAG, "HID keyboard key keycode=%u pressed=%d", keycode, pressed);
    return ESP_OK;
}

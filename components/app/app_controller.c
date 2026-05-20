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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "core/event_bus.h"
#include "hid/hid_service.h"
#include "app/app_controller.h"

static const char *TAG = "app_controller";

static void app_controller_task(void *arg)
{
    (void)arg;
    input_event_t event;

    while (true) {
        if (event_bus_consume(&event, portMAX_DELAY) == ESP_OK) {
            switch (event.type) {
            case INPUT_EVENT_MOUSE_MOVE:
                hid_mouse_move(event.data.mouse_move.dx, event.data.mouse_move.dy);
                break;
            case INPUT_EVENT_MOUSE_BUTTON:
                hid_mouse_button(event.data.mouse_button.button, event.data.mouse_button.pressed);
                break;
            case INPUT_EVENT_KEYBOARD_KEY:
                hid_keyboard_key(event.data.keyboard_key.keycode, event.data.keyboard_key.pressed);
                break;
            case INPUT_EVENT_SYSTEM:
                ESP_LOGI(TAG, "System event code=%u", event.data.system.code);
                break;
            default:
                ESP_LOGW(TAG, "Unknown input event type=%d", event.type);
                break;
            }
        }
    }
}

esp_err_t app_controller_init(void)
{
    BaseType_t result = xTaskCreate(app_controller_task, "app_controller", 4096, NULL, 5, NULL);
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create app controller task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

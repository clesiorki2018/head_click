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

#include "core/event_bus.h"
#include "esp_log.h"

static const char *TAG = "event_bus";
static const TickType_t EVENT_BUS_DEFAULT_PUBLISH_WAIT = pdMS_TO_TICKS(100);
static QueueHandle_t s_event_queue = NULL;

esp_err_t event_bus_init(void)
{
    if (s_event_queue != NULL) {
        return ESP_OK;
    }

    s_event_queue = xQueueCreate(16, sizeof(input_event_t));
    if (s_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t event_bus_publish(const input_event_t *event)
{
    return event_bus_publish_wait(event, EVENT_BUS_DEFAULT_PUBLISH_WAIT);
}

esp_err_t event_bus_publish_wait(const input_event_t *event, TickType_t ticks_to_wait)
{
    if (s_event_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xQueueSend(s_event_queue, event, ticks_to_wait) != pdTRUE) {
        ESP_LOGW(TAG, "Event queue is full");
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

esp_err_t event_bus_consume(input_event_t *event, TickType_t ticks_to_wait)
{
    if (s_event_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xQueueReceive(s_event_queue, event, ticks_to_wait) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

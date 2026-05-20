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

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "input/input_event.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t event_bus_init(void);
esp_err_t event_bus_publish(const input_event_t *event);
esp_err_t event_bus_publish_wait(const input_event_t *event, TickType_t ticks_to_wait);
esp_err_t event_bus_consume(input_event_t *event, TickType_t ticks_to_wait);

#ifdef __cplusplus
}
#endif

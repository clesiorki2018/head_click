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

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "core/event_bus.h"
#include "hid/hid_service.h"
#include "transport_espnow/espnow_receiver.h"
#include "app/app_controller.h"

static const char *TAG = "app_main";

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "Initializing event bus");
    ESP_ERROR_CHECK(event_bus_init());

    ESP_LOGI(TAG, "Initializing HID service");
    ESP_ERROR_CHECK(hid_service_init());

    ESP_LOGI(TAG, "Initializing ESP-NOW receiver");
    ESP_ERROR_CHECK(espnow_receiver_init());

    ESP_LOGI(TAG, "Initializing application controller");
    ESP_ERROR_CHECK(app_controller_init());

    ESP_LOGI(TAG, "System ready");
}

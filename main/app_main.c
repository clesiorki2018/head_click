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
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "core/event_bus.h"
#include "hid/hid_service.h"
#include "transport_espnow/espnow_receiver.h"
#include "app/app_controller.h"
#include "status_led/status_led.h"
#include "thermal_guard/thermal_guard.h"

static const char *TAG = "app_main";

static esp_err_t configure_power_management(void)
{
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 80,
        .light_sleep_enable = false,
    };

    esp_err_t err = esp_pm_configure(&pm_config);
    if (err == ESP_OK) {
        ESP_LOGI(TAG,
                 "Power management configured: min=%dMHz max=%dMHz light_sleep=%s",
                 pm_config.min_freq_mhz,
                 pm_config.max_freq_mhz,
                 pm_config.light_sleep_enable ? "enabled" : "disabled");
    }

    return err;
}

static void enter_fatal_state(const char *step, esp_err_t err)
{
    ESP_LOGE(TAG, "%s failed: %s", step, esp_err_to_name(err));
    status_led_set_state(STATUS_LED_STATE_ERROR);

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    (void)status_led_init();
    status_led_set_state(STATUS_LED_STATE_BOOTING);

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        enter_fatal_state("NVS init", err);
    }

    ESP_LOGI(TAG, "Configuring power management");
    err = configure_power_management();
    if (err != ESP_OK) {
        enter_fatal_state("Power management config", err);
    }

    ESP_LOGI(TAG, "Starting thermal guard");
    err = thermal_guard_init();
    if (err != ESP_OK) {
        enter_fatal_state("Thermal guard init", err);
    }

    ESP_LOGI(TAG, "Initializing event bus");
    err = event_bus_init();
    if (err != ESP_OK) {
        enter_fatal_state("Event bus init", err);
    }

    ESP_LOGI(TAG, "Initializing HID service");
    err = hid_service_init();
    if (err != ESP_OK) {
        enter_fatal_state("HID service init", err);
    }

    ESP_LOGI(TAG, "Initializing ESP-NOW receiver");
    err = espnow_receiver_init();
    if (err != ESP_OK) {
        enter_fatal_state("ESP-NOW receiver init", err);
    }

    ESP_LOGI(TAG, "Initializing application controller");
    err = app_controller_init();
    if (err != ESP_OK) {
        enter_fatal_state("Application controller init", err);
    }

    status_led_set_state(STATUS_LED_STATE_READY);
    ESP_LOGI(TAG, "System ready");
}

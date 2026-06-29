/*
 * head_click
 *
 * Thermal protection for ESP32-S3 receiver boards.
 *
 * Copyright (c) 2026 clesiorki2018
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#include <stdbool.h>

#include "driver/temperature_sensor.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "status_led/status_led.h"
#include "thermal_guard/thermal_guard.h"

static const char *TAG = "thermal_guard";

#define THERMAL_GUARD_RANGE_MIN_C 20
#define THERMAL_GUARD_RANGE_MAX_C 100
#define THERMAL_GUARD_WARNING_C 75.0f
#define THERMAL_GUARD_WARNING_CLEAR_C 70.0f
#define THERMAL_GUARD_CRITICAL_C 85.0f
#define THERMAL_GUARD_POLL_MS 2000
#define THERMAL_GUARD_TASK_STACK_SIZE 3072
#define THERMAL_GUARD_TASK_PRIORITY 3

static temperature_sensor_handle_t s_temp_sensor;
static TaskHandle_t s_thermal_task;
static bool s_critical_shutdown;
static bool s_warning_active;

static void thermal_guard_enter_critical(float celsius)
{
    if (!s_critical_shutdown) {
        s_critical_shutdown = true;
        ESP_LOGE(TAG,
                 "Critical internal temperature %.1f C reached; stopping Wi-Fi/ESP-NOW",
                 (double)celsius);
    }

    esp_err_t err = esp_wifi_stop();
    if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGW(TAG, "Wi-Fi stop failed during thermal protection: %s", esp_err_to_name(err));
    }

    status_led_set_state(STATUS_LED_STATE_THERMAL_CRITICAL);
}

static void thermal_guard_task(void *arg)
{
    (void)arg;

    while (true) {
        float celsius = 0.0f;
        esp_err_t err = temperature_sensor_get_celsius(s_temp_sensor, &celsius);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Temperature read failed: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(THERMAL_GUARD_POLL_MS));
            continue;
        }

        if (celsius >= THERMAL_GUARD_CRITICAL_C) {
            thermal_guard_enter_critical(celsius);
        } else if (!s_critical_shutdown) {
            if (!s_warning_active && celsius >= THERMAL_GUARD_WARNING_C) {
                s_warning_active = true;
                ESP_LOGW(TAG, "High internal temperature: %.1f C", (double)celsius);
                status_led_set_state(STATUS_LED_STATE_THERMAL_WARNING);
            } else if (s_warning_active && celsius <= THERMAL_GUARD_WARNING_CLEAR_C) {
                s_warning_active = false;
                ESP_LOGI(TAG, "Internal temperature recovered: %.1f C", (double)celsius);
                status_led_set_state(STATUS_LED_STATE_READY);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(THERMAL_GUARD_POLL_MS));
    }
}

esp_err_t thermal_guard_init(void)
{
    if (s_thermal_task != NULL) {
        return ESP_OK;
    }

    temperature_sensor_config_t temp_sensor_config =
        TEMPERATURE_SENSOR_CONFIG_DEFAULT(THERMAL_GUARD_RANGE_MIN_C, THERMAL_GUARD_RANGE_MAX_C);

    esp_err_t err = temperature_sensor_install(&temp_sensor_config, &s_temp_sensor);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Temperature sensor install failed: %s", esp_err_to_name(err));
        return err;
    }

    err = temperature_sensor_enable(s_temp_sensor);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Temperature sensor enable failed: %s", esp_err_to_name(err));
        return err;
    }

    BaseType_t task_result = xTaskCreate(thermal_guard_task,
                                         "thermal_guard",
                                         THERMAL_GUARD_TASK_STACK_SIZE,
                                         NULL,
                                         THERMAL_GUARD_TASK_PRIORITY,
                                         &s_thermal_task);
    if (task_result != pdPASS) {
        ESP_LOGW(TAG, "Thermal guard task creation failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG,
             "Thermal guard active: warning=%.1f C clear=%.1f C critical=%.1f C",
             (double)THERMAL_GUARD_WARNING_C,
             (double)THERMAL_GUARD_WARNING_CLEAR_C,
             (double)THERMAL_GUARD_CRITICAL_C);
    return ESP_OK;
}

/*
 * head_click
 *
 * Initial ESP32-S3 USB HID receiver architecture for accessibility.
 *
 * Copyright (c) 2026 clesiorki2018
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "status_led/status_led.h"

static const char *TAG = "status_led";

#define STATUS_LED_GPIO 48
#define STATUS_LED_RMT_RESOLUTION_HZ 10000000
#define STATUS_LED_TASK_STACK_SIZE 2048
#define STATUS_LED_TASK_PRIORITY 2

static const rmt_symbol_word_t ws2812_zero = {
    .level0 = 1,
    .duration0 = 0.3 * STATUS_LED_RMT_RESOLUTION_HZ / 1000000,
    .level1 = 0,
    .duration1 = 0.9 * STATUS_LED_RMT_RESOLUTION_HZ / 1000000,
};

static const rmt_symbol_word_t ws2812_one = {
    .level0 = 1,
    .duration0 = 0.9 * STATUS_LED_RMT_RESOLUTION_HZ / 1000000,
    .level1 = 0,
    .duration1 = 0.3 * STATUS_LED_RMT_RESOLUTION_HZ / 1000000,
};

static const rmt_symbol_word_t ws2812_reset = {
    .level0 = 0,
    .duration0 = STATUS_LED_RMT_RESOLUTION_HZ / 1000000 * 50 / 2,
    .level1 = 0,
    .duration1 = STATUS_LED_RMT_RESOLUTION_HZ / 1000000 * 50 / 2,
};

static rmt_channel_handle_t s_led_channel;
static rmt_encoder_handle_t s_led_encoder;
static TaskHandle_t s_status_task;
static volatile status_led_state_t s_state = STATUS_LED_STATE_BOOTING;
static volatile uint8_t s_rx_pulses;

static size_t ws2812_encoder_callback(const void *data,
                                      size_t data_size,
                                      size_t symbols_written,
                                      size_t symbols_free,
                                      rmt_symbol_word_t *symbols,
                                      bool *done,
                                      void *arg)
{
    (void)arg;

    if (symbols_free < 8) {
        return 0;
    }

    size_t data_pos = symbols_written / 8;
    const uint8_t *data_bytes = (const uint8_t *)data;

    if (data_pos < data_size) {
        size_t symbol_pos = 0;
        for (uint8_t bitmask = 0x80; bitmask != 0; bitmask >>= 1) {
            symbols[symbol_pos++] = (data_bytes[data_pos] & bitmask) ? ws2812_one : ws2812_zero;
        }
        return symbol_pos;
    }

    symbols[0] = ws2812_reset;
    *done = true;
    return 1;
}

static void status_led_write_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    if (s_led_channel == NULL || s_led_encoder == NULL) {
        return;
    }

    uint8_t pixel[3] = {green, red, blue};
    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };

    if (rmt_transmit(s_led_channel, s_led_encoder, pixel, sizeof(pixel), &tx_config) == ESP_OK) {
        (void)rmt_tx_wait_all_done(s_led_channel, pdMS_TO_TICKS(20));
    }
}

static void status_led_task(void *arg)
{
    (void)arg;

    while (true) {
        if (s_rx_pulses > 0 && s_state != STATUS_LED_STATE_THERMAL_CRITICAL &&
            s_state != STATUS_LED_STATE_ERROR) {
            --s_rx_pulses;
            status_led_write_rgb(0, 8, 8);
            vTaskDelay(pdMS_TO_TICKS(60));
            status_led_write_rgb(0, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(60));
            continue;
        }

        switch (s_state) {
        case STATUS_LED_STATE_BOOTING:
            status_led_write_rgb(0, 0, 8);
            vTaskDelay(pdMS_TO_TICKS(120));
            status_led_write_rgb(0, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(380));
            break;
        case STATUS_LED_STATE_READY:
            status_led_write_rgb(0, 6, 0);
            vTaskDelay(pdMS_TO_TICKS(40));
            status_led_write_rgb(0, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(1960));
            break;
        case STATUS_LED_STATE_THERMAL_WARNING:
            status_led_write_rgb(10, 6, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            status_led_write_rgb(0, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(400));
            break;
        case STATUS_LED_STATE_THERMAL_CRITICAL:
            status_led_write_rgb(12, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(80));
            status_led_write_rgb(0, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(80));
            break;
        case STATUS_LED_STATE_ERROR:
        default:
            status_led_write_rgb(10, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(180));
            status_led_write_rgb(0, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(180));
            break;
        }
    }
}

esp_err_t status_led_init(void)
{
    if (s_status_task != NULL) {
        return ESP_OK;
    }

    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = STATUS_LED_GPIO,
        .mem_block_symbols = 64,
        .resolution_hz = STATUS_LED_RMT_RESOLUTION_HZ,
        .trans_queue_depth = 2,
    };

    esp_err_t err = rmt_new_tx_channel(&tx_chan_config, &s_led_channel);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Status LED disabled: RMT channel init failed: %s", esp_err_to_name(err));
        return err;
    }

    const rmt_simple_encoder_config_t encoder_config = {
        .callback = ws2812_encoder_callback,
    };

    err = rmt_new_simple_encoder(&encoder_config, &s_led_encoder);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Status LED disabled: encoder init failed: %s", esp_err_to_name(err));
        return err;
    }

    err = rmt_enable(s_led_channel);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Status LED disabled: RMT enable failed: %s", esp_err_to_name(err));
        return err;
    }

    BaseType_t task_result = xTaskCreate(status_led_task,
                                         "status_led",
                                         STATUS_LED_TASK_STACK_SIZE,
                                         NULL,
                                         STATUS_LED_TASK_PRIORITY,
                                         &s_status_task);
    if (task_result != pdPASS) {
        ESP_LOGW(TAG, "Status LED task creation failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Status LED ready on WS2812 GPIO %d", STATUS_LED_GPIO);
    return ESP_OK;
}

void status_led_set_state(status_led_state_t state)
{
    s_state = state;
}

void status_led_pulse_rx(void)
{
    if (s_rx_pulses < 3) {
        ++s_rx_pulses;
    }
}

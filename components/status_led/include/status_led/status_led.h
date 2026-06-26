/*
 * head_click
 *
 * Initial ESP32-S3 USB HID receiver architecture for accessibility.
 *
 * Copyright (c) 2026 clesiorki2018
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STATUS_LED_STATE_BOOTING = 0,
    STATUS_LED_STATE_READY,
    STATUS_LED_STATE_THERMAL_WARNING,
    STATUS_LED_STATE_THERMAL_CRITICAL,
    STATUS_LED_STATE_ERROR,
} status_led_state_t;

esp_err_t status_led_init(void);
void status_led_set_state(status_led_state_t state);
void status_led_pulse_rx(void);

#ifdef __cplusplus
}
#endif

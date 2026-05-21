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

#include <limits.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hid/hid_service.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "class/hid/hid_device.h"

static const char *TAG = "hid_service";

enum {
    HID_REPORT_ID_KEYBOARD = 1,
    HID_REPORT_ID_MOUSE = 2,
    HID_REPORT_ID_GAMEPAD = 3,
};

#define HID_MOUSE_REPORT_LIMIT 127

static const uint8_t s_hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_REPORT_ID_KEYBOARD)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(HID_REPORT_ID_MOUSE)),
    TUD_HID_REPORT_DESC_GAMEPAD(HID_REPORT_ID(HID_REPORT_ID_GAMEPAD)),
};

static const char *s_hid_string_descriptor[] = {
    (char[]){0x09, 0x04},
    "head_click",
    "head_click HID Receiver",
    "000001",
    "HID interface",
};

#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

static const uint8_t s_hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(s_hid_report_descriptor), 0x81, 16, 10),
};

static uint8_t s_mouse_buttons;
static uint32_t s_gamepad_buttons;
static hid_gamepad_report_t s_gamepad_report = {
    .hat = GAMEPAD_HAT_CENTERED,
};

static bool hid_ready(void)
{
    return tud_mounted() && tud_hid_ready();
}

static int8_t clamp_mouse_delta(int16_t value)
{
    if (value > HID_MOUSE_REPORT_LIMIT) {
        return HID_MOUSE_REPORT_LIMIT;
    }

    if (value < -HID_MOUSE_REPORT_LIMIT) {
        return -HID_MOUSE_REPORT_LIMIT;
    }

    return (int8_t)value;
}

static int8_t scale_axis_to_gamepad(int16_t value)
{
    int32_t scaled = ((int32_t)value * 127) / INT16_MAX;

    if (scaled > 127) {
        return 127;
    }

    if (scaled < -127) {
        return -127;
    }

    return (int8_t)scaled;
}

static esp_err_t send_gamepad_report(void)
{
    if (!hid_ready()) {
        ESP_LOGW(TAG, "HID joystick report dropped: USB host is not ready");
        return ESP_ERR_INVALID_STATE;
    }

    tud_hid_gamepad_report(HID_REPORT_ID_GAMEPAD,
                           s_gamepad_report.x,
                           s_gamepad_report.y,
                           s_gamepad_report.z,
                           s_gamepad_report.rz,
                           s_gamepad_report.rx,
                           s_gamepad_report.ry,
                           s_gamepad_report.hat,
                           s_gamepad_buttons);
    return ESP_OK;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return s_hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance,
                               uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer,
                               uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

void tud_hid_set_report_cb(uint8_t instance,
                           uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer,
                           uint16_t bufsize)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}

esp_err_t hid_service_init(void)
{
    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();

    tusb_cfg.descriptor.device = NULL;
    tusb_cfg.descriptor.full_speed_config = s_hid_configuration_descriptor;
    tusb_cfg.descriptor.string = s_hid_string_descriptor;
    tusb_cfg.descriptor.string_count = sizeof(s_hid_string_descriptor) / sizeof(s_hid_string_descriptor[0]);
#if (TUD_OPT_HIGH_SPEED)
    tusb_cfg.descriptor.high_speed_config = s_hid_configuration_descriptor;
#endif

    esp_err_t err = tinyusb_driver_install(&tusb_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize TinyUSB HID: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "TinyUSB HID service ready");
    return ESP_OK;
}

esp_err_t hid_mouse_move(int16_t dx, int16_t dy)
{
    while (dx != 0 || dy != 0) {
        if (!hid_ready()) {
            ESP_LOGW(TAG, "HID mouse move dropped: USB host is not ready");
            return ESP_ERR_INVALID_STATE;
        }

        int8_t report_dx = clamp_mouse_delta(dx);
        int8_t report_dy = clamp_mouse_delta(dy);

        tud_hid_mouse_report(HID_REPORT_ID_MOUSE, s_mouse_buttons, report_dx, report_dy, 0, 0);

        dx -= report_dx;
        dy -= report_dy;

        if (dx != 0 || dy != 0) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }

    return ESP_OK;
}

esp_err_t hid_mouse_button(uint8_t button, bool pressed)
{
    if (button >= 8) {
        return ESP_ERR_INVALID_ARG;
    }

    if (pressed) {
        s_mouse_buttons |= (uint8_t)(1U << button);
    } else {
        s_mouse_buttons &= (uint8_t)~(1U << button);
    }

    if (!hid_ready()) {
        ESP_LOGW(TAG, "HID mouse button dropped: USB host is not ready");
        return ESP_ERR_INVALID_STATE;
    }

    tud_hid_mouse_report(HID_REPORT_ID_MOUSE, s_mouse_buttons, 0, 0, 0, 0);
    return ESP_OK;
}

esp_err_t hid_keyboard_key(uint8_t keycode, bool pressed)
{
    if (!hid_ready()) {
        ESP_LOGW(TAG, "HID keyboard key dropped: USB host is not ready");
        return ESP_ERR_INVALID_STATE;
    }

    if (pressed) {
        uint8_t keycodes[6] = {keycode};
        tud_hid_keyboard_report(HID_REPORT_ID_KEYBOARD, 0, keycodes);
    } else {
        tud_hid_keyboard_report(HID_REPORT_ID_KEYBOARD, 0, NULL);
    }

    return ESP_OK;
}

esp_err_t hid_joystick_axis(int16_t x, int16_t y, int16_t z, int16_t rz)
{
    s_gamepad_report.x = scale_axis_to_gamepad(x);
    s_gamepad_report.y = scale_axis_to_gamepad(y);
    s_gamepad_report.z = scale_axis_to_gamepad(z);
    s_gamepad_report.rz = scale_axis_to_gamepad(rz);

    return send_gamepad_report();
}

esp_err_t hid_joystick_button(uint8_t button, bool pressed)
{
    if (button >= 32) {
        return ESP_ERR_INVALID_ARG;
    }

    if (pressed) {
        s_gamepad_buttons |= (uint32_t)(1UL << button);
    } else {
        s_gamepad_buttons &= (uint32_t)~(1UL << button);
    }

    return send_gamepad_report();
}

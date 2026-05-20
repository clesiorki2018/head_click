#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t hid_service_init(void);
esp_err_t hid_mouse_move(int16_t dx, int16_t dy);
esp_err_t hid_mouse_button(uint8_t button, bool pressed);
esp_err_t hid_keyboard_key(uint8_t keycode, bool pressed);

#ifdef __cplusplus
}
#endif

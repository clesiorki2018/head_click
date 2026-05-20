#include "esp_log.h"
#include "hid_service.h"

static const char *TAG = "hid_service";

esp_err_t hid_service_init(void)
{
    ESP_LOGI(TAG, "Initializing HID service (stub)");
    return ESP_OK;
}

esp_err_t hid_mouse_move(int16_t dx, int16_t dy)
{
    ESP_LOGI(TAG, "HID mouse move dx=%d dy=%d", dx, dy);
    return ESP_OK;
}

esp_err_t hid_mouse_button(uint8_t button, bool pressed)
{
    ESP_LOGI(TAG, "HID mouse button button=%u pressed=%d", button, pressed);
    return ESP_OK;
}

esp_err_t hid_keyboard_key(uint8_t keycode, bool pressed)
{
    ESP_LOGI(TAG, "HID keyboard key keycode=%u pressed=%d", keycode, pressed);
    return ESP_OK;
}

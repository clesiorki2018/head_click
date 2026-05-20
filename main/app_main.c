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

#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "core/event_bus.h"
#include "input/input_mapper.h"
#include "transport_espnow/espnow_receiver.h"

static const char *TAG = "espnow_receiver";

static void espnow_receive_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (data == NULL || len <= 0) {
        ESP_LOGW(TAG, "Received empty ESP-NOW packet");
        return;
    }

    input_event_t event = input_mapper_map_from_espnow(data, (size_t)len);
    if (event_bus_publish(&event) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to publish input event");
    }
}

static esp_err_t wifi_init(void)
{
    esp_err_t err = esp_netif_init();
    if (err != ESP_OK) {
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        return err;
    }

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        return err;
    }

    err = esp_wifi_start();
    return err;
}

esp_err_t espnow_receiver_init(void)
{
    esp_err_t err = wifi_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi init failed: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_now_init();
    if (err != ESP_OK && err != ESP_ERR_ESPNOW_ALREADY_INIT) {
        ESP_LOGE(TAG, "ESP-NOW init failed: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_now_register_recv_cb(espnow_receive_cb);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Register receive callback failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "ESP-NOW receiver ready");
    return ESP_OK;
}

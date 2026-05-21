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
#include "system_config/system_config.h"
#include "transport_espnow/espnow_receiver.h"

static const char *TAG = "espnow_receiver";

static bool mac_matches(const uint8_t lhs[SYSTEM_CONFIG_MAC_SIZE],
                        const uint8_t rhs[SYSTEM_CONFIG_MAC_SIZE])
{
    return memcmp(lhs, rhs, SYSTEM_CONFIG_MAC_SIZE) == 0;
}

static bool espnow_is_authorized_sender(const esp_now_recv_info_t *info)
{
    const system_config_security_t *config = system_config_get_security();

    if (config->pairing_enabled) {
        return true;
    }

    if (info == NULL || info->src_addr == NULL) {
        return false;
    }

    for (size_t index = 0; index < SYSTEM_CONFIG_MAX_PEERS; ++index) {
        const system_config_esp_now_peer_t *peer = &config->peers[index];

        if (peer->enabled && peer->has_mac && mac_matches(peer->mac, info->src_addr)) {
            return true;
        }
    }

    return false;
}

static void espnow_receive_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (data == NULL || len <= 0) {
        ESP_LOGW(TAG, "Received empty ESP-NOW packet");
        return;
    }

    if (!espnow_is_authorized_sender(info)) {
        ESP_LOGW(TAG, "Rejected ESP-NOW packet from unauthorized sender");
        return;
    }

    input_event_t event;
    if (input_mapper_map_from_espnow(data, (size_t)len, &event) != ESP_OK) {
        ESP_LOGW(TAG, "Rejected invalid ESP-NOW input payload");
        return;
    }

    if (event_bus_publish_wait(&event, 0) != ESP_OK) {
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
    if (err != ESP_OK) {
        return err;
    }

    const system_config_security_t *config = system_config_get_security();
    return esp_wifi_set_channel(config->wifi_channel, WIFI_SECOND_CHAN_NONE);
}

static esp_err_t espnow_add_configured_peers(void)
{
    const system_config_security_t *config = system_config_get_security();
    size_t added_peers = 0;

    for (size_t index = 0; index < SYSTEM_CONFIG_MAX_PEERS; ++index) {
        const system_config_esp_now_peer_t *configured_peer = &config->peers[index];

        if (!configured_peer->enabled) {
            continue;
        }

        if (!configured_peer->has_mac) {
            ESP_LOGW(TAG, "Skipping peer '%s': missing MAC address", configured_peer->name);
            continue;
        }

        esp_now_peer_info_t peer = {
            .channel = config->wifi_channel,
            .ifidx = WIFI_IF_STA,
            .encrypt = config->encryption_enabled,
        };

        memcpy(peer.peer_addr, configured_peer->mac, sizeof(peer.peer_addr));
        if (config->encryption_enabled) {
            memcpy(peer.lmk, configured_peer->lmk, sizeof(peer.lmk));
        }

        esp_err_t err = esp_now_add_peer(&peer);
        if (err == ESP_ERR_ESPNOW_EXIST) {
            ESP_LOGI(TAG, "ESP-NOW peer '%s' already registered", configured_peer->name);
            ++added_peers;
            continue;
        }

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to add ESP-NOW peer '%s': %s",
                     configured_peer->name, esp_err_to_name(err));
            return err;
        }

        ++added_peers;
    }

    if (added_peers == 0 && !config->pairing_enabled) {
        ESP_LOGW(TAG, "No authorized ESP-NOW peers configured");
    }

    return ESP_OK;
}

static esp_err_t espnow_configure_security(void)
{
    const system_config_security_t *config = system_config_get_security();

    if (!config->has_env) {
        ESP_LOGW(TAG, "No local .env configuration found; ESP-NOW peers are not configured");
    }

    if (config->encryption_enabled) {
        esp_err_t err = esp_now_set_pmk(config->pmk);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set ESP-NOW PMK: %s", esp_err_to_name(err));
            return err;
        }
    }

    return espnow_add_configured_peers();
}

esp_err_t espnow_receiver_init(void)
{
    esp_err_t err = wifi_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi init failed: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_now_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "ESP-NOW init failed: %s", esp_err_to_name(err));
        return err;
    }

    err = espnow_configure_security();
    if (err != ESP_OK) {
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

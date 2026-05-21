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
#include <string.h>
#include "psa/crypto.h"
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
#include "transport_espnow/espnow_app_protocol.h"
#include "transport_espnow/espnow_receiver.h"

static const char *TAG = "espnow_receiver";

#define ESPNOW_APP_SEQUENCE_MAX_WINDOW 64

typedef struct {
    bool initialized;
    uint32_t highest_sequence;
    uint64_t seen_mask;
} espnow_replay_state_t;

static espnow_replay_state_t s_replay_states[SYSTEM_CONFIG_MAX_PEERS];

static bool mac_matches(const uint8_t lhs[SYSTEM_CONFIG_MAC_SIZE],
                        const uint8_t rhs[SYSTEM_CONFIG_MAC_SIZE])
{
    return memcmp(lhs, rhs, SYSTEM_CONFIG_MAC_SIZE) == 0;
}

static const char *enabled_label(bool enabled)
{
    return enabled ? "enabled" : "disabled";
}

static size_t configured_peer_limit(void)
{
    const system_config_security_t *config = system_config_get_security();

    if (config->max_peers > SYSTEM_CONFIG_MAX_PEERS) {
        return SYSTEM_CONFIG_MAX_PEERS;
    }

    return config->max_peers;
}

static void format_mac(const uint8_t mac[SYSTEM_CONFIG_MAC_SIZE], char *buffer, size_t buffer_size)
{
    snprintf(buffer,
             buffer_size,
             "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0],
             mac[1],
             mac[2],
             mac[3],
             mac[4],
             mac[5]);
}

static uint32_t read_u32_le(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static bool app_auth_key_is_configured(const system_config_security_t *config)
{
    uint8_t folded = 0;

    for (size_t index = 0; index < SYSTEM_CONFIG_APP_AUTH_KEY_SIZE; ++index) {
        folded |= config->app_auth_key[index];
    }

    return folded != 0;
}

static const system_config_esp_now_peer_t *espnow_find_authorized_sender(const esp_now_recv_info_t *info,
                                                                         size_t *peer_index)
{
    const system_config_security_t *config = system_config_get_security();

    if (config->pairing_enabled) {
        if (peer_index != NULL) {
            *peer_index = SYSTEM_CONFIG_MAX_PEERS;
        }

        return NULL;
    }

    if (info == NULL || info->src_addr == NULL) {
        return NULL;
    }

    for (size_t index = 0; index < configured_peer_limit(); ++index) {
        const system_config_esp_now_peer_t *peer = &config->peers[index];

        if (peer->enabled && peer->has_mac && mac_matches(peer->mac, info->src_addr)) {
            if (peer_index != NULL) {
                *peer_index = index;
            }

            return peer;
        }
    }

    return NULL;
}

static bool espnow_sequence_is_fresh(size_t peer_index, uint32_t sequence)
{
    const system_config_security_t *config = system_config_get_security();
    espnow_replay_state_t *state = &s_replay_states[peer_index];
    uint8_t window = config->sequence_window;

    if (window == 0 || window > ESPNOW_APP_SEQUENCE_MAX_WINDOW) {
        window = ESPNOW_APP_SEQUENCE_MAX_WINDOW;
    }

    if (!state->initialized) {
        state->initialized = true;
        state->highest_sequence = sequence;
        state->seen_mask = 1ULL;
        return true;
    }

    if (sequence > state->highest_sequence) {
        uint32_t delta = sequence - state->highest_sequence;

        state->seen_mask = delta >= ESPNOW_APP_SEQUENCE_MAX_WINDOW ? 0ULL : state->seen_mask << delta;
        state->seen_mask |= 1ULL;
        state->highest_sequence = sequence;
        return true;
    }

    uint32_t age = state->highest_sequence - sequence;
    if (age >= window) {
        return false;
    }

    uint64_t bit = 1ULL << age;
    if ((state->seen_mask & bit) != 0) {
        return false;
    }

    state->seen_mask |= bit;
    return true;
}

static esp_err_t espnow_verify_app_packet(const uint8_t *data, size_t len)
{
    const system_config_security_t *config = system_config_get_security();
    uint8_t digest[32];
    size_t digest_len = 0;

    if (!app_auth_key_is_configured(config)) {
        ESP_LOGE(TAG, "Application authentication key is not configured");
        return ESP_ERR_INVALID_STATE;
    }

    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_id_t key_id = PSA_KEY_ID_NULL;

    psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);
    psa_set_key_bits(&attributes, SYSTEM_CONFIG_APP_AUTH_KEY_SIZE * 8);
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
    psa_set_key_algorithm(&attributes, PSA_ALG_HMAC(PSA_ALG_SHA_256));

    psa_status_t status = psa_import_key(&attributes,
                                         config->app_auth_key,
                                         SYSTEM_CONFIG_APP_AUTH_KEY_SIZE,
                                         &key_id);
    psa_reset_key_attributes(&attributes);
    if (status != PSA_SUCCESS) {
        return ESP_FAIL;
    }

    status = psa_mac_compute(key_id,
                             PSA_ALG_HMAC(PSA_ALG_SHA_256),
                             data,
                             len - ESPNOW_APP_PROTOCOL_TAG_SIZE,
                             digest,
                             sizeof(digest),
                             &digest_len);
    psa_destroy_key(key_id);
    if (status != PSA_SUCCESS || digest_len < ESPNOW_APP_PROTOCOL_TAG_SIZE) {
        return ESP_FAIL;
    }

    const uint8_t *received_tag = &data[len - ESPNOW_APP_PROTOCOL_TAG_SIZE];
    uint8_t diff = 0;

    for (size_t index = 0; index < ESPNOW_APP_PROTOCOL_TAG_SIZE; ++index) {
        diff |= digest[index] ^ received_tag[index];
    }

    return diff == 0 ? ESP_OK : ESP_ERR_INVALID_CRC;
}

static esp_err_t espnow_unwrap_app_payload(const uint8_t *data,
                                           size_t len,
                                           size_t peer_index,
                                           const uint8_t **payload,
                                           size_t *payload_size)
{
    const system_config_security_t *config = system_config_get_security();
    size_t min_packet_size = ESPNOW_APP_PROTOCOL_HEADER_SIZE + 1 + ESPNOW_APP_PROTOCOL_TAG_SIZE;

    if (len < min_packet_size) {
        return ESP_ERR_INVALID_SIZE;
    }

    if (data[0] != ESPNOW_APP_PROTOCOL_MAGIC || data[1] != ESPNOW_APP_PROTOCOL_VERSION) {
        return ESP_ERR_INVALID_VERSION;
    }

    esp_err_t err = espnow_verify_app_packet(data, len);
    if (err != ESP_OK) {
        return err;
    }

    uint32_t sequence = read_u32_le(&data[2]);
    if (config->replay_protection_enabled && peer_index >= SYSTEM_CONFIG_MAX_PEERS) {
        return ESP_ERR_INVALID_STATE;
    }

    if (config->replay_protection_enabled && !espnow_sequence_is_fresh(peer_index, sequence)) {
        return ESP_ERR_INVALID_STATE;
    }

    *payload = &data[ESPNOW_APP_PROTOCOL_HEADER_SIZE];
    *payload_size = len - ESPNOW_APP_PROTOCOL_HEADER_SIZE - ESPNOW_APP_PROTOCOL_TAG_SIZE;
    return ESP_OK;
}

static void espnow_receive_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    const system_config_security_t *config = system_config_get_security();
    const uint8_t *input_payload = data;
    size_t input_payload_size = (size_t)len;
    size_t peer_index = SYSTEM_CONFIG_MAX_PEERS;

    if (data == NULL || len <= 0) {
        ESP_LOGW(TAG, "Received empty ESP-NOW packet");
        return;
    }

    if (espnow_find_authorized_sender(info, &peer_index) == NULL && !config->pairing_enabled) {
        ESP_LOGW(TAG, "Rejected ESP-NOW packet from unauthorized sender");
        return;
    }

    if (data[0] == ESPNOW_APP_PROTOCOL_MAGIC || config->replay_protection_enabled) {
        esp_err_t err = espnow_unwrap_app_payload(data,
                                                  (size_t)len,
                                                  peer_index,
                                                  &input_payload,
                                                  &input_payload_size);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Rejected unauthenticated ESP-NOW packet: %s", esp_err_to_name(err));
            return;
        }
    }

    input_event_t event;
    if (input_mapper_map_from_espnow(input_payload, input_payload_size, &event) != ESP_OK) {
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

    for (size_t index = 0; index < configured_peer_limit(); ++index) {
        const system_config_esp_now_peer_t *configured_peer = &config->peers[index];

        if (!configured_peer->enabled) {
            continue;
        }

        if (!configured_peer->has_mac) {
            ESP_LOGW(TAG, "Skipping peer '%s': missing MAC address", configured_peer->name);
            continue;
        }

        char mac_text[18];
        format_mac(configured_peer->mac, mac_text, sizeof(mac_text));

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

        ESP_LOGI(TAG,
                 "Authorized ESP-NOW peer slot=%u name='%s' mac=%s encrypted=%s",
                 (unsigned int)(index + 1),
                 configured_peer->name,
                 mac_text,
                 enabled_label(config->encryption_enabled));
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
    bool app_auth_enabled = app_auth_key_is_configured(config);

    ESP_LOGI(TAG,
             "ESP-NOW security channel=%u max_peers=%u encryption=%s pairing=%s app_auth=%s replay=%s sequence_window=%u",
             config->wifi_channel,
             config->max_peers,
             enabled_label(config->encryption_enabled),
             enabled_label(config->pairing_enabled),
             enabled_label(app_auth_enabled),
             enabled_label(config->replay_protection_enabled),
             config->sequence_window);

    if (!config->has_env) {
        ESP_LOGW(TAG, "No local .env configuration found; ESP-NOW peers are not configured");
    }

    if (config->pairing_enabled) {
        ESP_LOGW(TAG, "ESP-NOW pairing mode accepts unknown senders; use only during provisioning");
    }

    if (config->replay_protection_enabled && !app_auth_enabled) {
        ESP_LOGE(TAG, "Replay protection requires APP_AUTH_KEY_HEX");
        return ESP_ERR_INVALID_STATE;
    }

    if (app_auth_enabled) {
        psa_status_t status = psa_crypto_init();
        if (status != PSA_SUCCESS) {
            ESP_LOGE(TAG, "PSA crypto init failed: %d", (int)status);
            return ESP_FAIL;
        }
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

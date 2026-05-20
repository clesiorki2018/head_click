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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYSTEM_CONFIG_ESP_NOW_KEY_SIZE 16
#define SYSTEM_CONFIG_APP_AUTH_KEY_SIZE 32
#define SYSTEM_CONFIG_MAC_SIZE 6
#define SYSTEM_CONFIG_MAX_PEERS 4

typedef struct {
    const char *name;
    bool enabled;
    bool has_mac;
    uint8_t mac[SYSTEM_CONFIG_MAC_SIZE];
    uint8_t lmk[SYSTEM_CONFIG_ESP_NOW_KEY_SIZE];
} system_config_esp_now_peer_t;

typedef struct {
    bool has_env;
    bool encryption_enabled;
    bool pairing_enabled;
    bool replay_protection_enabled;
    uint8_t wifi_channel;
    uint8_t max_peers;
    uint8_t sequence_window;
    uint8_t pmk[SYSTEM_CONFIG_ESP_NOW_KEY_SIZE];
    uint8_t app_auth_key[SYSTEM_CONFIG_APP_AUTH_KEY_SIZE];
    system_config_esp_now_peer_t peers[SYSTEM_CONFIG_MAX_PEERS];
} system_config_security_t;

const system_config_security_t *system_config_get_security(void);

#ifdef __cplusplus
}
#endif

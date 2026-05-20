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

#include "system_config/system_config.h"
#include "head_click_private_config.h"

static const system_config_security_t s_security_config = {
    .has_env = HEAD_CLICK_CONFIG_HAS_ENV != 0,
    .encryption_enabled = HEAD_CLICK_ESP_NOW_ENCRYPTION_ENABLED != 0,
    .pairing_enabled = HEAD_CLICK_ESP_NOW_PAIRING_ENABLED != 0,
    .replay_protection_enabled = HEAD_CLICK_APP_REPLAY_PROTECTION_ENABLED != 0,
    .wifi_channel = HEAD_CLICK_ESP_NOW_WIFI_CHANNEL,
    .max_peers = HEAD_CLICK_ESP_NOW_MAX_PEERS,
    .sequence_window = HEAD_CLICK_APP_SEQUENCE_WINDOW,
    .pmk = {HEAD_CLICK_ESP_NOW_PMK_BYTES},
    .app_auth_key = {HEAD_CLICK_APP_AUTH_KEY_BYTES},
    .peers = {
        {
            .name = HEAD_CLICK_PEER_1_NAME,
            .enabled = HEAD_CLICK_PEER_1_ENABLED != 0,
            .has_mac = HEAD_CLICK_PEER_1_HAS_MAC != 0,
            .mac = {HEAD_CLICK_PEER_1_MAC_BYTES},
            .lmk = {HEAD_CLICK_PEER_1_LMK_BYTES},
        },
        {
            .name = HEAD_CLICK_PEER_2_NAME,
            .enabled = HEAD_CLICK_PEER_2_ENABLED != 0,
            .has_mac = HEAD_CLICK_PEER_2_HAS_MAC != 0,
            .mac = {HEAD_CLICK_PEER_2_MAC_BYTES},
            .lmk = {HEAD_CLICK_PEER_2_LMK_BYTES},
        },
        {
            .name = HEAD_CLICK_PEER_3_NAME,
            .enabled = HEAD_CLICK_PEER_3_ENABLED != 0,
            .has_mac = HEAD_CLICK_PEER_3_HAS_MAC != 0,
            .mac = {HEAD_CLICK_PEER_3_MAC_BYTES},
            .lmk = {HEAD_CLICK_PEER_3_LMK_BYTES},
        },
        {
            .name = HEAD_CLICK_PEER_4_NAME,
            .enabled = HEAD_CLICK_PEER_4_ENABLED != 0,
            .has_mac = HEAD_CLICK_PEER_4_HAS_MAC != 0,
            .mac = {HEAD_CLICK_PEER_4_MAC_BYTES},
            .lmk = {HEAD_CLICK_PEER_4_LMK_BYTES},
        },
    },
};

const system_config_security_t *system_config_get_security(void)
{
    return &s_security_config;
}

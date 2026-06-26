idf_component_register(SRCS "espnow_receiver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES core input system_config status_led
                    PRIV_REQUIRES log esp_wifi esp_netif mbedtls)

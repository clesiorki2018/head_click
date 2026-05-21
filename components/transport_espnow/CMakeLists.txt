idf_component_register(SRCS "espnow_receiver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES core input system_config
                    PRIV_REQUIRES log esp_wifi esp_netif mbedtls)

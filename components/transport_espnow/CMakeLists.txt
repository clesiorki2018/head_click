idf_component_register(SRCS "espnow_receiver.c"
                    INCLUDE_DIRS "include"
                    REQUIRES core input
                    PRIV_REQUIRES log esp_wifi esp_now esp_netif)

idf_component_register(SRCS "app_main.c"
                    INCLUDE_DIRS "."
                    REQUIRES app core hid status_led thermal_guard transport_espnow
                    PRIV_REQUIRES esp_pm log nvs_flash)

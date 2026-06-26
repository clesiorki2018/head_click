idf_component_register(SRCS "thermal_guard.c"
                    INCLUDE_DIRS "include"
                    REQUIRES status_led
                    PRIV_REQUIRES esp_driver_tsens esp_wifi freertos log)

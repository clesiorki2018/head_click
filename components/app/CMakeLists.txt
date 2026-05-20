idf_component_register(SRCS "app_controller.c"
                    INCLUDE_DIRS "include"
                    REQUIRES core hid
                    PRIV_REQUIRES log)

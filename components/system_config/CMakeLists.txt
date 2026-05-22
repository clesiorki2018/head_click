set(GENERATED_CONFIG_DIR "${CMAKE_BINARY_DIR}/generated/head_click")
set(GENERATED_CONFIG_HEADER "${GENERATED_CONFIG_DIR}/head_click_private_config.h")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${PROJECT_DIR}/.env")

if(DEFINED PYTHON)
    set(HEAD_CLICK_PYTHON "${PYTHON}")
else()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(HEAD_CLICK_PYTHON "${Python3_EXECUTABLE}")
endif()

execute_process(
    COMMAND "${HEAD_CLICK_PYTHON}"
            "${PROJECT_DIR}/tools/generate_private_config.py"
            --env-file "${PROJECT_DIR}/.env"
            --output "${GENERATED_CONFIG_HEADER}"
    RESULT_VARIABLE HEAD_CLICK_CONFIG_RESULT
)

if(NOT HEAD_CLICK_CONFIG_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate private head_click configuration")
endif()

idf_component_register(SRCS "system_config.c"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "${GENERATED_CONFIG_DIR}")

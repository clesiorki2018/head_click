/*
 * head_click
 *
 * Thermal protection for ESP32-S3 receiver boards.
 *
 * Copyright (c) 2026 clesiorki2018
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t thermal_guard_init(void);

#ifdef __cplusplus
}
#endif

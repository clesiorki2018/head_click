#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "input/input_event.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t event_bus_init(void);
esp_err_t event_bus_publish(const input_event_t *event);
esp_err_t event_bus_consume(input_event_t *event, TickType_t ticks_to_wait);

#ifdef __cplusplus
}
#endif

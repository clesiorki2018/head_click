#pragma once

#include <stddef.h>
#include <stdint.h>
#include "input_event.h"

#ifdef __cplusplus
extern "C" {
#endif

input_event_t input_mapper_map_from_espnow(const uint8_t *payload, size_t payload_size);

#ifdef __cplusplus
}
#endif

#include "esp_log.h"
#include "input_mapper.h"

static const char *TAG = "input_mapper";

input_event_t input_mapper_map_from_espnow(const uint8_t *payload, size_t payload_size)
{
    input_event_t event = {
        .type = INPUT_EVENT_SYSTEM,
        .data.system = {.code = 0},
    };

    if (payload == NULL || payload_size == 0) {
        ESP_LOGW(TAG, "Received empty ESP-NOW payload");
        return event;
    }

    uint8_t opcode = payload[0];
    switch (opcode) {
    case 0x01:
        if (payload_size >= 5) {
            event.type = INPUT_EVENT_MOUSE_MOVE;
            event.data.mouse_move.dx = (int16_t)(payload[1] | (payload[2] << 8));
            event.data.mouse_move.dy = (int16_t)(payload[3] | (payload[4] << 8));
        }
        break;
    case 0x02:
        if (payload_size >= 3) {
            event.type = INPUT_EVENT_MOUSE_BUTTON;
            event.data.mouse_button.button = payload[1];
            event.data.mouse_button.pressed = payload[2] != 0;
        }
        break;
    case 0x03:
        if (payload_size >= 3) {
            event.type = INPUT_EVENT_KEYBOARD_KEY;
            event.data.keyboard_key.keycode = payload[1];
            event.data.keyboard_key.pressed = payload[2] != 0;
        }
        break;
    default:
        event.type = INPUT_EVENT_SYSTEM;
        event.data.system.code = opcode;
        break;
    }

    return event;
}

# Codex instructions for head_click

- Respond in Brazilian Portuguese.
- Use `idf.py` from `PATH`; do not source `ESP_IDF_PATH/export.sh` before ESP-IDF commands.
- Use `/tmp/head_click_build` as the ESP-IDF build directory.
- Prefer `idf.py -B /tmp/head_click_build build` for builds.
- Prefer `idf.py -B /tmp/head_click_build -p /dev/ttyACM0 flash` for flashing this receiver board.
- Current receiver board: ESP32-S3 N16; do not write real board MAC addresses to versioned files. Thermal note from 2026-06-26: regulator is not heating, heat is on the ESP32-S3 metal package/PCB, IR thermometer around 40 C.

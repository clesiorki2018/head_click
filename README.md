# ESP32-S3 HID Receiver for Accessibility

## Objetivo

Este projeto disponibiliza a arquitetura inicial para um receptor USB HID baseado em ESP32-S3 com USB nativo. O dispositivo deve aparecer no PC como teclado e mouse.

## Arquitetura

- `main/app_main.c` - ponto de entrada do firmware.
- `components/core` - barramento de eventos central usando fila FreeRTOS.
- `components/hid` - serviço responsável por enviar comandos HID para o host.
- `components/input` - definição de eventos e mapeamento de pacotes ESP-NOW para eventos de input.
- `components/transport_espnow` - transporte ESP-NOW responsável por receber pacotes sem fio.
- `components/app` - controlador de aplicação que consome eventos e aciona o serviço HID.

## Fluxo de dados

1. `espnow_receiver` inicializa Wi-Fi e ESP-NOW.
2. Ao receber um pacote, ele usa `input_mapper` para converter o payload em `input_event_t`.
3. O evento é publicado no `event_bus`.
4. `app_controller` consome eventos do `event_bus`.
5. Para comandos de input, `app_controller` chama `hid_service`.

## Como compilar

No diretório do projeto:

```sh
idf.py build
```

Se ainda não estiver usando um terminal com o ambiente ESP-IDF configurado, use:

```shn
. $HOME/esp/esp-idf/export.sh
idf.py build
```

## Git

Recomenda-se commits pequenos e atômicos para preservar a arquitetura inicial.

```sh
git status
git add .
git commit -m "chore: create initial ESP32-S3 HID receiver architecture"
```

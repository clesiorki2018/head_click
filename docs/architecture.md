# Arquitetura do head_click

## Visão geral

Este projeto é uma base para um receptor USB HID de acessibilidade usando ESP32-S3 com USB nativo. O objetivo inicial é estruturar o firmware em camadas claras, separando domínio, transporte e HID.

## Componentes

- `main/app_main.c`
  - Inicializa NVS e os componentes principais.
  - Constrói o fluxo de inicialização e a ordem dos serviços.

- `components/core`
  - `event_bus.c` / `event_bus.h`
  - Implementa uma fila FreeRTOS central para troca de eventos.

- `components/hid`
  - `hid_service.c` / `hid_service.h`
  - Exponha a interface HID para mouse e teclado.
  - Stub atual que registra os comandos no log.

- `components/input`
  - `input_event.h`
  - `input_mapper.c` / `input_mapper.h`
  - Domínio de eventos de entrada e conversão de payloads ESP-NOW.

- `components/transport_espnow`
  - `espnow_receiver.c` / `espnow_receiver.h`
  - Inicializa Wi-Fi e ESP-NOW.
  - Recebe pacotes e converte para eventos do domínio.

- `components/app`
  - `app_controller.c` / `app_controller.h`
  - Consome eventos e aciona o serviço HID.

## Fluxo de eventos

1. `main/app_main.c` inicializa os componentes principais.
2. `espnow_receiver` configura ESP-NOW e aguarda pacotes.
3. Ao receber um pacote, `input_mapper` converte bytes em `input_event_t`.
4. O evento é publicado no `event_bus`.
5. `app_controller` consome eventos e chama `hid_service`.

## Camadas e separação de responsabilidades

- Domínio: `components/input` define os tipos de evento e mapeamentos.
- Transporte: `components/transport_espnow` implementa o recebimento via ESP-NOW.
- Infraestrutura HID: `components/hid` implementa a interface para o host.
- Core: `components/core` provê o barramento de eventos central.
- Aplicação: `components/app` orquestra o comportamento do sistema.

## Como usar

Para compilar com build em RAM:

```sh
idf.py -B /mnt/rambuild/head_click build
```

Para atualizar a documentação, edite `docs/architecture.md` e adicione novos arquivos em `docs/`.

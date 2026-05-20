# ESP32-S3 HID Receiver for Accessibility

## Objetivo

Este projeto define a arquitetura inicial para um receptor USB HID baseado em ESP32-S3 com USB nativo. O dispositivo deve se comportar como teclado e mouse USB para um host PC.

## Arquitetura

- `main/app_main.c` - inicialização do firmware e componentes principais.
- `components/core` - barramento de eventos central com fila FreeRTOS.
- `components/hid` - serviço HID que exporta ações de teclado e mouse.
- `components/input` - domínio de eventos e mapeamento de pacotes ESP-NOW em eventos de input.
- `components/transport_espnow` - transporte ESP-NOW para recebimento de comandos sem fio.
- `components/app` - controlador que consome eventos e dispara o serviço HID.

## Fluxo de dados

1. `app_main` inicializa NVS, barramento de eventos, HID, ESP-NOW e controlador de aplicação.
2. `espnow_receiver` recebe pacotes ESP-NOW e converte o payload em `input_event_t`.
3. O evento é publicado no `event_bus`.
4. `app_controller` consome eventos do `event_bus`.
5. Para cada evento de input, chama `hid_service` e envia o comando HID ao host.

## Documentação inicial

- `docs/architecture.md` - visão geral da arquitetura e componentes.
- `README.md` - instruções de compilação e documentação do projeto.
- `LICENSE` - licença Apache 2.0 do projeto.

## Como compilar

No diretório do projeto:

```sh
idf.py -B /mnt/rambuild/head_click build
```

Se ainda não estiver usando o ambiente ESP-IDF:

```sh
. $HOME/esp/esp-idf/export.sh
idf.py -B /mnt/rambuild/head_click build
```

## Licença

Este projeto está licenciado sob a Apache License 2.0.

## Contribuindo

Veja o guia de contribuição para padrões de commit, fluxo de trabalho de branches e diretrizes de código:

- `docs/CONTRIBUTING.md`

## Git

Use commits pequenos e atômicos para preservar a arquitetura e a clareza do projeto.

```sh
git status
git add .
git commit -m "docs: add initial documentation and Apache license headers"
```

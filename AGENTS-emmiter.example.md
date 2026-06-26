# AGENTS-emmiter.example.md

Exemplo versionavel para orientar a criacao de um repositorio emissor/bridge ESP32-S3 que envia comandos de teclado, mouse e joystick para o receptor `head_click` via ESP-NOW.

Copie este arquivo para `AGENTS-emmiter.md` em um ambiente local e preencha os valores reais. Nao coloque MACs reais de dispositivos, PMK, LMK ou `APP_AUTH_KEY_HEX` neste arquivo de exemplo.

## Comando para descobrir o MAC do ESP32-S3

Use o MAC Wi-Fi STA. Antes de existir firmware no dispositivo, leia pelo `esptool`:

```sh
python -m esptool --chip esp32s3 -p /dev/ttyACM0 read_mac
```

Se a placa aparecer em outra porta, troque `/dev/ttyACM0` por `/dev/ttyUSB0` ou pela porta mostrada pelo sistema.

Quando o receptor ja estiver gravado, abra o monitor serial e copie a linha:

```sh
idf.py -B /tmp/head_click_build -p /dev/ttyACM0 monitor
```

Procure:

```text
Local Wi-Fi STA MAC: AA:BB:CC:DD:EE:FF
```

Esse valor deve entrar no `.env` do emissor como `HEAD_CLICK_RECEIVER_WIFI_STA_MAC`.

## Objetivo do repositorio emissor

Criar um firmware bridge para ESP32-S3 que:

- Le eventos locais de teclado, mouse e joystick.
- Converte esses eventos para o protocolo binario deste receptor.
- Envia os comandos ao MAC Wi-Fi STA do receptor via ESP-NOW.
- Usa o mesmo canal, PMK, LMK e chave HMAC configurados no receptor.
- Mantem contador de sequencia monotonicamente crescente para protecao anti-replay.

O emissor decide o significado dos inputs. O receptor valida pacote, autentica, rejeita replay e transforma payloads aprovados em HID USB.

## Modelo de `.env` do receptor

Exemplo sem dados sensiveis:

```env
# ESP-NOW radio configuration
ESP_NOW_WIFI_CHANNEL=6
ESP_NOW_ENCRYPTION_ENABLED=1
ESP_NOW_PAIRING_ENABLED=0
ESP_NOW_MAX_PEERS=4

# 16 bytes como 32 caracteres hex. Gere um valor real localmente.
ESP_NOW_PMK_HEX=00000000000000000000000000000000

# 32 bytes como 64 caracteres hex. Gere um valor real localmente.
APP_AUTH_KEY_HEX=0000000000000000000000000000000000000000000000000000000000000000
APP_REPLAY_PROTECTION_ENABLED=1
APP_SEQUENCE_WINDOW=32

ESP_NOW_PEER_1_NAME=combo
ESP_NOW_PEER_1_MAC=AA:BB:CC:DD:EE:01
ESP_NOW_PEER_1_LMK_HEX=11111111111111111111111111111111
ESP_NOW_PEER_1_ENABLED=1

ESP_NOW_PEER_2_NAME=bridge
ESP_NOW_PEER_2_MAC=AA:BB:CC:DD:EE:02
ESP_NOW_PEER_2_LMK_HEX=22222222222222222222222222222222
ESP_NOW_PEER_2_ENABLED=1

ESP_NOW_PEER_3_NAME=keyboard
ESP_NOW_PEER_3_MAC=
ESP_NOW_PEER_3_LMK_HEX=33333333333333333333333333333333
ESP_NOW_PEER_3_ENABLED=0

ESP_NOW_PEER_4_NAME=joystick
ESP_NOW_PEER_4_MAC=
ESP_NOW_PEER_4_LMK_HEX=44444444444444444444444444444444
ESP_NOW_PEER_4_ENABLED=0
```

## Modelo de `.env` para o emissor bridge

Crie um `.env` local no repositorio do bridge emissor:

```env
# Local environment. Do not commit.
ESP_IDF_PATH=/home/max/projetos/esp/esp-idf
ESPPORT=/dev/ttyACM0

# Identity
HEAD_CLICK_SENDER_NAME=bridge
HEAD_CLICK_SENDER_WIFI_STA_MAC=AA:BB:CC:DD:EE:02

# Receiver
HEAD_CLICK_RECEIVER_WIFI_STA_MAC=AA:BB:CC:DD:EE:FF

# ESP-NOW
ESP_NOW_WIFI_CHANNEL=6
ESP_NOW_ENCRYPTION_ENABLED=1
ESP_NOW_PMK_HEX=00000000000000000000000000000000
ESP_NOW_LMK_HEX=22222222222222222222222222222222

# Application authentication
APP_AUTH_KEY_HEX=0000000000000000000000000000000000000000000000000000000000000000
APP_REPLAY_PROTECTION_ENABLED=1

# Sequence persistence
APP_SEQUENCE_NAMESPACE=headclick
APP_SEQUENCE_KEY=seq_bridge
```

Substitua todos os valores placeholder antes de compilar.

## Inicializacao ESP-NOW no emissor

Fluxo minimo:

1. Inicializar NVS.
2. Inicializar `esp_netif`.
3. Criar event loop padrao.
4. Criar interface Wi-Fi STA.
5. Inicializar Wi-Fi.
6. Configurar `WIFI_MODE_STA`.
7. Iniciar Wi-Fi.
8. Fixar o canal `ESP_NOW_WIFI_CHANNEL`.
9. Inicializar ESP-NOW.
10. Se `ESP_NOW_ENCRYPTION_ENABLED=1`, chamar `esp_now_set_pmk()` com `ESP_NOW_PMK_HEX`.
11. Adicionar o receptor como peer usando `HEAD_CLICK_RECEIVER_WIFI_STA_MAC`, `ESP_NOW_LMK_HEX`, canal configurado, `WIFI_IF_STA` e `.encrypt = true`.
12. Para cada comando, montar envelope seguro e enviar com `esp_now_send(receiver_mac, packet, packet_len)`.

Exemplo do peer:

```c
esp_now_peer_info_t receiver = {
    .channel = HEAD_CLICK_ESP_NOW_WIFI_CHANNEL,
    .ifidx = WIFI_IF_STA,
    .encrypt = true,
};

memcpy(receiver.peer_addr, receiver_mac, sizeof(receiver.peer_addr));
memcpy(receiver.lmk, sender_lmk, sizeof(receiver.lmk));
ESP_ERROR_CHECK(esp_now_add_peer(&receiver));
```

## Envelope seguro da aplicacao

Com `APP_REPLAY_PROTECTION_ENABLED=1`, o receptor espera este pacote:

```text
magic:           1 byte   0xa5
version:         1 byte   0x01
sequence:        4 bytes  uint32 little-endian, crescente por emissor
command_payload: N bytes  comando HID descrito abaixo
tag:             16 bytes primeiros bytes do HMAC-SHA256
```

O HMAC-SHA256 usa `APP_AUTH_KEY_HEX` e cobre todos os bytes antes de `tag`:

```text
magic || version || sequence_le || command_payload
```

O receptor compara apenas os primeiros 16 bytes do digest. A sequencia deve ser persistida em NVS para nao voltar apos reset.

Valores fixos:

```c
#define HEAD_CLICK_APP_MAGIC 0xa5
#define HEAD_CLICK_APP_VERSION 0x01
#define HEAD_CLICK_APP_HEADER_SIZE 6
#define HEAD_CLICK_APP_TAG_SIZE 16
```

## Payloads de comando aceitos pelo receptor

Todos os inteiros de 16 bits sao little-endian.

### Movimento de mouse

```text
opcode: 0x01
size:   5 bytes
bytes:  opcode, dx_lo, dx_hi, dy_lo, dy_hi
```

`dx` e `dy` sao `int16_t`. O HID do receptor limita cada relatorio de mouse para a faixa efetiva de -127 a 127.

### Botao de mouse

```text
opcode: 0x02
size:   3 bytes
bytes:  opcode, button, pressed
```

`pressed = 0` solta. Qualquer valor diferente de zero pressiona.

Use mascaras HID de mouse:

```c
#define HEAD_CLICK_MOUSE_BUTTON_LEFT   0x01
#define HEAD_CLICK_MOUSE_BUTTON_RIGHT  0x02
#define HEAD_CLICK_MOUSE_BUTTON_MIDDLE 0x04
```

### Tecla de teclado

```text
opcode: 0x03
size:   3 bytes
bytes:  opcode, keycode, pressed
```

`keycode` e o usage ID HID de teclado. Exemplos comuns:

```c
#define HID_KEY_A      0x04
#define HID_KEY_B      0x05
#define HID_KEY_C      0x06
#define HID_KEY_ENTER  0x28
#define HID_KEY_ESCAPE 0x29
#define HID_KEY_SPACE  0x2c
```

O receptor mantem ate 6 teclas simultaneas no relatorio de teclado.

### Joystick

```text
opcode 0x04: eixos, 9 bytes: opcode, x_lo, x_hi, y_lo, y_hi, z_lo, z_hi, rz_lo, rz_hi
opcode 0x05: botao, 3 bytes: opcode, button, pressed
```

## Funcoes recomendadas no emissor

Crie uma camada pequena de envio com estas funcoes:

```c
esp_err_t head_click_send_mouse_move(int16_t dx, int16_t dy);
esp_err_t head_click_send_mouse_button(uint8_t button, bool pressed);
esp_err_t head_click_send_keyboard_key(uint8_t keycode, bool pressed);
esp_err_t head_click_send_keyboard_tap(uint8_t keycode, uint32_t hold_ms);
esp_err_t head_click_send_joystick_axis(int16_t x, int16_t y, int16_t z, int16_t rz);
esp_err_t head_click_send_joystick_button(uint8_t button, bool pressed);
```

Cada funcao monta `command_payload`, chama uma funcao comum para envelopar com HMAC e envia por ESP-NOW.

## Regras de seguranca e compatibilidade

- Nunca versionar `.env`, chaves, MACs privados ou arquivos gerados de provisionamento.
- O emissor deve usar exatamente o mesmo canal do receptor.
- O receptor aceita apenas MACs cadastrados quando `ESP_NOW_PAIRING_ENABLED=0`.
- PMK tem 16 bytes, codificados como 32 caracteres hex.
- LMK tem 16 bytes, codificados como 32 caracteres hex.
- `APP_AUTH_KEY_HEX` tem 32 bytes, codificados como 64 caracteres hex.
- Nao enviar pacotes sem envelope enquanto `APP_REPLAY_PROTECTION_ENABLED=1`.
- Nao reutilizar sequencias antigas. Sequencia repetida ou antiga sera descartada.
- Logs podem mostrar MAC e status, mas nao devem imprimir PMK, LMK nem `APP_AUTH_KEY_HEX`.

## Teste de integracao

1. Descobrir o MAC Wi-Fi STA do receptor.
2. Preencher `HEAD_CLICK_RECEIVER_WIFI_STA_MAC` no `.env` do emissor.
3. Descobrir o MAC Wi-Fi STA do emissor.
4. Garantir que o slot correspondente no `.env` do receptor contenha o MAC do emissor.
5. Compilar e gravar o receptor.
6. Compilar e gravar o emissor.
7. Abrir o monitor serial do receptor e confirmar:

```text
ESP-NOW security channel=6 ... encryption=enabled ... app_auth=enabled replay=enabled
esp_now_add_peer OK slot=2 name='bridge' mac=AA:BB:CC:DD:EE:02 channel=6 encrypt=enabled
ESP-NOW RX ... result=accepted reason=input_event_published
```

8. Enviar primeiro um clique simples ou uma tecla sem risco, por exemplo `Space`.
9. Confirmar que o receptor solta estados automaticamente depois de 3 segundos sem eventos.

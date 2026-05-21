# ESP32-S3 HID Receiver for Accessibility

## Objetivo

Este projeto define a arquitetura inicial para um receptor USB HID baseado em ESP32-S3 com USB nativo. O dispositivo deve se comportar como teclado e mouse USB para um host PC.

O cenário previsto tem dois lados:

- Transmissor: outro ESP32 lê sensores, botões, acelerômetro, IMU, comandos configuráveis ou qualquer outra entrada física.
- Receptor: este projeto roda no ESP32-S3 conectado por USB ao computador, recebe comandos via ESP-NOW e os entrega ao sistema operacional como eventos HID.

> Estado atual: a arquitetura de recebimento, mapeamento e despacho já existe. O serviço `hid_service` ainda é um stub que registra os comandos no log. A etapa final de USB HID real ainda precisa ser implementada para o sistema operacional enxergar mouse e teclado de verdade.

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

## Fluxo ponta a ponta

O fluxo completo, considerando o firmware final com HID USB implementado, é:

1. O ESP32 transmissor detecta uma intenção de entrada.
   - Exemplo de mouse: mover ponteiro, pressionar botão esquerdo, soltar botão.
   - Exemplo de teclado: pressionar `Enter`, soltar `Enter`, enviar uma tecla de atalho.
2. O transmissor transforma essa intenção em um pacote ESP-NOW.
   - O primeiro byte do payload é o opcode do comando.
   - Os bytes seguintes carregam os argumentos do comando.
3. O transmissor envia o pacote para o MAC do ESP32-S3 receptor.
4. O receptor recebe o pacote no callback ESP-NOW em `espnow_receiver`.
5. `input_mapper` interpreta o payload e cria um `input_event_t`.
6. `event_bus` coloca o evento em uma fila FreeRTOS.
7. `app_controller` consome a fila e chama a função HID correspondente.
8. `hid_service` gera o relatório USB HID.
9. O host USB recebe o relatório pelo barramento USB.
10. O sistema operacional interpreta o dispositivo como HID padrão.
11. Aplicativos recebem eventos normais de teclado e mouse, como se viessem de periféricos USB comuns.

Na arquitetura atual, os opcodes iniciais são:

| Opcode | Evento | Payload |
| --- | --- | --- |
| `0x01` | Movimento de mouse | `opcode`, `dx_lo`, `dx_hi`, `dy_lo`, `dy_hi` |
| `0x02` | Botão de mouse | `opcode`, `button`, `pressed` |
| `0x03` | Tecla de teclado | `opcode`, `keycode`, `pressed` |
| `0x04` | Eixos de joystick | `opcode`, `x_lo`, `x_hi`, `y_lo`, `y_hi`, `z_lo`, `z_hi`, `rz_lo`, `rz_hi` |
| `0x05` | Botão de joystick | `opcode`, `button`, `pressed` |

`dx`, `dy`, `x`, `y`, `z` e `rz` são inteiros de 16 bits em little-endian. `pressed` usa `0` para solto e qualquer valor diferente de zero para pressionado. O receptor rejeita payloads com tamanho diferente do esperado para cada opcode.

## Teclado e mouse ao mesmo tempo

Sim, o projeto pode ser evoluído para receber teclado e mouse ao mesmo tempo. O modelo de domínio já separa eventos de mouse e teclado em `input_event_t`, e o `app_controller` já despacha cada tipo para uma função específica de HID.

Existem duas formas comuns de expor isso ao sistema operacional:

- Dispositivo HID composto: um único dispositivo USB com interface de mouse e interface de teclado.
- Dispositivo HID com múltiplos report IDs: uma interface HID que diferencia relatórios de mouse e teclado por ID.

Para o usuário do computador, o resultado esperado é o mesmo: o sistema operacional enxerga um teclado e um mouse HID padrão. Os eventos chegam em sequência pela fila, mas podem representar comandos misturados, como mover o mouse, clicar, pressionar `Ctrl`, pressionar uma tecla e soltar tudo depois.

A configuração de quais teclas, cliques e movimentos enviar deve ficar no ESP32 transmissor. Este receptor deve continuar simples: validar pacote, mapear para evento de domínio e publicar no HID. Isso preserva a separação de responsabilidades:

- Transmissor decide o significado dos sensores e botões.
- Receptor converte comandos já decididos em HID USB.
- Host PC recebe apenas eventos HID padronizados.

Exemplo conceitual no transmissor:

```c
/* Botao fisico 1 vira clique esquerdo. */
send_mouse_button(MOUSE_BUTTON_LEFT, true);
send_mouse_button(MOUSE_BUTTON_LEFT, false);

/* Movimento de cabeca vira deslocamento do ponteiro. */
send_mouse_move(dx, dy);

/* Acao configurada vira tecla Enter. */
send_keyboard_key(HID_KEY_ENTER, true);
send_keyboard_key(HID_KEY_ENTER, false);
```

## ESP-NOW

ESP-NOW é um protocolo sem conexão da Espressif para comunicação direta entre dispositivos ESP usando rádio Wi-Fi 2.4 GHz. Ele não precisa de roteador, não precisa conectar em uma rede Wi-Fi tradicional e funciona por troca de pacotes entre peers.

Neste projeto, o receptor inicializa o Wi-Fi em modo STA e depois registra um callback de recebimento ESP-NOW. Quando um pacote chega, o callback recebe os bytes do payload e os entrega ao mapper.

Configuração mínima do receptor:

1. Inicializar NVS.
2. Inicializar `esp_netif`.
3. Criar o event loop padrão.
4. Criar a interface Wi-Fi STA.
5. Inicializar Wi-Fi.
6. Definir modo `WIFI_MODE_STA`.
7. Iniciar Wi-Fi.
8. Inicializar ESP-NOW.
9. Registrar o callback de recebimento.

Configuração mínima do transmissor:

1. Inicializar NVS, rede e Wi-Fi.
2. Definir o mesmo canal Wi-Fi usado pelo receptor.
3. Inicializar ESP-NOW.
4. Adicionar o receptor como peer usando o MAC dele.
5. Enviar payloads com `esp_now_send`.

Para comunicação estável, transmissor e receptor devem usar o mesmo canal. Se o receptor também estiver conectado a uma rede Wi-Fi, o canal geralmente fica preso ao canal do AP. Para um produto dedicado, é mais previsível fixar um canal e manter ambos os ESP32 nele.

## Configuração dos transmissores

Em uma instalação com dois ou mais ESP32 transmissores, a configuração é espelhada:

- O receptor conhece os transmissores autorizados.
- Cada transmissor conhece apenas o receptor para o qual deve enviar comandos.

No receptor, o arquivo `.env` local deve conter o MAC Wi-Fi STA de cada transmissor. A configuração inicial recomendada é um emissor `combo`, capaz de enviar comandos de mouse, teclado e joystick:

```env
ESP_NOW_PEER_1_NAME=combo
ESP_NOW_PEER_1_MAC=AA:BB:CC:DD:EE:01
ESP_NOW_PEER_1_LMK_HEX=...
ESP_NOW_PEER_1_ENABLED=1
```

Se o projeto crescer para emissores físicos separados, cadastre cada um como peer dedicado:

```env
ESP_NOW_PEER_2_NAME=mouse
ESP_NOW_PEER_2_MAC=AA:BB:CC:DD:EE:02
ESP_NOW_PEER_2_LMK_HEX=...
ESP_NOW_PEER_2_ENABLED=1

ESP_NOW_PEER_3_NAME=keyboard
ESP_NOW_PEER_3_MAC=AA:BB:CC:DD:EE:03
ESP_NOW_PEER_3_LMK_HEX=...
ESP_NOW_PEER_3_ENABLED=1

ESP_NOW_PEER_4_NAME=joystick
ESP_NOW_PEER_4_MAC=AA:BB:CC:DD:EE:04
ESP_NOW_PEER_4_LMK_HEX=...
ESP_NOW_PEER_4_ENABLED=1
```

Em cada transmissor, grave:

- MAC Wi-Fi STA do receptor.
- Canal ESP-NOW do receptor.
- PMK compartilhada.
- LMK específica daquele transmissor.
- Chave de autenticação da aplicação, quando a validação HMAC/replay estiver habilitada.

Cada transmissor deve usar apenas a LMK do seu papel. Por exemplo, o ESP32 `combo` usa a LMK do peer `combo`; o ESP32 de mouse usa a LMK do peer `mouse`; o ESP32 de teclado usa a LMK do peer `keyboard`; o ESP32 de joystick usa a LMK do peer `joystick`. Isso permite revogar ou trocar a chave de um transmissor sem trocar todas as outras.

O arquivo local `sender_config.env` é gerado para ajudar o provisionamento dos transmissores. Ele contém os dados que devem ser copiados para cada emissor e é ignorado pelo Git porque contém segredos.

Antes de gravar os transmissores:

1. Descubra o MAC Wi-Fi STA do receptor.
2. Preencha `HEAD_CLICK_RECEIVER_WIFI_STA_MAC` em `sender_config.env`.
3. Descubra o MAC Wi-Fi STA de cada transmissor.
4. Preencha `ESP_NOW_PEER_<n>_MAC` no `.env` do receptor.
5. Compile e grave receptor e transmissores com canal, PMK e LMKs compatíveis.

No firmware de um transmissor, o peer ESP-NOW deve apontar para o receptor:

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

## Segurança do ESP-NOW

ESP-NOW não deve ser tratado como uma interface confiável só porque não usa roteador. Qualquer comando que vira teclado ou mouse precisa de proteção, porque um pacote aceito pelo receptor pode virar ação no computador.

Recomendações para este projeto:

- Parear por MAC: aceitar apenas transmissores conhecidos.
- Usar criptografia ESP-NOW com PMK e LMK.
- Guardar chaves em NVS, nunca em arquivos versionados.
- Validar tamanho exato do payload para cada opcode.
- Adicionar contador de sequência para rejeitar replay.
- Adicionar checksum, CRC ou autenticação de mensagem no payload da aplicação.
- Ter modo de pareamento físico, acionado por botão, para cadastrar um novo transmissor.
- Ignorar pacotes desconhecidos ou truncados sem gerar eventos HID.
- Registrar falhas de validação com log técnico, sem despejar dados sensíveis.

Termos importantes:

- PMK: chave primária usada pelo ESP-NOW para proteger chaves locais.
- LMK: chave local do peer, usada para criptografar a comunicação com aquele dispositivo.
- Peer: dispositivo remoto cadastrado para envio ou recebimento.
- Canal: canal Wi-Fi 2.4 GHz usado pelos dois ESP32.

Uma estratégia segura para produção é:

1. O receptor inicia aceitando apenas peers cadastrados.
2. O usuário segura um botão físico para abrir uma janela curta de pareamento.
3. O transmissor envia um pedido de pareamento.
4. O receptor grava MAC e chave do transmissor em NVS.
5. Fora do modo de pareamento, pacotes de MAC desconhecido são descartados.

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

## Variáveis de ambiente

Use um arquivo `.env` local para valores sensíveis de ambiente que não devem ser comitados.
Copie o template e preencha os valores adequados:

```sh
cp .env.example .env
```

O arquivo `.env` é ignorado pelo Git e pode conter variáveis de configuração locais, como porta serial de flash, credenciais de Wi-Fi ou tokens de API.

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

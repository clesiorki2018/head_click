# Hardware

## Visão geral

Este documento descreve o hardware alvo para o projeto `head_click`, um receptor USB HID baseado em ESP32-S3 com USB nativo. O objetivo é fornecer documentação técnica detalhada do equipamento e das interfaces físicas necessárias.

## Placa alvo

- Modelo atual do receptor: ESP32-S3 N16
- Chip reconhecido: ESP32-S3 QFN56, revisão v0.2
- Flash detectada: 16 MB
- PSRAM detectada: 8 MB embedded
- MAC Wi-Fi STA: manter apenas em `.env` local ou anotações privadas, nunca em arquivo versionado
- USB: USB nativo integrado ao chip ESP32-S3 para HID; porta USB-serial separada quando disponível para monitor/flash

## Histórico de placas

### ESP32-S3 Zero anterior

- Modelo: ESP32-S3 Zero
- Chip: ESP32-S3FH4R2
- CPU: Xtensa LX7 dual-core (ou single-core dependendo da variante)
- USB: USB nativo integrado ao chip ESP32-S3
- RF: Wi-Fi 2.4 GHz com suporte a ESP-NOW
- Memória: SRAM e flash on-board conforme especificado pela placa

### Observação térmica da ESP32-S3 N16

Registro de 2026-06-26:

- A placa ESP32-S3 N16 atual não apresenta aquecimento perceptível no regulador.
- O aquecimento observado fica no invólucro metálico do microcontrolador e na PCB ao redor do ESP32-S3.
- Medição com termômetro infravermelho indicou aproximadamente 40 °C.
- Esta temperatura, isoladamente, não caracteriza superaquecimento crítico para o ESP32-S3, mas deve continuar sendo monitorada junto com o sensor interno e com a temperatura do regulador.

### LEDs observados na ESP32-S3 N16

Registro de 2026-06-26:

- A placa possui 4 LEDs visíveis.
- Um LED fica piscando verde.
- Dois LEDs ficam vermelhos continuamente.
- Um dos LEDs vermelhos é fisicamente maior.
- Sem o esquema elétrico da placa, não assumir que todos estes LEDs são controláveis por firmware.
- Placas ESP32-S3 N16R8 visualmente semelhantes documentam um LED RGB WS2812 e três LEDs indicadores pequenos para PWR, TX e RX.
- A foto local `foto-esp32s3n16r8.jpeg` mostra um LED quadrado grande com serigrafia `RGB`, compatível com LED RGB endereçável/WS2812.
- Hipótese atual: apenas o LED quadrado `RGB` é controlável pelo firmware como LED de status; os LEDs pequenos provavelmente são ligados à alimentação ou ao conversor USB-serial e não devem ser tratados como LEDs livres de GPIO sem confirmar o esquema elétrico.
- O firmware atual controla apenas o LED de status WS2812 configurado em `GPIO21`; se o WS2812 desta N16 estiver em outro GPIO, ajustar `STATUS_LED_GPIO` em `components/status_led/status_led.c`.

## Objetivo do hardware

O hardware deve operar como um dispositivo USB HID para um host PC, entregando as seguintes funções:

- Mouse USB: movimentos e botões
- Teclado USB: eventos de tecla
- Dispositivo compatível com HID Generic ou Composite USB Device
- Recebimento de comandos sem fio via ESP-NOW de outros módulos ESP32

## Interfaces físicas

### USB

A conexão USB é crítica e deve ser tratada como interface principal:

- VBUS / 5V: alimentação da placa e do chip USB
- D+ / D-: sinais de dados USB nativos do ESP32-S3
- GND: referência comum
- USB ID: não usado em modo dispositivo (devicemode)

O firmware deve configurar o periférico USB nativo do ESP32-S3 como dispositivo HID. A conexão deve ser feita diretamente aos pinos USB do chip, sem multiplexação inadequada.

### Alimentação

- Fonte padrão: 5 V via USB
- Corrente mínima recomendada: 500 mA
- Recomenda-se uso de um regulador interno confiável da placa e capacitores de desacoplamento próximos ao chip
- Boas práticas:
  - adicione capacitor de 10 μF e 100 nF próximo ao conector USB
  - mantenha trilhas curtas e espessas para VBUS e GND
  - evite ruído na linha de alimentação que possa impactar o transceptor Wi-Fi

### Wi-Fi / ESP-NOW

- O ESP32-S3 usa a antena 2.4 GHz integrada ou externa da placa
- ESP-NOW é um protocolo sem conexão e ponto a ponto
- Não há necessidade de roteador ou AP para comunicação
- O firmware deve inicializar Wi-Fi no modo STA para habilitar ESP-NOW
- Recomendação: mantenha a antena longe de fontes de interferência como reguladores de potência ou conectores USB metálicos

## Pinos e periféricos auxiliares

Para prototipagem e funcionalidades adicionais, considere expor os seguintes pinos:

- LED de status
  - GPIO para sinalizar inicialização, conexão ESP-NOW e erro
- Botão de modo ou emparelhamento
  - GPIO para acionar modo de configuração ou reset de pairing
- Pines de expansão
  - SPI, I2C, UART, ADC para sensores ou controles adicionais

Exemplo de mapeamento lógico:

- `GPIOx` - LED de status
- `GPIOy` - botão de ação
- `GPIOz` - entrada digital extra para eventos locais

> Observação: o mapeamento depende da variante exata da placa. Verifique o datasheet e o esquema elétrico da ESP32-S3 Zero antes de usar pinos específicos.

## Requisitos de compatibilidade

- A placa deve suportar o modo de dispositivo USB nativo do ESP32-S3
- O cabo USB deve ser de qualidade e capaz de transmitir dados (não apenas alimentação)
- A configuração do USB deve estar correta no firmware TinyUSB/TUSB
- A antena deve permitir operação estável em 2.4 GHz

## Considerações de design

- Mantenha a separação clara entre hardware e firmware:
  - `hardware.md` deve documentar a plataforma e interfaces físicas
  - o firmware deve conter apenas a lógica de inicialização e controle
- Documente o esquema elétrico completo e o layout de pinos em projetos futuros
- Em projetos de produção, verifique EMC, resistência de sinal e integridade de linha USB

## Teste e validação

- Validar alimentação USB com multímetro e bancada
- Verificar se a placa inicializa corretamente no host via USB
- Confirmar que o dispositivo é detectado como HID no PC
- Testar comunicação ESP-NOW com um dispositivo parceiro

## Referências

- ESP32-S3 Technical Reference Manual
- ESP-IDF USB HID documentation
- ESP-NOW protocol documentation
- Datasheet do ESP32-S3FH4R2

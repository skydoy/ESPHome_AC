# Configuração

Referência de entidades, opções, ecrã, atualizações e organização do repositório. A instalação do dia a dia não precisa desta página.

## Entidades

O componente `hisense_ac` ([`components/hisense_ac`](https://github.com/skydoy/ESPHome_AC/blob/main/components/hisense_ac)) expõe:

| Entidade | Notas |
|---|---|
| Clima | Off / Cool / Heat / Dry / Fan only / Auto, 16–32 °C em passos de 1 °C |
| Velocidades | Auto, Quiet, Low, Medium-Low, Medium, Medium-High, High |
| Swing | Vertical; horizontal e os dois quando `horizontal_swing: true` |
| Predefinições | Eco, Boost (turbo), Sleep, Heat 8C (proteção anti-gelo a +8 °C) |
| Interruptores | LED do visor, Eco, Super (turbo), Fan Mute (silencioso), beep ao comando, AC frame logging (diagnóstico) |
| Select | Sleep Mode: Off, Sleep 1–4 |
| Botões | Light On, Light Off |
| Sensores | Sala (temperatura atual do clima), exterior, serpentina interior e serpentina exterior, frequência real e alvo do compressor, humidade interior, tensão de alimentação, potência, corrente, energia de hoje |
| Diagnóstico | AC link (ligado), texto do estado da ligação, fault (problema), texto dos códigos de avaria, aquecimento auxiliar, um sensor de problema por cada bit de avaria (19, ver abaixo), AC Firmware Version, AC Bus Checksum Errors / AC Bus Framing Errors (contadores desde o arranque). Escondidos por predefinição: Humidity Set Point, AC Panel Unit (Celsius/Fahrenheit), AC Capabilities, AC Device Type. De `core/diagnostics.yaml`: Firmware Version, ESPHome Version, Reset Reason, Uptime, Wi-Fi Signal |

Eco, Super, Fan Mute e Sleep Mode enviam os mesmos comandos que as predefinições do clima e o modo de ventilação Quiet, e seguem o estado que a máquina reporta, por isso as duas vistas ficam sincronizadas. Eco, Super e Sleep são predefinições exclusivas: ligar uma desliga as outras.

Sensores por avaria (todos opcionais, nomes como em [hisense-ac.yaml](https://github.com/skydoy/ESPHome_AC/blob/main/brands/hisense/hisense-ac.yaml)): `fault_indoor_temperature_sensor`, `fault_indoor_coil_sensor`, `fault_indoor_humidity_sensor`, `fault_water_tank_full`, `fault_indoor_fan_motor`, `fault_grille_protection`, `fault_zero_crossing`, `fault_indoor_outdoor_communication` (byte 39); `fault_display_communication`, `fault_keypad_communication`, `fault_wifi_communication`, `fault_electrical`, `fault_indoor_eeprom` (byte 40); `fault_outdoor_eeprom`, `fault_outdoor_coil_sensor`, `fault_discharge_sensor`, `fault_outdoor_temperature_sensor` (byte 64); `fault_overheat_protection`, `fault_high_discharge_temperature` (byte 66).

Face à app ou integração ConnectLife, isto **não** está no barramento RS-485: a capacidade de frio/calor (BTU/h) e a definição "electricity reporting mode" (lado da nuvem). A energia diária é o sensor "Energy Today" (integrado a partir da potência) e a corrente é reportada em A. As notas de protocolo estimam a corrente como `P / (V × 0.95)`. Vê [Questões em aberto](open-questions.md).

Unidades sem bloco elétrico, sensor de humidade ou bloco exterior deixam esses sensores indisponíveis. Humidade, tensão de alimentação e corrente só são publicadas quando o AC envia um valor real, e a config incluída adiciona-as desligadas por predefinição.

O "AC Firmware Version" vem da resposta do AC ao pedido de versão `07`, enviado uma vez no arranque (`handshake: true`). O formato ainda não está confirmado. Vê [protocol.md](https://github.com/skydoy/ESPHome_AC/blob/main/brands/hisense/protocol.md).

Os contadores de erro do barramento são vistos de 10 em 10 s e só são publicados quando mudam. "Quiet" é um modo de ventilação porque o Home Assistant não tem uma predefinição quiet.

## Arranque da ligação e o AEH-W41H1

O módulo W41H1 de origem faz uma sequência de arranque (tipo de dispositivo, versão, heartbeat) antes de pedir o estado, e as unidades interiores enviam tramas de estado longas de 160 bytes. Este componente faz o mesmo (`handshake: true`, predefinição) e descodifica tramas de estado de 82, 150 e 160 bytes. Põe `handshake: false` para uma sondagem simples, como nos componentes comunitários mais antigos. Detalhes em [brands/hisense/protocol.md](https://github.com/skydoy/ESPHome_AC/blob/main/brands/hisense/protocol.md).

## Opções do componente

```yaml
climate:
  - platform: hisense_ac
    name: "Air Conditioner"
    uart_id: ac_uart            # 9600 8N1
    handshake: true             # W41H1-style link bring-up
    horizontal_swing: false     # true if the unit has a motorised horizontal louvre
    sleep_profile: 1            # 1-4, used by the Sleep preset
    beep: true                  # default for the beep switch
    reply_timeout: 1000ms
    cycle_interval: 1000ms
    special_command_gap: 8s     # spacing between eco/turbo/sleep/quiet/8C frames
    debug_frames: false         # log raw frames at DEBUG level (see below)
    debug_frames_interval: 10s  # at most one status frame per interval
```

Cada sub-entidade é opcional. A lista completa está em [brands/hisense/hisense-ac.yaml](https://github.com/skydoy/ESPHome_AC/blob/main/brands/hisense/hisense-ac.yaml).

## Registo de tramas em bruto

Para trabalho de descodificação e calibração. Quando está ligado, o componente regista ao nível DEBUG:

- cada comando que envia (`TX command frame`) e a resposta de estado seguinte;
- uma resposta de estado (`RX status frame`) por `debug_frames_interval` (predefinição 10 s);
- respostas de capacidades (`RX capabilities frame`);
- uma linha `key:` com os bytes ainda em calibração, em decimal (sala, serpentina interior, humidade, compressor, exterior, tensão, potência).

Cada trama é impressa em hexadecimal, 16 bytes por linha, com o offset do byte à frente.

Liga com `debug_frames: true`, ou em tempo real com o interruptor **AC Frame Logging** (diagnóstico, desligado depois de cada arranque) que a config incluída traz, para o firmware pré-compilado o poder usar sem regravar. O logger tem de estar em `DEBUG` ou abaixo (a predefinição).

Para leres o registo sem o painel do ESPHome, usa a API nativa a partir de qualquer computador na mesma rede:

```bash
pip install aioesphomeapi
aioesphomeapi-logs --no-states <device-ip>   # prebuilt firmware has no API encryption key
```

## Ecrã de estado

[`core/screen/m5stickc-plus2.yaml`](https://github.com/skydoy/ESPHome_AC/blob/main/core/screen/m5stickc-plus2.yaml) (independente da marca):

- ecrã de arranque cerca de 2,5 s: logótipo do projeto, "ESPHome AC" e a versão do firmware (substituição `splash_ms`; o crachá é `core/screen/splash_badge.png`);
- temperatura da sala em grande, temperatura alvo e modo;
- indicador de ligação: **OK** (verde), **Handshake** / **Searching** (âmbar), **No link** (vermelho);
- linha de pista: código de avaria, rede de configuração Wi-Fi, reset por brownout, ou uma pista de ligações (`No AC: swap A/B? G0 10k pull-up?`) ao fim de 60 s sem ligação; caso contrário, temperatura exterior e endereço IP.

Durante uma atualização OTA o ecrã liga-se e mostra uma barra de progresso (ou *Update failed*). O ecrã só é redesenhado quando muda algo do que mostra, nunca com a retroiluminação apagada.

A retroiluminação apaga-se ao fim de 30 s. Acorda com o botão A ou B, ou quando mudam o modo, o setpoint, a ligação ou a avaria. Os botões só acordam o ecrã. Nos primeiros 5 minutos depois do arranque o ecrã fica ligado enquanto o Wi-Fi ou a ligação ao AC estão em baixo.

## LED e botão de estado (ATOM Lite)

[`core/boards/atom-lite.yaml`](https://github.com/skydoy/ESPHome_AC/blob/main/core/boards/atom-lite.yaml) não tem ecrã. O LED RGB mostra **azul** (sem Wi-Fi / modo de configuração), **vermelho** (sem ligação ao AC) ou **verde** (ligação OK). O verde apaga-se ao fim de 30 s (`status_led_timeout_ms`, `"0"` = sempre ligado) e fica ligado nos primeiros 5 minutos depois do arranque. Vermelho e azul ficam ligados. Um toque curto no botão mostra outra vez o estado. Se o mantiveres carregado, o LED fica branco. Ao fim de **10 s** faz uma reposição de fábrica. O interruptor de config **Status LED** desliga o LED por completo. `status_led_brightness` (0.0–1.0, predefinição 0.4) define o brilho.

## Atualizações

Há dois tipos de firmware, feitos a partir de dois ficheiros por placa:

| Ficheiro | Quem o usa | Como atualiza |
|---|---|---|
| `devices/<board>.factory.yaml` | O firmware do instalador web (não adotado) | Sozinho de madrugada (**Auto Update**), ou a entidade **Firmware Update** no Home Assistant (OTA por HTTPS a partir do GitHub Pages) |
| `devices/<board>.yaml` (base) | Configs adotadas no ESPHome Device Builder, as tuas próprias builds | **Install** no Device Builder |

Os manifestos de fábrica e os nomes da v0.3.0 estão em [Atualizações](updates.md).

### ESPHome Device Builder

**O YAML adotado é teu.** Adotar escreve um YAML pequeno no Device Builder (nome, chave de API, Wi-Fi) que *referencia* este repositório. O repositório nunca edita esse ficheiro, por isso a tua chave e os extras estão seguros. O que muda é o código para onde a referência aponta.

**Segue a `@stable`.** A adoção escreve isto (desde a v0.2.12):

```yaml
packages:
  skydoy.esphome-ac: github://skydoy/ESPHome_AC/devices/hisense-m5stickc-plus2.yaml@stable
  # ATOM Lite: devices/hisense-atom-lite-tail485.yaml
```

O CI move o ramo `stable` para cada release, por isso **Install** compila sempre a release mais recente. O componente `hisense_ac` está fixo dentro do pacote na tag dessa release (`github://skydoy/ESPHome_AC@v${project_version}`), por isso o YAML e o componente coincidem sempre.

**A release nova ainda não apareceu?** O Device Builder guarda as fontes git em cache até 1 dia. Abre o menu **⋮** da placa → **Clean Build Files** e volta a *Install*.

Queres uma versão fixa? Substitui `@stable` por uma tag, por exemplo `@v0.2.12`. Evita `@main`.

**Erros depois de uma atualização?** **Clean Build Files** e volta a *Install*.

### Encriptação da API e palavra-passe OTA (opcional)

Os pacotes não põem chave de API nem palavra-passe OTA de propósito, para o firmware pré-compilado e cada placa já existente continuarem a funcionar. É por isso que o ESPHome avisa que o tráfego API/OTA vai em claro. Para protegeres a tua build, acrescenta isto ao YAML da placa (funde-se com o pacote):

```yaml
api:
  encryption:
    key: !secret api_key        # openssl rand -base64 32
ota:
  - platform: esphome
    password: !secret ota_password
```

Depois de gravares, o Home Assistant pede a chave nova quando volta a ligar. O Device Builder costuma acrescentar uma chave de API quando adota a placa.

Se uma placa com chave receber o firmware de fábrica (sem chave), o Home Assistant continua a tentar a chave antiga e a placa aparece indisponível. Ou voltas a instalar a tua config a partir do Device Builder (repõe a chave), ou no Home Assistant abres **Definições → Dispositivos e serviços → ESPHome** e segues o pedido de nova autenticação ou *Reconfigure*, deixando a chave de encriptação vazia. Se nenhuma das duas aparecer, apaga aí o dispositivo e volta a adicioná-lo pelo IP.

### Servidor web

As duas configs de placa incluem [`core/web-server.yaml`](https://github.com/skydoy/ESPHome_AC/blob/main/core/web-server.yaml). Abre `http://<ip-da-placa>/` na tua LAN para veres cada entidade, as controlares e leres o registo em direto (servidor web ESPHome v3, porta 80). O JS/CSS da página vem da CDN do ESPHome, por isso o navegador precisa de internet. O firmware em si não precisa.

**Não tem palavra-passe.** Quem estiver na tua rede pode ver e controlar o AC. Mantém a placa numa rede de confiança. Para pores um login, acrescenta isto ao teu YAML (funde-se com o pacote):

```yaml
web_server:
  auth:
    username: admin
    password: !secret web_password
```

Para tirares a página, usa `web_server: !remove` na tua config (ou tira `core/web-server.yaml` dos pacotes). O portal cativo da configuração Wi-Fi não é afetado.

### Sensor Firmware Version

Mostra `esphome: project: version` mais a origem da build entre parênteses: `0.2.12 (release)` para o firmware do instalador/OTA, `0.2.12 (self-built)` quando o Device Builder compila o pacote `github://` adotado (ou quando compilas um checkout), `main @abc1234` para builds de CI de um ramo. Sobrepõe a substituição `firmware_source` na tua config para etiquetares a build (por exemplo `firmware_source: "my fork @1a2b3c4"`, ou `""` só para a versão). Configs sem `esphome: project:` mostram `unknown`.

As cadeias `0.2.12` acima são os exemplos que já estão em [docs/configuration.md](https://github.com/skydoy/ESPHome_AC/blob/main/docs/configuration.md). Não querem dizer que a 0.2.12 é a release atual. Vê o [CHANGELOG.md](https://github.com/skydoy/ESPHome_AC/blob/main/CHANGELOG.md).

### Fuso horário

`core/diagnostics.yaml` tira a hora do Home Assistant (`platform: homeassistant`) e não define fuso, por isso a placa segue o fuso configurado no Home Assistant. Só defines `timezone:` se compilares sem a hora do Home Assistant.

## Organização do repositório

| Caminho | O que é |
|---|---|
| `devices/hisense-m5stickc-plus2.yaml`, `devices/hisense-atom-lite-tail485.yaml` | Configs de base da placa (core + placa + marca). O que o Device Builder adota |
| `devices/*.factory.yaml` | Config de base mais `core/auto-update.yaml`. Compilado pelo CI e oferecido pelo instalador web |
| `core/` | Pacotes independentes da marca: `boards/`, `screen/`, aprovisionamento, diagnóstico, auto-update, verificação de atualização, servidor web |
| `brands/hisense/` | Pacote Hisense, notas de ligações, notas W41H1/W4xx e notas de protocolo |
| `adapters/` | Tabela de adaptadores da camada física (RS-485, TTL UART, P1/P2, IR) |
| `components/hisense_ac/` | O componente externo ESPHome (ESP-IDF; Arduino não testado) |
| `tests/` | Testes de unidade no anfitrião para a camada de protocolo (`make -C tests`) |
| `static/index.html` | Página do instalador ESP Web Tools (escolha de placa, PT/EN, ilustrações) |
| `examples/` | Config para compilares no teu ESPHome Device Builder |

Exemplo para o teu builder: [`examples/my-ac-livingroom.yaml`](https://github.com/skydoy/ESPHome_AC/blob/main/examples/my-ac-livingroom.yaml).

### Compilar localmente

```bash
pip install esphome==2026.10.1
# Use the component from this checkout instead of the release tag on GitHub:
esphome -s hisense_component_source ../components compile devices/hisense-m5stickc-plus2.factory.yaml
make -C tests
```

A versão do ESPHome acima é a que está fixa em [`.github/workflows/publish.yml`](https://github.com/skydoy/ESPHome_AC/blob/main/.github/workflows/publish.yml) e no guia de configuração que já existe.

## Publicar uma versão

As notas de release do próprio repositório dizem: sobe `project_version` nos dois `devices/<board>.yaml` (e a tag no exemplo) e depois faz push de uma tag `vX.Y.Z` igual. O GitHub Actions compila o firmware com ESPHome 2026.10.1, publica o instalador e os manifestos, move a `stable` para a tag e verifica o YAML que o Device Builder gera. Esta pré-visualização da documentação não cria uma release e não muda esse workflow. [CONTRIBUTING.md](https://github.com/skydoy/ESPHome_AC/blob/main/CONTRIBUTING.md).

## Segurança física

- As unidades interiores têm **tensão da rede (230 V AC)** ao lado da porta do módulo Wi-Fi. Desliga o AC **no disjuntor** (não só no comando) e espera uns minutos antes de o abrires.
- As portas dos módulos levam baixa tensão, mas nem sempre 5 V. Algumas marcas põem **12–15 V** no mesmo conector ou barramento. **Mede cada pino com um multímetro** antes de ligares o que quer que seja.
- Nunca ligues GPIOs do ESP32 diretamente a uma porta do AC. Usa o adaptador certo (transceiver RS-485, conversor de nível, transceiver de barramento) de [adapters/README.md](https://github.com/skydoy/ESPHome_AC/blob/main/adapters/README.md).
- Não ultrapasses o que a porta consegue fornecer (a porta AEH-W41H1 está especificada a 5 V / 450 mA).
- Abrir a máquina pode anular a garantia. És responsável pela tua instalação.

## Créditos e proveniência

O componente é uma implementação original (PolyForm Noncommercial 1.0.0, vê a [LICENSE](https://github.com/skydoy/ESPHome_AC/blob/main/LICENSE)). Não foi copiado código-fonte dos projetos abaixo. Serviram como documentação do protocolo e do comportamento real:

- [AndrewDemsDS/hisense-w41h1](https://github.com/AndrewDemsDS/hisense-w41h1) (MIT): arranque da ligação W41H1, modelos de tramas, mapa de estado de 160 bytes e calibração da potência.
- [akrabi/hisense_ac_esphome](https://github.com/akrabi/hisense_ac_esphome) (MIT segundo o README): documentação do protocolo e disposição dos comandos.
- Só factos: [Druidblack/AC-Hisense](https://github.com/Druidblack/AC-Hisense), [pio2398/W4G2](https://github.com/pio2398/W4G2) (GPL-3.0), [straga/scrivo_project](https://github.com/straga/scrivo_project), vins.vins (4pda), [zoffypal/esphome_hisence_ac](https://github.com/zoffypal/esphome_hisence_ac), [pslawinski/esphome_airconintl](https://github.com/pslawinski/esphome_airconintl), [artshevchenko/AC-Hi](https://github.com/artshevchenko/AC-Hi).

O guia no GitHub está só em inglês: [docs/configuration.md](https://github.com/skydoy/ESPHome_AC/blob/main/docs/configuration.md).

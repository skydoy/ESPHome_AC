# Home Assistant

## Adicionar o dispositivo

**Definições → Dispositivos e serviços.** O **Air Conditioner** aparece em *Descobertos* (ESPHome). Em placas instaladas antes da v0.3.0 o nome é **Hisense AC**. Carrega em **Configurar** e atribui-lhe uma divisão (por exemplo, Sala).

**Teste:** põe o **Air Conditioner** em *Cool* a 24 °C e confirma que a máquina arranca.

Se não aparecer, adiciona-o à mão: **Adicionar integração → ESPHome**, o IP da placa, porta `6053`. O Home Assistant e a placa têm de estar na mesma rede. Redes de convidados e o isolamento de clientes impedem a descoberta. Vê [problemas](troubleshooting.md#o-home-assistant-nao-encontra-a-placa).

## O que obténs

| Entidade | Para que serve |
|---|---|
| **Air Conditioner** | Off / Cool / Heat / Dry / Fan / Auto, 16–32 °C, 7 velocidades incluindo silenciosa, swing, predefinições Eco, Boost, Sleep e proteção anti-gelo a 8 °C. |
| **Sensores** | Temperaturas exterior e das serpentinas, frequência do compressor, humidade, tensão, potência, corrente, energia de hoje (quando a máquina os envia). |
| **AC Link**, **AC Fault** | Se a ligação à máquina está bem, e os códigos de avaria. |
| **AC Display**, **Eco**, **Super**, **Fan Mute**, **Sleep Mode** | Luz do painel e os modos extra da própria máquina. |
| **Firmware Update**, **Auto Update** | Novas versões com um clique, ou sozinhas de madrugada. |

Um sensor que a máquina não envia aparece como *unavailable*. A lista completa está em [Configuração](configuration.md#entidades).

## Cartão para o painel

Não há nada para instalar. Abre o painel → **Editar** → **Adicionar cartão** → **Manual**, cola isto e **Guardar**.

Antes de guardares, substitui cada `hisense_ac_b04d18` pelo ID da tua placa. Abre **Definições → Dispositivos e serviços → ESPHome**, abre o teu AC e clica na entidade **Air Conditioner** → a engrenagem. O ID parece `climate.air_conditioner_xxxxxx_air_conditioner` (placas instaladas desde a v0.3.0, por isso usa `air_conditioner_xxxxxx`) ou `climate.hisense_ac_xxxxxx_air_conditioner` (placas instaladas antes da v0.3.0, ou adotadas no Device Builder antes disso).

```yaml
type: vertical-stack
cards:
  - type: thermostat
    entity: climate.hisense_ac_b04d18_air_conditioner
    features:
      - type: climate-hvac-modes
        hvac_modes: [cool, heat, dry, fan_only, auto, "off"]
  - type: entities
    title: AC
    show_header_toggle: false
    entities:
      - entity: sensor.hisense_ac_b04d18_outdoor_temperature
        name: Outdoor
      - entity: sensor.hisense_ac_b04d18_power
        name: Power
      - entity: sensor.hisense_ac_b04d18_energy_today
        name: Energy today
      - entity: binary_sensor.hisense_ac_b04d18_ac_link
        name: AC link
      - entity: binary_sensor.hisense_ac_b04d18_ac_fault
        name: Fault
  - type: history-graph
    title: Last 24 hours
    hours_to_show: 24
    entities:
      - entity: climate.hisense_ac_b04d18_air_conditioner
        name: AC
      - entity: sensor.hisense_ac_b04d18_outdoor_temperature
        name: Outdoor
```

Apaga a linha de qualquer sensor que o teu AC não envie.

## Sensor da sala (Shelly H&T)

O AC mede a temperatura junto ao teto, onde o ar volta a entrar, por isso costuma marcar mais do que a sala. O barramento RS-485 não tem campo para lhe enviar uma temperatura da sala (o iFeel só funciona por infravermelhos). Em vez disso, o dispositivo pode seguir um sensor da sala, ajustando a temperatura que pede ao AC.

| Entidade | O que faz |
|---|---|
| **Room Temperature** | A temperatura da sala. Um Shelly envia-a diretamente (abaixo), ou o Home Assistant define-a. É ignorada se tiver mais de 4 h. |
| **Follow Room Sensor** | Ligado: a temperatura pedida ao AC é corrigida em (sensor do AC − sala), no máximo ±3 °C e no máximo a cada 5 min, até a sala chegar ao teu objetivo. O cartão do AC mostra o teu objetivo e a temperatura da sala. |
| **AC Sensor Temperature** | O que o sensor do próprio AC mede. |

Se mudares a temperatura no AC (comando ou painel), esse passa a ser o novo objetivo. Sem leitura durante 4 h, o AC volta a usar o sensor dele.

**Shelly H&T (Plus / Gen3 / Gen4), sem Home Assistant:** na app do Shelly ou na página dele, abre **Ações** (ou **Webhooks**), cria uma em **Temperature change** com este URL e grava:

```
http://<IP do dispositivo do AC>/number/Room%20Temperature/set?value=${ev.tC}
```

Dá um IP fixo ao dispositivo do AC no router, para o URL continuar a funcionar.

## Velocidade de aquecimento e arrefecimento

**Heating Rate** e **Cooling Rate** mostram quão depressa a sala aquece ou arrefece (°C por hora) enquanto o AC está a aquecer ou a arrefecer, com base nos últimos 30 minutos. Usam o sensor da sala quando o segue; senão, o sensor do AC (em graus inteiros, por isso o valor é mais grosseiro). Ficam vazios quando o AC não está a aquecer nem a arrefecer, e nos primeiros 10 minutos.

## Horários

Há quatro horários guardados no próprio dispositivo, por isso também funcionam com o Home Assistant em baixo. Cada um tem, em **Configuração**:

| Entidade | O que faz |
|---|---|
| **Schedule N** | Ligado = o horário está ativo. |
| **Schedule N Time** | Hora do dia (hora local). |
| **Schedule N Days** | Todos os dias (Every day), dias úteis (Weekdays), fim de semana (Weekend) ou um dia (Monday … Sunday). |
| **Schedule N Action** | Desligar (Off), arrefecer (Cool), aquecer (Heat), Auto, desumidificar (Dry) ou ventilar (Fan). |
| **Schedule N Temperature** | 16–32 °C (não conta para Off e Fan). |
| **Schedule N Fan** | Manter (Keep), Auto, Baixa (Low), Média (Medium), Alta (High) ou Silencioso (Quiet). |

Exemplo: Horário 1 = Weekdays, 07:00, Heat, 21 °C, Auto; Horário 2 = Weekdays, 08:30, Off.

A hora vem do Home Assistant ou, sem ele, da internet (SNTP). O fuso horário é o último que o Home Assistant enviou e fica guardado no dispositivo; até o dispositivo se ligar uma vez ao Home Assistant, usa UTC.

## Luz do painel

**Light On** / **Light Off** ligam e desligam a luz da unidade interior. Em algumas unidades Hisense os dois comandos estão trocados: se o Light On apagar a luz, liga **Invert Panel Light** (Configuração).

## Aviso de limpeza do filtro

O dispositivo conta os dias desde a última limpeza do filtro de ar e avisa quando chega a altura.

| Entidade | O que faz |
|---|---|
| **Filter Cleaning Interval** | Desligado, 30, 60, 90, 182 ou 365 dias (por defeito 365). Com animais ou pó, escolhe 182 ou menos. |
| **Filter Cleaned** | Carrega depois de limpar o filtro: a contagem recomeça a partir de hoje. |
| **Filter Days Left** | Dias até à próxima limpeza (negativo quando já passou). |
| **Filter Needs Cleaning** | Liga quando o intervalo passou. |
| **Filter Last Cleaned** | Data da última limpeza. |

Quando chega a altura, o ecrã do Stick mostra **Clean the AC filter** e o Home Assistant recebe uma notificação. Para a notificação, liga **Permitir que o dispositivo execute ações do Home Assistant**: Definições → Dispositivos e serviços → ESPHome → o teu AC → **Configurar**. Carregar em **Filter Cleaned** apaga a notificação.

A contagem começa no primeiro dia em que o dispositivo recebe a hora do Home Assistant e mantém-se depois de reinícios e atualizações. Os fabricantes costumam aconselhar lavar o filtro de ar a cada 2–4 semanas em uso normal.

## Aviso de atualização

Para placas adotadas no ESPHome Device Builder (têm a entidade **Update Available**), importa este blueprint e cria uma automação que avisa o telemóvel quando sai uma versão nova.

[![Abre a tua instância Home Assistant e mostra o diálogo de importação deste blueprint.](https://my.home-assistant.io/badges/blueprint_import.svg)](https://my.home-assistant.io/redirect/blueprint_import/?blueprint_url=https%3A%2F%2Fraw.githubusercontent.com%2Fskydoy%2FESPHome_AC%2Fstable%2Fblueprints%2Fautomation%2Fesphome_ac_update_available.yaml)

1. Clica no botão acima, depois **Preview** → **Import blueprint**.
2. Clica em **Create automation**, escolhe a entidade **Update Available** do teu AC e o telemóvel, e **Save**.

O telemóvel precisa da app Home Assistant Companion. Sem o botão, abre **Definições → Automações e cenas → Blueprints → Import blueprint** e cola:

`https://raw.githubusercontent.com/skydoy/ESPHome_AC/stable/blueprints/automation/esphome_ac_update_available.yaml`

As placas instaladas pelo instalador web não precisam disto. Atualizam de madrugada com o **Auto Update** ligado, e o Home Assistant lista atualizações pendentes em **Definições → Atualizações**.

O guia no GitHub está só em inglês: [docs/home-assistant.md](https://github.com/skydoy/ESPHome_AC/blob/main/docs/home-assistant.md).

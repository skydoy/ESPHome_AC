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

## Aviso de atualização

Para placas adotadas no ESPHome Device Builder (têm a entidade **Update Available**), importa este blueprint e cria uma automação que avisa o telemóvel quando sai uma versão nova.

[![Abre a tua instância Home Assistant e mostra o diálogo de importação deste blueprint.](https://my.home-assistant.io/badges/blueprint_import.svg)](https://my.home-assistant.io/redirect/blueprint_import/?blueprint_url=https%3A%2F%2Fraw.githubusercontent.com%2Fskydoy%2FESPHome_AC%2Fstable%2Fblueprints%2Fautomation%2Fesphome_ac_update_available.yaml)

1. Clica no botão acima, depois **Preview** → **Import blueprint**.
2. Clica em **Create automation**, escolhe a entidade **Update Available** do teu AC e o telemóvel, e **Save**.

O telemóvel precisa da app Home Assistant Companion. Sem o botão, abre **Definições → Automações e cenas → Blueprints → Import blueprint** e cola:

`https://raw.githubusercontent.com/skydoy/ESPHome_AC/stable/blueprints/automation/esphome_ac_update_available.yaml`

As placas instaladas pelo instalador web não precisam disto. Atualizam de madrugada com o **Auto Update** ligado, e o Home Assistant lista atualizações pendentes em **Definições → Atualizações**.

O guia no GitHub está só em inglês: [docs/home-assistant.md](https://github.com/skydoy/ESPHome_AC/blob/main/docs/home-assistant.md).

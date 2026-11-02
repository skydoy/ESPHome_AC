# Home Assistant

Ready-made pieces for Home Assistant: a dashboard card and an update notification. Nothing to
install: both use only what Home Assistant already includes.

## Dashboard card
Open your dashboard → **✏️ Edit** → **+ Add card** → **Manual**, paste this and **Save**.

Before saving, replace every `hisense_ac_b04d18` with your device's ID. To find it, open
**Settings → Devices & services → ESPHome**, open your AC and click its **Air Conditioner**
entity → ⚙️: the entity ID looks like `climate.air_conditioner_xxxxxx_air_conditioner`
(boards installed since v0.3.0, so replace with `air_conditioner_xxxxxx`) or
`climate.hisense_ac_xxxxxx_air_conditioner` (boards installed before v0.3.0, or adopted in the
Device Builder before then).

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

A sensor your AC does not report shows as *unavailable*; delete its line.

## Update notification
For devices adopted in the ESPHome Device Builder (they have an **Update Available** entity):
get a phone notification when a new release is out.

[![Open your Home Assistant instance and show the blueprint import dialog with this blueprint pre-filled.](https://my.home-assistant.io/badges/blueprint_import.svg)](https://my.home-assistant.io/redirect/blueprint_import/?blueprint_url=https%3A%2F%2Fraw.githubusercontent.com%2Fskydoy%2FESPHome_AC%2Fstable%2Fblueprints%2Fautomation%2Fesphome_ac_update_available.yaml)

1. Click the button above, then **Preview** → **Import blueprint**.
2. Click **Create automation**, pick your AC's **Update Available** entity and your phone, and
   **Save**.

The phone needs the Home Assistant Companion app. No button? In Home Assistant open
**Settings → Automations & scenes → Blueprints → Import blueprint** and paste:
`https://raw.githubusercontent.com/skydoy/ESPHome_AC/stable/blueprints/automation/esphome_ac_update_available.yaml`

Devices installed from the web installer do not need this: they update by themselves at night
(**Auto Update** switch), and Home Assistant lists pending updates under **Settings → Updates**.

# Home Assistant

## Add the device

**Settings → Devices & services.** **Air Conditioner** shows up under *Discovered* (ESPHome). On boards installed before v0.3.0 the name is **Hisense AC**. Press **Configure** and give it an area (for example Living room).

**Test:** set **Air Conditioner** to *Cool* at 24 °C and check that the unit starts.

If it does not appear, add it by hand: **Add integration → ESPHome**, the device IP, port `6053`. Home Assistant and the device must be on the same network. Guest Wi-Fi and client isolation block discovery. See [troubleshooting](troubleshooting.md#home-assistant-does-not-find-the-device).

## What you get

| Entity | What it does |
|---|---|
| **Air Conditioner** | Off / Cool / Heat / Dry / Fan / Auto, 16–32 °C, 7 fan speeds including Quiet, swing, presets Eco, Boost, Sleep and 8 °C frost protection. |
| **Sensors** | Outdoor and coil temperatures, compressor frequency, humidity, voltage, power, current, energy today (when the unit reports them). |
| **AC Link**, **AC Fault** | Whether the link to the unit is up, and its fault codes. |
| **AC Display**, **Eco**, **Super**, **Fan Mute**, **Sleep Mode** | Panel light and the unit's own extra modes. |
| **Firmware Update**, **Auto Update** | New versions in one click, or by themselves at night. |

A sensor the unit does not report shows as *unavailable*. The full list is on [Configuration](configuration.md#entities).

## Dashboard card

Nothing to install. Open your dashboard → **Edit** → **Add card** → **Manual**, paste this and **Save**.

Before saving, replace every `hisense_ac_b04d18` with your device's ID. Open **Settings → Devices & services → ESPHome**, open your AC and click its **Air Conditioner** entity → the gear icon. The entity ID looks like `climate.air_conditioner_xxxxxx_air_conditioner` (boards installed since v0.3.0, so use `air_conditioner_xxxxxx`) or `climate.hisense_ac_xxxxxx_air_conditioner` (boards installed before v0.3.0, or adopted in the Device Builder before then).

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

Delete the line of any sensor your AC does not report.

## Update notification

For devices adopted in the ESPHome Device Builder (they have an **Update Available** entity), import this blueprint and create an automation that notifies your phone when a new release is out.

[![Open your Home Assistant instance and show the blueprint import dialog with this blueprint pre-filled.](https://my.home-assistant.io/badges/blueprint_import.svg)](https://my.home-assistant.io/redirect/blueprint_import/?blueprint_url=https%3A%2F%2Fraw.githubusercontent.com%2Fskydoy%2FESPHome_AC%2Fstable%2Fblueprints%2Fautomation%2Fesphome_ac_update_available.yaml)

1. Click the button above, then **Preview** → **Import blueprint**.
2. Click **Create automation**, pick your AC's **Update Available** entity and your phone, and **Save**.

The phone needs the Home Assistant Companion app. Without the button, open **Settings → Automations & scenes → Blueprints → Import blueprint** and paste:

`https://raw.githubusercontent.com/skydoy/ESPHome_AC/stable/blueprints/automation/esphome_ac_update_available.yaml`

Devices installed from the web installer do not need this. They update at night when **Auto Update** is on, and Home Assistant lists pending updates under **Settings → Updates**.

Repository copy: [docs/home-assistant.md](https://github.com/skydoy/ESPHome_AC/blob/main/docs/home-assistant.md).

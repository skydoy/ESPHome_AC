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

## Room sensor (Shelly H&T)

The AC measures the temperature near the ceiling, where its air comes back in, so it often reads warmer than the room. The RS-485 bus has no field to send it a room temperature (iFeel only works over infrared). Instead, the device can follow a room sensor by adjusting the set point it sends to the AC.

| Entity | What it does |
|---|---|
| **Room Temperature** | The room reading. A Shelly sends it directly (below), or Home Assistant sets it. Ignored when older than 4 h. |
| **Follow Room Sensor** | On: the AC set point is shifted by (AC sensor − room), at most ±3 °C and at most every 5 min, until the room reaches your target. The AC card shows your target and the room temperature. |
| **AC Sensor Temperature** | What the AC's own sensor reads. |

A set point changed at the AC (remote or panel) becomes the new target. Without a reading for 4 h, the AC goes back to its own sensor.

**Shelly H&T (Plus / Gen3 / Gen4), without Home Assistant:** in the Shelly app or its web page, open **Actions** (or **Webhooks**), add one on **Temperature change** with this URL, and save:

```
http://<AC device IP>/number/Room%20Temperature/set?value=${ev.tC}
```

Give the AC device a fixed IP in your router so the URL keeps working.

## Heating and cooling rate

**Heating Rate** and **Cooling Rate** show how fast the room warms up or cools down (°C per hour) while the AC is heating or cooling, from the last 30 minutes. They use the room sensor when following it, otherwise the AC's sensor (whole degrees, so the value is rougher). They stay empty while the AC is not heating or cooling, and for the first 10 minutes.

## Schedules

Four schedules are stored on the device, so they also run when Home Assistant is down. Each one has, under **Configuration**:

| Entity | What it does |
|---|---|
| **Schedule N** | On = the schedule is active. |
| **Schedule N Time** | Time of day (local time). |
| **Schedule N Days** | Every day, Weekdays, Weekend, or one day (Monday … Sunday). |
| **Schedule N Action** | Off, Cool, Heat, Auto, Dry or Fan. |
| **Schedule N Temperature** | 16–32 °C (not used by Off and Fan). |
| **Schedule N Fan** | Keep (leave as is), Auto, Low, Medium, High or Quiet. |

Example: Schedule 1 = Weekdays, 07:00, Heat, 21 °C, Auto; Schedule 2 = Weekdays, 08:30, Off.

The clock comes from Home Assistant, or from internet time (SNTP) without it. The time zone is the one Home Assistant last sent, saved on the device; until the device has connected to Home Assistant once, it uses UTC.

## Panel light

**Light On** / **Light Off** switch the light on the indoor unit. Some Hisense units swap the two commands: if Light On turns the light off, turn on **Invert Panel Light** (Configuration).

## Filter cleaning reminder

The device counts the days since the air filter was last cleaned and warns you when it is time.

| Entity | What it does |
|---|---|
| **Filter Cleaning Interval** | Off, 30, 60, 90, 182 or 365 days (default 365). Pick 182 or less with pets or dust. |
| **Filter Cleaned** | Press it after cleaning the filter: the count starts again from today. |
| **Filter Days Left** | Days until the next cleaning (negative when overdue). |
| **Filter Needs Cleaning** | On when the interval has passed. |
| **Filter Last Cleaned** | Date of the last cleaning. |

When it is time, the Stick screen shows **Clean the AC filter** and Home Assistant gets a notification. For the notification, turn on **Allow the device to perform Home Assistant actions**: Settings → Devices & services → ESPHome → your AC → **Configure**. Pressing **Filter Cleaned** removes the notification.

The count starts on the first day the device gets the time from Home Assistant, and it survives restarts and updates. Air-filter makers usually advise washing the filter every 2–4 weeks in regular use.

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

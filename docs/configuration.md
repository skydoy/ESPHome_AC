# Configuration and details

## Entities
The `hisense_ac` component (in [`components/hisense_ac`](../components/hisense_ac)) exposes:

| Entity | Notes |
|---|---|
| Climate | Off / Cool / Heat / Dry / Fan only / Auto, 16–32 °C in 1 °C steps |
| Fan modes | Auto, Quiet, Low, Medium-Low, Medium, Medium-High, High |
| Swing | Vertical; horizontal and both when `horizontal_swing: true` |
| Presets | Eco, Boost (turbo), Sleep, Heat 8C (+8 °C frost protection) |
| Switches | AC display LED, Eco, Super (turbo), Fan Mute (quiet), beep on command, AC frame logging (diagnostic) |
| Select | Sleep Mode: Off, Sleep 1–4 |
| Buttons | Light On, Light Off |
| Sensors | Room (climate current temperature), outdoor, indoor coil and outdoor coil temperatures, compressor actual and target frequency, indoor humidity, supply voltage, power, current, energy today |
| Diagnostics | AC link (connected), link status text, fault (problem), fault codes text, auxiliary heater, one problem sensor per fault bit (19, see below); from `core/diagnostics.yaml`: Firmware Version, ESPHome Version, Reset Reason, Uptime, Wi-Fi Signal |

Eco, Super, Fan Mute and Sleep Mode drive the same commands as the climate presets and the Quiet
fan mode, and follow the AC's reported state, so both views stay in sync. Eco, Super and Sleep
are mutually exclusive presets: turning one on turns the others off.

Per-fault sensors (all optional, names as in [hisense-ac.yaml](../brands/hisense/hisense-ac.yaml)):
`fault_indoor_temperature_sensor`, `fault_indoor_coil_sensor`, `fault_indoor_humidity_sensor`,
`fault_water_tank_full`, `fault_indoor_fan_motor`, `fault_grille_protection`,
`fault_zero_crossing`, `fault_indoor_outdoor_communication` (byte 39);
`fault_display_communication`, `fault_keypad_communication`, `fault_wifi_communication`,
`fault_electrical`, `fault_indoor_eeprom` (byte 40); `fault_outdoor_eeprom`,
`fault_outdoor_coil_sensor`, `fault_discharge_sensor`, `fault_outdoor_temperature_sensor`
(byte 64); `fault_overheat_protection`, `fault_high_discharge_temperature` (byte 66).

Compared with the ConnectLife app/integration, these are **not** available on the RS-485 bus:
cooling/heating capacity (BTU/h) and the "electricity reporting mode" setting (cloud side).
Daily energy is the "Energy Today" sensor (integrated from power) and current is reported in A.

Units without an electrical block, humidity sensor or outdoor block simply leave those sensors
unavailable. Humidity, supply voltage and current are not published at all until the AC sends a
real value, and the bundled config adds them disabled by default. "Quiet" is a fan mode because Home Assistant has no quiet preset.

## Link bring-up and the AEH-W41H1
The stock W41H1 module performs a bring-up sequence (device type, version, heartbeat) before
polling status, and its indoor units send long 160-byte status frames. This component does the
same (`handshake: true`, default) and decodes 82-, 150- and 160-byte status frames. Set
`handshake: false` for plain polling like the older community components. Details in
[brands/hisense/protocol.md](../brands/hisense/protocol.md).

## Component options
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
    # Every sub-entity is optional, see [brands/hisense/hisense-ac.yaml](../brands/hisense/hisense-ac.yaml) for the full list.
```

## Raw frame logging
For decoding and calibration work. When on, the component logs at DEBUG level:

- every command it sends (`TX command frame`) and the next status reply;
- one status reply (`RX status frame`) per `debug_frames_interval` (default 10 s);
- capability replies (`RX capabilities frame`);
- a `key:` line with the bytes still being calibrated in decimal (room, indoor coil, humidity,
  compressor, outdoor, voltage, power).

Each frame is printed as hex, 16 bytes per line with the byte offset in front.

Turn it on with `debug_frames: true`, or at runtime with the **AC Frame Logging** switch
(diagnostic, off after every boot) that the bundled config includes, so prebuilt firmware can
use it without reflashing. The logger must be at `DEBUG` or lower (the default).

To read the log without the ESPHome dashboard, use the native API from any computer on the
same network:

```bash
pip install aioesphomeapi
aioesphomeapi-logs --no-states <device-ip>   # prebuilt firmware has no API encryption key
```

## Status screen
[`core/screen/m5stickc-plus2.yaml`](../core/screen/m5stickc-plus2.yaml) (brand agnostic):

- boot splash for about 2.5 s: project logo, "ESPHome AC" and the firmware version
  (`splash_ms` substitution; the badge is `core/screen/splash_badge.png`);
- big room temperature, target temperature and mode;
- link indicator: **OK** (green), **Handshake** / **Searching** (amber), **No link** (red);
- hint line: fault code, Wi-Fi setup hotspot, brownout reset, or a wiring hint
  (`No AC: swap A/B? G0 10k pull-up?`) after 60 s without a link; otherwise outdoor
  temperature and IP address.

During an OTA update the screen turns on and shows a progress bar (or *Update failed*). The
screen is redrawn only when something it shows changes, never while the backlight is off.

The backlight turns off after 30 s. It wakes on button A or B, or when the mode, set point,
link or fault changes. The buttons only wake the screen. During the first 5 minutes after boot
the screen stays on while Wi-Fi or the AC link is down.

## Status LED and button (ATOM Lite)
[`core/boards/atom-lite.yaml`](../core/boards/atom-lite.yaml) has no screen. The RGB LED shows
**blue** (no Wi-Fi / setup mode), **red** (no AC link) or **green** (link OK). Green turns off
after 30 s (`status_led_timeout_ms`, `"0"` = always on) and stays on during the first 5 minutes
after boot; red and blue stay on. A short press on the button shows the status again. Holding it
turns the LED white; after **10 s** it does a factory reset. The **Status LED** config switch
turns the LED off entirely. `status_led_brightness` (0.0–1.0, default 0.4) sets the brightness.

## Updates
There are two kinds of firmware, built from two files per board:

| File | Who uses it | How it updates |
|---|---|---|
| `devices/<board>.factory.yaml` | The web installer firmware (not adopted) | **Firmware Update** entity in Home Assistant (OTA over HTTPS from GitHub Pages) |
| `devices/<board>.yaml` (base) | Configs adopted in the ESPHome Device Builder, your own builds | **Install** in the Device Builder |

- **Factory firmware:** the `Firmware Update` entity installs new releases from the board's manifest:
  `https://skydoy.github.io/ESPHome_AC/firmware/manifest.json` (Stick) or
  `https://skydoy.github.io/ESPHome_AC/firmware/atom-lite-tail485/manifest.json` (ATOM).
There are two ways to update, and neither needs a YAML edit:
1. **Not adopted** (factory firmware from the installer): press **Install** on the
   **Firmware Update** entity in Home Assistant. One click.
2. **Adopted in the ESPHome Device Builder:** your YAML imports `@stable`; just press
   **Install** → *Wirelessly*. The adopted build has no `Firmware Update` entity on purpose
   (the generic firmware would replace your config and drop your API key).
- **Is there a new release?** Adopted builds have **Update Available** and **Latest Version**
  (diagnostic), read from the board's manifest at boot and every 6 h, plus a **Check for Update**
  button. They only report; nothing is installed (`core/update-check.yaml`).
- Adopted before v0.2.9? Those builds still include the update entity. Do not press *Install* on
  it; press **Install** in the Device Builder once and the entity disappears.
- Progress: an update takes about 1-2 minutes. The download blocks the device's main loop, so
  Home Assistant shows no progress and the device looks offline until it reboots with the new
  version. The Stick's screen shows a progress bar; the live log (Device Builder, or
  `http://<device-ip>/` until the update starts) shows `Progress: NN%`.
- Build it yourself from remote packages: [`examples/my-ac-livingroom.yaml`](../examples/my-ac-livingroom.yaml).

### ESPHome Device Builder
**Your adopted YAML is yours.** Adopting writes a small YAML file in the Device Builder (name,
API key, Wi-Fi) that *references* this repository. The repository never edits that file, so
your key and extras are safe; what changes is the code that the reference points to.

**Follow `@stable`.** Adoption writes this (since v0.2.12):

```yaml
packages:
  skydoy.esphome-ac: github://skydoy/ESPHome_AC/devices/hisense-m5stickc-plus2.yaml@stable
  # ATOM Lite: devices/hisense-atom-lite-tail485.yaml
```

The `stable` branch is moved by CI to every release, so **Install** always builds the latest
release. The `hisense_ac` component is pinned inside the package to that release's tag
(`github://skydoy/ESPHome_AC@v${project_version}`), so YAML and component always match.

**New release not picked up yet?** The Device Builder caches git sources for up to 1 day. Open
the device's ⋮ menu → **Clean Build Files**, then *Install* again to fetch it now.

Want a fixed version instead? Replace `@stable` with a tag such as `@v0.2.12`. Avoid `@main`
(development only, and it can mix new YAML with an old cached component).

**Errors after an update?** Open the device's ⋮ menu → **Clean Build Files**, then
*Install* again.

### API encryption and OTA password (optional)
The packages deliberately set no API key and no OTA password, so the pre-built firmware and
every existing device keep working; this is why ESPHome warns that API/OTA traffic is plaintext.
To secure your own build, add this to your device YAML (it merges with the package):

```yaml
api:
  encryption:
    key: !secret api_key        # openssl rand -base64 32
ota:
  - platform: esphome
    password: !secret ota_password
```

After flashing, Home Assistant asks for the new key when it reconnects. The Device Builder usually
adds an API key for you when it adopts the device.

If a device with a key gets the factory firmware (no key), Home Assistant keeps trying the old key
and the device shows as unavailable. Either install your own config again from the Device
Builder (restores the key), or in Home Assistant open **Settings → Devices & services →
ESPHome** and follow the re-authentication prompt or *Reconfigure* the device, leaving the
encryption key empty (if neither is offered, delete the device there and add it again by IP).

### Web server
Both device configs include [`core/web-server.yaml`](../core/web-server.yaml): open
`http://<device-ip>/` on your LAN to see every entity, control them and read the live log
(ESPHome web server v3, port 80). The page's JS/CSS loads from the ESPHome CDN, so the browser
needs internet access; the firmware itself does not.

**It has no password:** anyone on your network can view and control the AC. Keep the device on
a trusted network. To add a login, add this to your own device YAML (it merges with the package):

```yaml
web_server:
  auth:
    username: admin
    password: !secret web_password
```

To remove the page, use `web_server: !remove` in your config (or drop
`core/web-server.yaml` from its packages). The captive portal for Wi-Fi setup is unaffected.

### Firmware Version sensor
Shows `esphome: project: version` plus the build source in brackets: `0.2.12 (release)` for the
web-installer/OTA firmware, `0.2.12 (self-built)` when the Device Builder builds the adopted
`github://` package (or you build a checkout), `main @abc1234` for CI builds of a branch.
Override the `firmware_source` substitution in your own config to label your build
(e.g. `firmware_source: "my fork @1a2b3c4"`, or `""` for the version only). Configs without
`esphome: project:` show `unknown`.

### Time zone
`core/diagnostics.yaml` takes the time from Home Assistant (`platform: homeassistant`) and sets
no time zone, so the device follows the zone configured in Home Assistant. Only set
`timezone:` yourself if you build without Home Assistant time.

## Repository layout
| Path | What |
|---|---|
| `devices/hisense-m5stickc-plus2.yaml`, `devices/hisense-atom-lite-tail485.yaml` | Base device configs (core + board + brand); what the Device Builder adopts |
| `devices/*.factory.yaml` | Base config + `core/auto-update.yaml`; built by CI and offered by the web installer |
| `core/` | Brand-agnostic packages: `boards/`, `screen/`, provisioning, diagnostics, auto-update, update check, web server |
| `brands/hisense/` | Hisense package, wiring, W41H1/W4xx notes and protocol notes |
| `adapters/` | Physical-layer adapters table (RS-485, TTL UART, P1/P2, IR) |
| `components/hisense_ac/` | The ESPHome external component (ESP-IDF; Arduino untested) |
| `tests/` | Host unit tests for the protocol layer (`make -C tests`) |
| `static/index.html` | ESP Web Tools installer page with brand selector |
| `examples/` | Config for building in your own ESPHome Device Builder |

### Building locally
```bash
pip install esphome==2026.10.1
# Use the component from this checkout instead of the release tag on GitHub:
esphome -s hisense_component_source ../components compile devices/hisense-m5stickc-plus2.factory.yaml
make -C tests
```

## Releasing
Bump `project_version` in both `devices/<board>.yaml` (and the tag in the example), then push a
matching tag `vX.Y.Z`: GitHub Actions builds the firmware with ESPHome 2026.10.1, then publishes
the installer page and `firmware/manifest.json` to GitHub Pages (Settings → Pages → Source:
**GitHub Actions**). Devices running the factory firmware pick up the new version through the
`Firmware Update` entity. After the release, CI moves the `stable` branch to the tag (adopted
devices then just press **Install**) and validates the exact YAML the Device Builder generates
on adoption (`@stable`), straight from GitHub.

## Safety
- Indoor units have **mains voltage (230 V AC)** next to the Wi-Fi module port. Switch the AC
  off **at the breaker** (not just the remote) and wait a few minutes before opening it.
- Module ports carry low voltage but not always 5 V: some brands put **12-15 V** on the same
  connector or bus. **Measure every pin with a multimeter** before connecting anything.
- Never connect ESP32 GPIOs directly to an AC port. Use the matching adapter (RS-485
  transceiver, level shifter, bus transceiver) from [adapters/README.md](../adapters/README.md).
- Do not exceed what the port can supply (the AEH-W41H1 port is rated 5 V / 450 mA).
- Opening the unit may void its warranty. You are responsible for your installation.

## Credits and provenance
The component is an original implementation (PolyForm Noncommercial 1.0.0, see [LICENSE](../LICENSE)). No source code was copied from the projects
below; they were used as documentation of the protocol and of real-world behaviour:

- [AndrewDemsDS/hisense-w41h1](https://github.com/AndrewDemsDS/hisense-w41h1) (MIT): W41H1 link
  bring-up, frame templates, 160-byte status map and power calibration. Also a good
  alternative component if this one does not fit your unit.
- [akrabi/hisense_ac_esphome](https://github.com/akrabi/hisense_ac_esphome) (MIT per its README):
  protocol documentation and command layout.
- Facts only: [Druidblack/AC-Hisense](https://github.com/Druidblack/AC-Hisense) (150-byte frames,
  power), [pio2398/W4G2](https://github.com/pio2398/W4G2) (GPL-3.0; status byte notes),
  [straga/scrivo_project](https://github.com/straga/scrivo_project), vins.vins (4pda),
  [zoffypal/esphome_hisence_ac](https://github.com/zoffypal/esphome_hisence_ac),
  [pslawinski/esphome_airconintl](https://github.com/pslawinski/esphome_airconintl),
  [artshevchenko/AC-Hi](https://github.com/artshevchenko/AC-Hi).

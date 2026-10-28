# Configuration and details

## Entities
The `hisense_ac` component (in [`components/hisense_ac`](../components/hisense_ac)) exposes:

| Entity | Notes |
|---|---|
| Climate | Off / Cool / Heat / Dry / Fan only / Auto, 16–32 °C in 1 °C steps |
| Fan modes | Auto, Quiet, Low, Medium-Low, Medium, Medium-High, High |
| Swing | Vertical; horizontal and both when `horizontal_swing: true` |
| Presets | Eco, Boost (turbo), Sleep, Heat 8C (+8 °C frost protection) |
| Switches | AC display LED, beep on command |
| Sensors | Room (climate current temperature), outdoor, indoor coil and outdoor coil temperatures, compressor actual and target frequency, indoor humidity, supply voltage, power, current, energy today |
| Diagnostics | AC link (connected), link status text, fault (problem), fault codes text, auxiliary heater |

Units without an electrical block, humidity sensor or outdoor block simply leave those sensors
unavailable. "Quiet" is a fan mode because Home Assistant has no quiet preset.

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
    # Every sub-entity is optional, see [brands/hisense/hisense-ac.yaml](../brands/hisense/hisense-ac.yaml) for the full list.
```

## Status screen
[`core/screen/m5stickc-plus2.yaml`](../core/screen/m5stickc-plus2.yaml) (brand agnostic):

- big room temperature, target temperature and mode;
- link indicator: **OK** (green), **Handshake** / **Searching** (amber), **No link** (red);
- hint line: fault code, Wi-Fi setup hotspot, brownout reset, or a wiring hint
  (`No AC: swap A/B? G0 10k pull-up?`) after 60 s without a link; otherwise outdoor
  temperature and IP address.

The backlight turns off after 30 s. It wakes on button A or B, or when the mode, set point,
link or fault changes. The buttons only wake the screen. During the first 5 minutes after boot
the screen stays on while Wi-Fi or the AC link is down.

## Updates
- **OTA:** the `Firmware Update` entity installs new releases from
  `https://skydoy.github.io/ESPHome_AC/firmware/manifest.json`.
- **Adopt:** the device announces `dashboard_import`, so the ESPHome Device Builder can adopt
  it and build your own copy from [`devices/hisense-m5stickc-plus2.yaml`](../devices/hisense-m5stickc-plus2.yaml).
- Build it yourself from remote packages: [`examples/my-ac-livingroom.yaml`](../examples/my-ac-livingroom.yaml).

## Repository layout
| Path | What |
|---|---|
| `devices/hisense-m5stickc-plus2.yaml` | Device config (core + board + brand) built by CI and offered by the web installer |
| `core/` | Brand-agnostic packages: `boards/`, `screen/`, provisioning, diagnostics, auto-update |
| `brands/hisense/` | Hisense package, wiring, W41H1/W4xx notes and protocol notes |
| `adapters/` | Physical-layer adapters table (RS-485, TTL UART, P1/P2, IR) |
| `components/hisense_ac/` | The ESPHome external component (ESP-IDF; Arduino untested) |
| `tests/` | Host unit tests for the protocol layer (`make -C tests`) |
| `static/index.html` | ESP Web Tools installer page with brand selector |
| `examples/` | Config for building in your own ESPHome Device Builder |

### Building locally
```bash
pip install esphome==2026.10.1
# Use the component from this checkout instead of github://...@main:
esphome -s hisense_component_source ../components compile devices/hisense-m5stickc-plus2.yaml
make -C tests
```

## Releasing
Push a tag `vX.Y.Z`: GitHub Actions builds the firmware with ESPHome 2026.10.1, then publishes
the installer page and `firmware/manifest.json` to GitHub Pages (Settings → Pages → Source:
**GitHub Actions**). Devices pick up the new version through the `Firmware Update` entity.

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

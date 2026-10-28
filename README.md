# ESPHome AC

[![Build](https://github.com/skydoy/ESPHome_AC/actions/workflows/publish.yml/badge.svg)](https://github.com/skydoy/ESPHome_AC/actions/workflows/publish.yml)
[![Validate](https://github.com/skydoy/ESPHome_AC/actions/workflows/validate.yml/badge.svg)](https://github.com/skydoy/ESPHome_AC/actions/workflows/validate.yml)
[![Release](https://img.shields.io/github/v/release/skydoy/ESPHome_AC?sort=semver)](https://github.com/skydoy/ESPHome_AC/releases/latest)
[![License: MIT](https://img.shields.io/github/license/skydoy/ESPHome_AC)](LICENSE)

Replace your **Hisense** AC's Wi-Fi module (AEH-W4x / **AEH-W41H1**) with an **M5StickC Plus2 +
RS485 Hat** and control it locally from Home Assistant. No cloud, no app.

**[Installer](https://skydoy.github.io/ESPHome_AC/)** · [Wiring](docs/wiring.md) ·
[Configuration](docs/configuration.md) · [Protocol](brands/hisense/protocol.md) ·
[Releases](https://github.com/skydoy/ESPHome_AC/releases)

## Install in 3 steps
1. **Flash:** plug the Stick into your computer by USB and open the
   **[web installer](https://skydoy.github.io/ESPHome_AC/)** (Chrome or Edge) → *Connect* → *Install*.
2. **Wire:** AC off at the breaker, measure the plug, connect it as shown below
   ([full guide](docs/wiring.md)).
3. **Connect:** join the `Hisense-AC Setup` Wi-Fi hotspot (or use Improv in the installer or
   over Bluetooth), then adopt the device in Home Assistant → **Settings → Devices → ESPHome**.

Updates then arrive over the air through the **Firmware Update** entity.

[![Wiring diagram](docs/wiring.svg)](docs/wiring.md)

> ⚠️ **G0 boot pin:** if the Stick boots from USB but not from the AC, add a 10 kΩ pull-up from
> G0 to 3.3 V. [Details](docs/wiring.md#3-boot-pin-g0-read-this-before-closing-the-ac)

## Features
- Climate: Off / Cool / Heat / Dry / Fan / Auto, 16–32 °C, 7 fan speeds incl. Quiet, swing
- Presets: Eco, Boost, Sleep, 8 °C frost protection
- Sensors: room, outdoor and coil temperatures, compressor frequency, humidity, voltage,
  power, current, energy today (when the unit reports them)
- AEH-W41H1 link handshake and 82 / 150 / 160-byte status frames
- Diagnostics: link state, fault codes, reset reason
- Status screen with auto-off; OTA auto-update; `dashboard_import` adoption

## Links
| What | Where |
|---|---|
| Web installer | https://skydoy.github.io/ESPHome_AC/ |
| Wiring (5 V and 12 V) | [docs/wiring.md](docs/wiring.md) |
| Configuration, entities, screen | [docs/configuration.md](docs/configuration.md) |
| Hisense notes and modules | [brands/hisense/README.md](brands/hisense/README.md) |
| Hardware details | [brands/hisense/hardware.md](brands/hisense/hardware.md) |
| Protocol | [brands/hisense/protocol.md](brands/hisense/protocol.md) |
| Adapters and other brands | [adapters/README.md](adapters/README.md) |
| Dashboard import / own build | [devices/hisense-m5stickc-plus2.yaml](devices/hisense-m5stickc-plus2.yaml), [examples/](examples/my-ac-livingroom.yaml) |
| Contributing (new brands) | [CONTRIBUTING.md](CONTRIBUTING.md) |
| Releases and changelog | [Releases](https://github.com/skydoy/ESPHome_AC/releases) |
| Issues | [Issues](https://github.com/skydoy/ESPHome_AC/issues) |

Only Hisense is supported today. Other brands (Mitsubishi, Daikin, Midea, …) wait for testers
with real hardware; see [CONTRIBUTING.md](CONTRIBUTING.md).

## Credits
Original MIT implementation; no code copied. Protocol knowledge from
[AndrewDemsDS/hisense-w41h1](https://github.com/AndrewDemsDS/hisense-w41h1),
[akrabi/hisense_ac_esphome](https://github.com/akrabi/hisense_ac_esphome),
[Druidblack/AC-Hisense](https://github.com/Druidblack/AC-Hisense),
[pio2398/W4G2](https://github.com/pio2398/W4G2),
[zoffypal/esphome_hisence_ac](https://github.com/zoffypal/esphome_hisence_ac),
[straga/scrivo_project](https://github.com/straga/scrivo_project),
[pslawinski/esphome_airconintl](https://github.com/pslawinski/esphome_airconintl) and
[artshevchenko/AC-Hi](https://github.com/artshevchenko/AC-Hi).

## License
[MIT](LICENSE). Use at your own risk: the indoor unit has mains voltage inside. Hisense is a
trademark of its owner; this project is not affiliated with it.

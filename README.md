<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/logo-dark.png">
    <source media="(prefers-color-scheme: light)" srcset="docs/logo-light.png">
    <img src="docs/logo-light.png" alt="ESPHome_AC" width="256">
  </picture>
</p>

<p align="center"><b>Like it? Help me build something better.</b><br>
  <a href="https://ko-fi.com/skydoy"><img src="https://img.shields.io/badge/Ko--fi-Support%20me-FF5E5B?logo=ko-fi&logoColor=white" alt="Support me on Ko-fi"></a>
  <a href="https://buymeacoffee.com/skydoy"><img src="https://img.shields.io/badge/Buy%20Me%20a%20Coffee-Support%20me-FFDD00?logo=buymeacoffee&logoColor=black" alt="Buy Me a Coffee"></a>
</p>

<p align="center">
  <a href="https://github.com/skydoy/ESPHome_AC/actions/workflows/publish.yml"><img src="https://github.com/skydoy/ESPHome_AC/actions/workflows/publish.yml/badge.svg" alt="Build"></a>
  <a href="https://github.com/skydoy/ESPHome_AC/actions/workflows/validate.yml"><img src="https://github.com/skydoy/ESPHome_AC/actions/workflows/validate.yml/badge.svg" alt="Validate"></a>
  <a href="https://github.com/skydoy/ESPHome_AC/releases/latest"><img src="https://img.shields.io/github/v/release/skydoy/ESPHome_AC?sort=semver" alt="Release"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-PolyForm%20Noncommercial%201.0.0-blue" alt="License: PolyForm Noncommercial"></a>
</p>

<p align="center"><b>Your air conditioner in Home Assistant. Plug in by USB, click Install, done. No YAML.</b><br>
🇵🇹 <a href="README.pt.md">Ler em português</a></p>

<p align="center">
  <a href="https://skydoy.github.io/ESPHome_AC/"><img src="https://img.shields.io/badge/Open%20the%20web%20installer-1784F7?style=for-the-badge&logo=esphome&logoColor=white" alt="Open the web installer" height="40"></a>
</p>

Replace your **Hisense** AC's Wi-Fi module (AEH-W4x / **AEH-W41H1**) with a small M5Stack ESP32
board and control it locally from Home Assistant. No cloud for control; optional anonymous compatibility report. No vendor app. More AC Brands Will be added as The project Grows.

Free for personal and non-commercial use. Forks welcome. Commercial use is not permitted.

**Is my AC supported?** See the [compatibility list](docs/compatibility.md) · [Report my AC](https://github.com/skydoy/ESPHome_AC/issues/new?template=compatibility_report.yml)

## Install in 3 steps
1. **Flash.** Open the **[web installer](https://skydoy.github.io/ESPHome_AC/)** in Chrome or Edge,
   pick your board, plug it into your computer by USB → **Install**.
   (ATOM: flash before fitting the Tail485, it uses the USB-C port.)
2. **Wi-Fi.** Type your Wi-Fi in the same window. (Or later: join the `ESPHome_AC Setup` hotspot from your phone.)
3. **Wire and add.** AC off at the breaker, connect the 4-wire plug of the old Wi-Fi module to the board
   ([wiring](docs/wiring.md)), breaker back on. Home Assistant shows *Air Conditioner* (*Hisense AC* on boards installed before v0.3.0) under
   **Settings → Devices & services** → **Configure**.

Step by step with pictures: **[Quick start](docs/quick-start.md)** · **[Guia rápido (PT)](docs/guia-rapido.md)**

[![Wiring diagram](docs/wiring.svg)](docs/wiring.md)

## What you get in Home Assistant
| Entity | What it does |
|---|---|
| **Air Conditioner** | Off / Cool / Heat / Dry / Fan / Auto, 16–32 °C, 7 fan speeds incl. Quiet, swing, presets Eco, Boost, Sleep and 8 °C frost protection. |
| **Sensors** | Outdoor and coil temperatures, compressor frequency, humidity, voltage, power, current, energy today (when the unit reports them). |
| **AC Link**, **AC Fault** | Whether the link to the unit is up, and its fault codes. |
| **AC Display**, **Eco**, **Super**, **Fan Mute**, **Sleep Mode** | Panel light and the unit's own extra modes. |
| **Firmware Update**, **Auto Update** | New versions in one click, or by themselves at night. |

The device also has a web page at `http://<device-ip>/` with states and live logs (no password, LAN only;
[how to add one or turn it off](docs/configuration.md#web-server)).
The firmware ships without an API key; to add one (recommended), see
[API encryption](docs/configuration.md#api-encryption-and-ota-password-optional).
Adopted in the ESPHome Device Builder? Just press **Install**: it follows `@stable`
([details](docs/configuration.md#esphome-device-builder)).

## Boards
| | **ATOM Lite + ATOM Tail485** | **M5StickC Plus2 + RS485 Hat** |
|---|---|---|
| Approx. price | ≈ US$15 ($7.50 + $6.95) | ≈ US$28 ($19.95 + ≈ $8) |
| Status | RGB LED + button | Screen + two buttons |
| Wiring | [ATOM wiring](docs/wiring.md#atom-lite--tail485) | [Stick wiring](docs/wiring.md#2-connect) |
| Links | [ATOM Lite](https://docs.m5stack.com/en/core/ATOM%20Lite) · [Tail485](https://docs.m5stack.com/en/atom/tail485) | [M5StickC Plus2](https://docs.m5stack.com/en/core/M5StickC%20PLUS2) · [RS485 Hat](https://docs.m5stack.com/en/hat/hat_rs485) |

Prices are M5Stack shop list prices, before shipping and tax. The Plus2 is end-of-life at the M5Stack shop.

> ⚠️ **G0 boot pin (Stick only):** if the Stick boots from USB but not from the AC, add a 10 kΩ pull-up from
> G0 to 3.3 V. [Details](docs/wiring.md#3-boot-pin-g0-read-this-before-closing-the-ac)

> **Status:** only Hisense is supported today (units whose Wi-Fi module sits on the 4-pin RS-485 connector,
> see the [compatibility table](docs/compatibility.md)). The ATOM build is checked automatically but not yet
> tested on a real unit. Tried it? Please [tell us how it went](https://github.com/skydoy/ESPHome_AC/issues/new?template=compatibility_report.yml).
> Other brands (Mitsubishi, Daikin, Midea, …) wait for testers with real hardware; see [CONTRIBUTING.md](CONTRIBUTING.md).

## Automatic updates (OTA)
The board checks for new firmware on its own (at start-up and every 6 hours). Only tested
releases from the `stable` branch are offered.
API key, Device Builder vs factory OTA, and the open LAN web page:
[Security and OTA](docs/security.md).
- **Web installer image:** Home Assistant shows **Firmware Update**. Press **Install** (or leave
  **Auto Update** on for a night install). Wi-Fi and settings are kept.
- **Adopted in the ESPHome Device Builder:** the board only *reports* a new version
  (**Update Status**, **Check for Update**). Press **Install** in the Device Builder; your
  config already follows `stable`, so no YAML edit is needed.

## Changelog
See [CHANGELOG.md](CHANGELOG.md).

## Docs
[Troubleshooting](docs/troubleshooting.md) · [Problemas (PT)](docs/problemas.md) ·
[Wiring](docs/wiring.md) · [Home Assistant](docs/home-assistant.md) ·
[Configuration](docs/configuration.md) · [Compatibility](docs/compatibility.md) ·
[Hisense notes](brands/hisense/README.md) · [Hardware](brands/hisense/hardware.md) ·
[Protocol](brands/hisense/protocol.md) · [Adapters](adapters/README.md) ·
[Changelog](CHANGELOG.md) · [Security and OTA](docs/security.md) · [Report a problem](https://github.com/skydoy/ESPHome_AC/issues/new/choose) ·
[Discussions](https://github.com/skydoy/ESPHome_AC/discussions) · [Security policy](SECURITY.md) · [Third-party notices](THIRD_PARTY_NOTICES.md)

Own build or dashboard import: [Stick](devices/hisense-m5stickc-plus2.yaml) / [ATOM](devices/hisense-atom-lite-tail485.yaml)
device configs, [examples/](examples/my-ac-livingroom.yaml).

## Credits
- Original implementation; no code copied. Protocol knowledge from
  (see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)):
  [AndrewDemsDS/hisense-w41h1](https://github.com/AndrewDemsDS/hisense-w41h1),
  [akrabi/hisense_ac_esphome](https://github.com/akrabi/hisense_ac_esphome),
  [Druidblack/AC-Hisense](https://github.com/Druidblack/AC-Hisense),
  [pio2398/W4G2](https://github.com/pio2398/W4G2),
  [zoffypal/esphome_hisence_ac](https://github.com/zoffypal/esphome_hisence_ac),
  [straga/scrivo_project](https://github.com/straga/scrivo_project),
  [pslawinski/esphome_airconintl](https://github.com/pslawinski/esphome_airconintl) and
  [artshevchenko/AC-Hi](https://github.com/artshevchenko/AC-Hi).
- Firmware, installer and logo: [skydoy](https://github.com/skydoy). Board pictures are illustrations.

[PolyForm Noncommercial 1.0.0](LICENSE): free for personal and non-commercial use, forks
welcome, commercial use not permitted. Use at your own risk: the indoor unit has mains voltage inside.
Hisense is a trademark of its owner; this is an independent community project, not affiliated with it.

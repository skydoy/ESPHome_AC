# Overview

Details that used to live in the root [README](../README.md). Short start: [README](../README.md) · [Quick start](quick-start.md).

## What it is

Replace your **Hisense** AC's Wi-Fi module (AEH-W4x / **AEH-W41H1**) with a small M5Stack ESP32
board and control it locally from Home Assistant. No cloud for control; optional anonymous
compatibility report. No vendor app. More AC brands will be added as the project grows.

Free for personal and non-commercial use. Forks welcome. Commercial use is not permitted.

**Is my AC supported?** See the [compatibility list](compatibility.md) ·
[Report my AC](https://github.com/skydoy/ESPHome_AC/issues/new?template=compatibility_report.yml)

## What you get in Home Assistant

| Entity | What it does |
|---|---|
| **Air Conditioner** | Off / Cool / Heat / Dry / Fan / Auto, 16–32 °C, 7 fan speeds incl. Quiet, swing, presets Eco, Boost, Sleep and 8 °C frost protection. |
| **Sensors** | Outdoor and coil temperatures, compressor frequency, humidity, voltage, power, current, energy today (when the unit reports them). |
| **AC Link**, **AC Fault** | Whether the link to the unit is up, and its fault codes. |
| **AC Display**, **Eco**, **Super**, **Fan Mute**, **Sleep Mode** | Panel light and the unit's own extra modes. |
| **Firmware Update**, **Auto Update** | New versions in one click, or by themselves at night. |

The device also has a web page at `http://<device-ip>/` with states and live logs (no password, LAN only;
[how to add one or turn it off](configuration.md#web-server)).
The firmware ships without an API key; to add one (recommended), see
[API encryption](configuration.md#api-encryption-and-ota-password-optional).
Adopted in the ESPHome Device Builder? Just press **Install**: it follows `@stable`
([details](configuration.md#esphome-device-builder)).

Screenshots of the device web page: [Device web page](web-page.md).

## Boards

| | **ATOM Lite + ATOM Tail485** | **M5StickC Plus2 + RS485 Hat** |
|---|---|---|
| Approx. price | ≈ US$15 ($7.50 + $6.95) | ≈ US$28 ($19.95 + ≈ $8) |
| Status | RGB LED + button | Screen + two buttons |
| Wiring | [ATOM wiring](wiring.md#atom-lite--tail485) | [Stick wiring](wiring.md#2-connect) |
| Links | [ATOM Lite](https://docs.m5stack.com/en/core/ATOM%20Lite) · [Tail485](https://docs.m5stack.com/en/atom/tail485) | [M5StickC Plus2](https://docs.m5stack.com/en/core/M5StickC%20PLUS2) · [RS485 Hat](https://docs.m5stack.com/en/hat/hat_rs485) |

Prices are M5Stack shop list prices, before shipping and tax. The Plus2 is end-of-life at the M5Stack shop.

> ⚠️ **G0 boot pin (Stick only):** if the Stick boots from USB but not from the AC, add a 10 kΩ pull-up from
> G0 to 3.3 V. [Details](wiring.md#3-boot-pin-g0-read-this-before-closing-the-ac)

> **Status:** only Hisense is supported today (units whose Wi-Fi module sits on the 4-pin RS-485 connector,
> see the [compatibility table](compatibility.md)). The ATOM build is checked automatically but not yet
> tested on a real unit. Tried it? Please
> [tell us how it went](https://github.com/skydoy/ESPHome_AC/issues/new?template=compatibility_report.yml).
> Other brands (Mitsubishi, Daikin, Midea, …) wait for testers with real hardware; see
> [CONTRIBUTING.md](../CONTRIBUTING.md).

[![Wiring diagram](wiring.svg)](wiring.md)

## Automatic updates (OTA)

The board checks for new firmware on its own (at start-up and every 6 hours). Only tested
releases from the `stable` branch are offered.
API key, Device Builder vs factory OTA, and the open LAN web page:
[Security and OTA](security.md).

- **Web installer image:** Home Assistant shows **Firmware Update**. Press **Install** (or leave
  **Auto Update** on for a night install). Wi-Fi and settings are kept.
- **Adopted in the ESPHome Device Builder:** the board only *reports* a new version
  (**Update Status**, **Check for Update**). Press **Install** in the Device Builder; your
  config already follows `stable`, so no YAML edit is needed.

## Device web page

Open `http://<device-ip>/` on your LAN for live states, the AC diagram and logs
(no password by design; [security notes](security.md)).
Language: PT/EN toggle (top right). Diagnostic text values follow the same language.

Full screenshots: [Device web page](web-page.md).

## Own build / Device Builder

- Stick: [devices/hisense-m5stickc-plus2.yaml](../devices/hisense-m5stickc-plus2.yaml)
- ATOM: [devices/hisense-atom-lite-tail485.yaml](../devices/hisense-atom-lite-tail485.yaml)
- Example: [examples/my-ac-livingroom.yaml](../examples/my-ac-livingroom.yaml)

## Credits

- Original implementation; no code copied. Protocol knowledge from
  (see [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md)):
  [AndrewDemsDS/hisense-w41h1](https://github.com/AndrewDemsDS/hisense-w41h1),
  [akrabi/hisense_ac_esphome](https://github.com/akrabi/hisense_ac_esphome),
  [Druidblack/AC-Hisense](https://github.com/Druidblack/AC-Hisense),
  [pio2398/W4G2](https://github.com/pio2398/W4G2),
  [zoffypal/esphome_hisence_ac](https://github.com/zoffypal/esphome_hisence_ac),
  [straga/scrivo_project](https://github.com/straga/scrivo_project),
  [pslawinski/esphome_airconintl](https://github.com/pslawinski/esphome_airconintl) and
  [artshevchenko/AC-Hi](https://github.com/artshevchenko/AC-Hi).
- Firmware, installer and logo: [skydoy](https://github.com/skydoy). Board pictures are illustrations.

[PolyForm Noncommercial 1.0.0](../LICENSE): free for personal and non-commercial use, forks
welcome, commercial use not permitted. Use at your own risk: the indoor unit has mains voltage inside.
Hisense is a trademark of its owner; this is an independent community project, not affiliated with it.

## More docs

[Troubleshooting](troubleshooting.md) · [Wiring](wiring.md) · [Home Assistant](home-assistant.md) ·
[Configuration](configuration.md) · [Compatibility](compatibility.md) ·
[Hisense notes](../brands/hisense/README.md) · [Hardware](../brands/hisense/hardware.md) ·
[Protocol](../brands/hisense/protocol.md) · [Adapters](../adapters/README.md) ·
[Changelog](../CHANGELOG.md) · [Security and OTA](security.md) ·
[Report a problem](https://github.com/skydoy/ESPHome_AC/issues/new/choose) ·
[Discussions](https://github.com/skydoy/ESPHome_AC/discussions) ·
[Security policy](../SECURITY.md) · [Third-party notices](../THIRD_PARTY_NOTICES.md)

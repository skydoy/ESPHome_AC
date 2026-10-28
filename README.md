# ESPHome Hisense AC (M5StickC Plus2 + RS485 Hat)

Replaces the Hisense Wi-Fi module (AEH-W4A1 / W4B1 / W4E1 / W4G1 / W4G2 / **AEH-W41H1**, all on
the RS-485 "XM" bus with `F4 F5 … F4 FB` frames, 9600 8N1) with an **M5StickC Plus2 + M5Stack
RS485 Hat** running ESPHome, for local control from Home Assistant. No cloud, no ConnectLife app.

- **Install from the browser:** https://skydoy.github.io/ESPHome_HisenceAC/ (ESP Web Tools,
  Wi-Fi set up in the same wizard via Improv)
- **Adopt in the ESPHome Device Builder:** the device announces `dashboard_import`.
- **Updates:** the `Firmware Update` entity installs new releases over the air (OTA via
  `http_request` from the manifest on GitHub Pages).

> ⚠️ Switch the AC off at the breaker before opening the indoor unit. Use at your own risk.

## Features
The `hisense_ac` component (in [`components/hisense_ac`](components/hisense_ac)) exposes:

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

### Link bring-up and the AEH-W41H1
The stock W41H1 module performs a bring-up sequence (device type, version, heartbeat) before
polling status, and its indoor units send long 160-byte status frames. This component does the
same (`handshake: true`, default) and decodes 82-, 150- and 160-byte status frames. Set
`handshake: false` for plain polling like the older community components. Details in
[docs/protocol.md](docs/protocol.md).

### Configuration
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
    # Every sub-entity is optional, see packages/hisense-ac.yaml for the full list.
```

## Status screen and buttons
The Plus2 screen shows Wi-Fi state and IP address, the AC link state, mode and set point,
indoor/outdoor temperature and a fault hint. It turns off 30 s after boot or after the last
press of **button A**; it stays on while Wi-Fi or the AC link is down so problems are visible
during installation. Button B is exposed to Home Assistant for your own automations.

## Wiring
See [docs/hardware.md](docs/hardware.md), including the **G0 boot pin caveat** of the RS485 hat.

## Repository layout
| Path | What |
|---|---|
| `hisense-ac-m5stickc-plus2.yaml` | Factory config built by CI and offered by the web installer |
| `packages/` | Board, screen, AC, provisioning and auto-update packages |
| `components/hisense_ac/` | The ESPHome external component (ESP-IDF; Arduino untested) |
| `tests/` | Host unit tests for the protocol layer (`make -C tests`) |
| `static/index.html` | ESP Web Tools installer page |
| `examples/` | Config for building in your own ESPHome Device Builder |

## Releasing
Push a tag `vX.Y.Z`: GitHub Actions builds the firmware with ESPHome 2026.10.1, then publishes
the installer page and `firmware/manifest.json` to GitHub Pages (Settings → Pages → Source:
**GitHub Actions**). Devices pick up the new version through the `Firmware Update` entity.

## Ideas (not implemented)
- **IR on G19:** send iFeel room temperature (iFeel is IR-only, Kelon 168-bit protocol) or
  control units without a working bus.
- **Button B:** toggle the AC or its display LED locally.
- **Second screen page:** fault codes, power and today's energy.
- **QR code** on screen pointing to the device's web page or the setup hotspot.

## Credits and provenance
The component is an original implementation (MIT). No source code was copied from the projects
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

## License
MIT, see [LICENSE](LICENSE). Hisense and ConnectLife are trademarks of their owners; this project
is not affiliated with them.

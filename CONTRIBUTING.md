# Contributing

## Repository layout
```
components/<component>/   ESPHome external components (ESPHome requires this folder name)
core/                     Brand-agnostic packages
  boards/<board>.yaml     ESP board: chip, power hold, buttons, pins of the adapter header
  screen/<board>.yaml     Status screen; reads only the generic ids listed below
  provisioning.yaml       Improv serial/BLE, fallback hotspot, API, OTA
  diagnostics.yaml        Uptime, Wi-Fi signal, version, restart buttons, time source
  auto-update.yaml        OTA from the GitHub Pages manifest
brands/<brand>/           Brand package (<brand>-ac.yaml) + wiring and protocol docs
adapters/README.md        Physical-layer adapters table
devices/<brand>-<board>.yaml  Device configs: core + board + brand, built by CI
tests/                    Host unit tests (make -C tests)
static/index.html         ESP Web Tools installer page
```

## Adding a brand
1. **Open an issue first** with the model, the Wi-Fi/service port, and photos of the
   connector and labels.
2. Add `brands/<brand>/<brand>-ac.yaml`. It must:
   - use the board's `ac_tx_pin` / `ac_rx_pin` substitutions for its UART;
   - provide the generic ids `ac_climate`, `ac_link`, `ac_link_status`, `ac_fault`,
     `ac_fault_codes` and `ac_outdoor_temperature` (template entities are fine) so the core
     screen works;
   - prefer an existing upstream ESPHome component referenced by URL. Do not copy code from
     projects without a licence, and keep licences compatible (this repo is MIT; GPL code
     cannot be added to it).
3. Add `brands/<brand>/README.md` and `hardware.md` (adapter, pin-out, voltages, sources).
4. Add `devices/<brand>-<board>.yaml`, add it to `.github/workflows/validate.yml`, and to the
   publish workflow and `static/index.html` once it is tested.
5. Update the row in `adapters/README.md`.

## Testing required before a brand is marked "supported"
- `esphome config` and `esphome compile` pass for the device config (CI does this).
- Protocol code has host unit tests in `tests/` where practical.
- At least one person has run it on a real unit for a week and reported: model numbers,
  every mode, fan speeds, swing, set point, power off/on, recovery after a mains power cut,
  and that the AC's own remote still works.

## Safety
Read the [safety notes](docs/wiring.md) before opening any AC. Never connect an ESP32
directly to an AC port: always use the adapter for that bus, and measure voltages first.

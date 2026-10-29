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
     projects without a licence, and keep licences compatible (this repo is PolyForm Noncommercial 1.0.0; only code
     under permissive licences such as MIT, BSD or Apache-2.0 can be added, keeping its notice;
     GPL code cannot be added to it). Contributions are accepted under the same licence.
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

## Pull requests and releases
- Use the [issue forms](https://github.com/skydoy/ESPHome_AC/issues/new/choose); model reports go
  in a **Compatibility report** and end up in [docs/compatibility.md](docs/compatibility.md).
- Add a line under **Unreleased** in [CHANGELOG.md](CHANGELOG.md).
- Releases: move the Unreleased entries to a new `## [x.y.z]` section, bump `project_version`
  in both base device configs (`devices/<board>.yaml`) and the tag in
  `examples/my-ac-livingroom.yaml`, then push a `vx.y.z` tag that matches `project_version`
  (CI fails otherwise: adopted configs fetch the component at `v<project_version>`). CI builds the firmware, publishes the
  installer and OTA manifest, and creates the GitHub Release with those notes and the images.
- Be kind: see the [Code of Conduct](CODE_OF_CONDUCT.md). Security problems go through
  [SECURITY.md](SECURITY.md), not public issues.

## Safety
Read the [safety notes](docs/wiring.md) before opening any AC. Never connect an ESP32
directly to an AC port: always use the adapter for that bus, and measure voltages first.

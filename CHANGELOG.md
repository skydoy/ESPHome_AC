# Changelog

All notable changes to this project are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/spec/v2.0.0.html). Each `v*` tag publishes a
[GitHub Release](https://github.com/skydoy/ESPHome_AC/releases) whose notes are the matching
section below, with the factory and OTA images attached.

## [Unreleased]

## [0.2.11] - 2026-10-29
### Added
- **Update check for adopted / Device Builder builds** (`core/update-check.yaml`, in both base
  configs): `Update Available` (diagnostic, update class) and `Latest Version` (diagnostic),
  read from the board's GitHub Pages manifest at boot and every 6 h, plus a `Check for Update`
  button (configuration) to check now. Report only: nothing is installed, so the adopted config
  and API key stay intact. The factory firmware keeps its `Firmware Update` entity instead.

### Changed
- `update_manifest_url` now lives in the base device files (one place per board).

## [0.2.10] - 2026-10-29
### Fixed
- **Device Builder rebuilt an old version, or failed with 28 errors after an update.** Adopted
  configs referenced `@main`, and ESPHome caches git sources for one day (the one-line
  `github://` form cannot change that), so *Install* could rebuild the old release, or combine
  new YAML with an old cached component (`debug_frames is an invalid option`).
  Adoption (`dashboard_import`) and the `hisense_ac` component are now pinned to the release tag
  of the firmware (`@v<project_version>`): packages and component always come from the same
  immutable tag, so a stale or mismatched cache is impossible. To update an adopted device,
  change the tag in its `packages:` line and press *Install*
  ([details](docs/configuration.md#esphome-device-builder)).

### Changed
- `hisense_component_source` defaults to `github://skydoy/ESPHome_AC@v${project_version}`; new
  `hisense_component_refresh` substitution (default `1d`) for people tracking a branch.
- Firmware Version shows `(self-built)` instead of `(main)` for Device Builder and local builds.
- CI: the tag must match `project_version`; both boards must share one version; shipped configs
  must not reference `@main`; after each release the adopted YAML is validated from GitHub.

## [0.2.9] - 2026-10-29
### Fixed
- **Adopted devices were replaced by the generic firmware.** The `Firmware Update` entity was
  part of the device config that the ESPHome Device Builder adopts, so pressing *Install* on it in
  Home Assistant flashed the generic web-installer firmware over an adopted config. That firmware
  has no API encryption key, so Home Assistant (still using the old key) could not reconnect and
  the update looked like it had done nothing. Each board now has a **base** config
  (`devices/<board>.yaml`, the one that is adopted, no update entity) and a **factory** config
  (`devices/<board>.factory.yaml` = base + `core/auto-update.yaml`, built by CI for the web
  installer). Adopted devices update with *Install* in the Device Builder
  ([details](docs/configuration.md#updates)).

### Added
- OTA feedback: the Stick's screen turns on and shows a progress bar during any OTA update
  (Device Builder or `Firmware Update`), and *Update failed* if it fails. Clear log lines for
  the start, end and errors of each update.
- CI checks that only the factory configs contain the update entity.
- Unit tests for the command queue (coalescing, special-command spacing, `millis()` wrap).

### Changed
- Faster, smoother control:
  - a command from Home Assistant starts a bus cycle at once when the bus is idle, and a command
    that arrives mid-cycle starts the next cycle right away (was: up to 1 s wait);
  - quick repeated changes (set point clicks, fan steps) are merged while they wait, so the AC
    gets one frame with the final value; swing axes and special modes keep their own frames;
  - the Eco / Turbo / Quiet switches and the Sleep select follow a climate change immediately
    instead of 3 s later.
- No blocking in the main loop: UART writes no longer wait for the transmission to finish
  (up to ~50 ms per command at 9600 baud); the reply timeout starts after the last byte instead.
  UART reads are done in chunks instead of byte by byte.
- Fewer updates: fault binary sensors and the fault text are only published when the fault bytes
  change (the fault text string is no longer rebuilt on every status frame).
- Status screen: drawn only when something it shows changes (checked every 0.5 s) instead of a
  full redraw every 2 s, and never while the backlight is off.
- After an AC link outage, capabilities are re-queried if they never arrived and every entity is
  refreshed with the first status frame.

## [0.2.8] - 2026-10-29
### Added
- **Web server** on both boards (`core/web-server.yaml`): open `http://<device-ip>/` on the LAN
  for entity states, controls and the live log (ESPHome web server v3, port 80, JS/CSS from the
  ESPHome CDN). It has **no authentication**; add `auth:` or remove it with
  `web_server: !remove` in your own config ([details](docs/configuration.md#web-server)).

## [0.2.7] - 2026-10-29
### Added
- Second board: **M5Stack ATOM Lite + ATOM Tail485**, the cheapest option (≈ US$15). No screen:
  the RGB LED shows blue (Wi-Fi setup), red (no AC link) or green (link OK, turns off after
  30 s) and the button shows the status or, held for 10 s, does a factory reset. Optional
  **Status LED** config switch. Pins from the M5Stack docs: TX = G26, RX = G32.
  ([wiring](docs/wiring.md#atom-lite--tail485), [LED and button](docs/configuration.md#status-led-and-button-atom-lite))
- Web installer: board selector with the matching wiring diagram; each board has its own
  firmware and OTA manifest (`firmware/atom-lite-tail485/manifest.json` for the ATOM). The
  M5StickC Plus2 keeps `firmware/manifest.json`, so existing devices update as before.
- README hardware table with approximate prices.

### Changed
- The board package now sets `ac_tx_ignore_strapping` (only the Stick's G0 TX is a
  strapping pin). Own configs that use the brand package with a custom board must set it.

## [0.2.6] - 2026-10-29
### Added
- ConnectLife-style controls: **Eco**, **Super** (turbo) and **Fan Mute** (quiet) switches, a
  **Sleep Mode** select (Off, Sleep 1–4) and **Light On** / **Light Off** buttons. They stay in
  sync with the climate presets and fan mode.
- One diagnostic problem binary sensor per fault bit (19 in total, e.g. indoor coil sensor,
  outdoor EEPROM, exhaust sensor, WiFi/indoor communication). The fault codes text sensor stays.
- **Firmware Version** diagnostic text sensor: the project version plus the build source, e.g.
  `0.2.6 (release)` or `0.2.6 (main)` for Device Builder builds of the adopted package
  (`firmware_source` substitution, set by CI).
- Docs: how to add an optional API encryption key and OTA password
  ([configuration](docs/configuration.md#api-encryption-and-ota-password-optional)). The
  firmware still ships without them, so existing devices keep working.

### Fixed
- Device Builder compile warnings:
  - the splash image uses the `platform: file` image syntax;
  - the splash PNG is loaded by URL, so remote and bundled builds no longer skip it
    ("skipping file outside config directory");
  - the fixed strapping pins (G0 UART TX, LCD GPIO5/12/15) set `ignore_strapping_warning`.

## [0.2.5] - 2026-10-29
### Added
- Raw frame logging: `debug_frames` / `debug_frames_interval` options and an
  **AC Frame Logging** switch to toggle it at runtime. Logs TX commands and rate-limited RX status
  frames as hex at DEBUG level, plus the bytes still being calibrated.
  See [configuration](docs/configuration.md#raw-frame-logging).

### Changed
- AEH-W41H1 marked ✅ working (maintainer-tested) in the
  [compatibility table](docs/compatibility.md), with what is confirmed and what still needs
  calibration (power steps, indoor coil, voltage/current/humidity).
- Humidity, supply voltage and current are no longer published as NaN when the AC never reports
  them: they stay "unknown" until a real value arrives, and are disabled by default in the
  bundled config. Humidity also follows the AC's capability reply when it has one.

## [0.2.4] - 2026-10-28
### Added
- [Compatibility table](docs/compatibility.md) of AC models and Wi-Fi modules.
- Issue forms (bug, feature, compatibility report), pull request template, code owners.
- `SECURITY.md` with private vulnerability reporting, `CODE_OF_CONDUCT.md` (Contributor Covenant 2.1),
  `.editorconfig` and Dependabot for GitHub Actions.
- Releases are created automatically from tags, with notes from this changelog and the
  `factory.bin` / `ota.bin` images attached.
- Social preview image (`docs/social-preview.png`).

### Changed
- README: installer button, links to the compatibility table, changelog and issue forms.

## [0.2.3] - 2026-10-28
### Changed
- Vector logo with a transparent background; dark and light variants in the README.
- Sharper favicon, installer icons and boot splash.
- GitHub Releases are now published for every version.

## [0.2.2] - 2026-10-28
### Added
- Project logo and favicon on the web installer.
- 2.5 s boot splash on the Stick screen showing the firmware version.

## [0.2.1] - 2026-10-28
### Changed
- License changed to [PolyForm Noncommercial 1.0.0](LICENSE): free for personal and
  non-commercial use, forks welcome, commercial use not permitted.

## [0.2.0] - 2026-10-28
### Added
- Status screen with link indicator, fault code, backlight timeout and wake on button press
  or state change.
- Boot pin (G0) and brownout hints; wiring diagram for 5 V and 12 V connectors.
- Reset reason diagnostic.
- New README, [configuration](docs/configuration.md) and [wiring](docs/wiring.md) guides.

### Changed
- Repository renamed to `skydoy/ESPHome_AC` (project name `skydoy.esphome-ac`) with a new OTA
  manifest URL. **Devices on 0.1.0 must be flashed once from the
  [web installer](https://skydoy.github.io/ESPHome_AC/)**; later updates arrive over the air.

## [0.1.0] - 2026-10-28
### Added
- `hisense_ac` ESPHome component for the Hisense RS-485 "XM" bus, including the AEH-W41H1
  link handshake.
- M5StickC Plus2 + RS485 Hat device configuration.
- Modular layout: `core/`, `core/boards/`, `brands/`, `devices/`.
- ESP Web Tools installer and OTA manifest on GitHub Pages.

[Unreleased]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.11...HEAD
[0.2.11]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.10...v0.2.11
[0.2.10]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.9...v0.2.10
[0.2.9]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.8...v0.2.9
[0.2.8]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.7...v0.2.8
[0.2.7]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.6...v0.2.7
[0.2.6]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.5...v0.2.6
[0.2.5]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.4...v0.2.5
[0.2.4]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.3...v0.2.4
[0.2.3]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.2...v0.2.3
[0.2.2]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.1...v0.2.2
[0.2.1]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/skydoy/ESPHome_AC/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/skydoy/ESPHome_AC/releases/tag/v0.1.0

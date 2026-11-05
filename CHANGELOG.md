# Changelog

All notable changes to this project are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/spec/v2.0.0.html). Each `v*` tag publishes a
[GitHub Release](https://github.com/skydoy/ESPHome_AC/releases) whose notes are the matching
section below, with the factory and OTA images attached.

## [Unreleased]

## [0.3.5] - 2026-11-05

### Fixed
- **Report my AC** text sensor now shows a short Worker redirect
  (`…/r/<8 hex>`, ~66 chars) instead of the huge prefilled GitHub URL that broke
  web_server v3 layout. Full GitHub URL remains in the INFO log only.

### Changed
- Compatibility Worker: `GET /r/<code>` and `GET /r?i=&f=&b=` **302** to the GitHub issue
  form (no issue created). Short codes are stored in KV during `POST /report`.

## [0.3.4] - 2026-11-05

### Added
- Factory web UI / HA: **Check for Update** (`mdi:cloud-search`) and **Install Update**
  (`mdi:download`) config buttons next to **Firmware Update** (check via `component.update`,
  install via `update.perform` with `force_update: false`). The update entity stays visible on
  web_server v3.

## [0.3.3] - 2026-11-05

### Added
- **AC Identity** (read-only): at boot (and via **Identify AC**), the board reads device type,
  AC firmware version, capabilities and one status frame, then shows **AC Identity** and a
  prefilled **Report my AC** GitHub link.
- **Auto report** (config switch, default ON): after identify, when Wi-Fi is up, POSTs once to a
  Cloudflare Worker (`worker/`) with identity, firmware, board and `sha256(MAC)` as `device_id`
  (never the raw MAC; IPs not stored). Only resent when identity changes. Turn the switch off to
  disable. See `docs/compatibility.md` / `docs/compatibilidade.md`.

### Changed
- README wording: no cloud **for control**; optional anonymous compatibility report.

## [0.3.2] - 2026-11-04
### Changed
- **Portuguese texts now use informal "tu"** (pt-PT) everywhere: web installer, `README.pt.md`,
  `docs/guia-rapido.md`, `docs/problemas.md` and `docs/compatibilidade.md` ("A tua placa",
  "Liga por USB, carrega em Instalar"), matching the other ESPHome projects by skydoy.

### Notes
- No firmware changes besides the version number.

## [0.3.1] - 2026-11-02
### Added
- **M5StickC Plus2 boot animation** now shows the original project logo (`boot_logo.png`, from
  `docs/icon.png`) instead of the old badge.
- **Donation links.** "Like it? Help me build something better." with Ko-fi
  (https://ko-fi.com/skydoy) and Buy Me a Coffee (https://buymeacoffee.com/skydoy) buttons at the
  top of both READMEs and in the web installer header (PT/EN, light and dark themes), plus
  `.github/FUNDING.yml` so GitHub shows the Sponsor button.

### Notes
- No functional changes to the AC control. Device names stay as in 0.3.0 (new installs
  `esphome-ac`, existing devices keep `hisense-ac`).

## [0.3.0] - 2026-11-02
### Changed
- **New installs get brand-neutral names.** Boards flashed from the web installer are now called
  `esphome-ac-xxxxxx` (M5StickC Plus2) or `esphome-ac-atom-xxxxxx` (ATOM), with the friendly
  name **Air Conditioner**, so their Home Assistant entities are
  `climate.air_conditioner_xxxxxx_air_conditioner` and so on. Release files follow
  (`esphome-ac-esp32-*.bin`, `esphome-ac-atom-esp32-*.bin`).

**Existing devices keep their names and entity IDs.**
- Boards flashed before 0.3.0 keep updating from the same `firmware/...` manifests, which keep
  serving builds named `hisense-ac` / **Hisense AC** (`hisense-ac-*-v0.3.0.bin` on the release).
  An update never renames a device.
- Configs adopted in the ESPHome Device Builder set `name` and `friendly_name` themselves, and the
  base configs still default to `hisense-ac` / `Hisense AC`, so pressing **Install** changes nothing.

### Added
- `devices/esphome-ac-m5stickc-plus2.factory.yaml` and `devices/esphome-ac-atom-lite-tail485.factory.yaml`
  (new-install builds, OTA from `install/<board>/manifest.json` on GitHub Pages).
- CI builds both the legacy and the new-install images and checks that the legacy manifests
  still point to hisense-ac builds.

### Docs
- Installer, READMEs, quick start guides and the Home Assistant card name both the new
  (*Air Conditioner*) and the old (*Hisense AC*) names.

## [0.2.18] - 2026-11-02
### Changed
- The Wi-Fi setup hotspot is now called **`ESPHome_AC Setup`** (was `Hisense-AC Setup`; the beta
  setup firmware used `ESPHome-AC Setup`). The M5StickC Plus2 screen, the installer page and the
  guides say the same. Device names, entity IDs and Home Assistant setups are unchanged.
- The beta setup page title now reads "ESPHome_AC setup".
- Product naming is brand-neutral: the installer page title, header and description, and the
  README taglines say **ESPHome_AC** / "your air conditioner" (Hisense is named as the brand
  supported today). The beta setup screen header reads "ESPHome_AC Setup".

Note: a board that already saved your Wi-Fi keeps working. Only the name of the fallback hotspot
changes, after the update.

## [0.2.17] - 2026-11-02
### Added
- Light / dark mode button on the web installer (follows the OS by default, remembered).
- **Setup firmware (beta, not on the installer yet):** `devices/setup-m5stickc-plus2.yaml` and
  `devices/setup-atom-lite-tail485.yaml`, a brand-neutral image that detects the AC and then
  installs the matching brand firmware over OTA. New `ac_setup` component: `/setup` page, REST
  API and OTA install, plus a read-only probe engine (strict allow-list, rate limited) with host
  unit tests.
- CI builds the setup images and checks their size, the symbol allow-list (no brand command
  code linked) and that they have no static Wi-Fi networks.

### Changed
- The read-only Hisense protocol code moved into its own `hisense_proto` component, and the
  command builders into `hisense_ac/hisense_command.*`. `hisense_ac` loads `hisense_proto`
  automatically, so existing configs need no change.

The regular Hisense firmware behaves exactly like 0.2.16.

## [0.2.16] - 2026-11-02
### Added
- **Is my AC supported?** card on the web installer (PT/EN) with links to the compatibility list
  and a **Report my AC** button.
- [Compatibilidade](docs/compatibilidade.md): the compatibility list in Portuguese.
- Compatibility list: "Board tested" and "Reported by" columns, ❔ Untested status, ATOM row.
- Compatibility report form: board dropdown and a log ("RX" lines) field.

No firmware changes: 0.2.16 behaves exactly like 0.2.15.

## [0.2.15] - 2026-11-02
### Changed
- **New web installer page**: one card per step (board → install → Wi-Fi → wiring → Home
  Assistant) with a "You should see" check after each, a board picker with pictures of both
  boards, per-board wiring diagrams and tables, a troubleshooting card and a **Português /
  English** toggle (remembered in the browser). Everything from 0.2.14 is kept: USB driver help,
  the 5 V / 12 V check, photo slots, help-us-test link and the auto-update notes.
- **README** rewritten to be shorter: install in 3 steps, what you get in Home Assistant, boards
  and status at a glance.

### Added
- [README.pt.md](README.pt.md): the README in Portuguese.
- [Quick start](docs/quick-start.md) and [Guia rápido](docs/guia-rapido.md): illustrated
  step-by-step guides in English and Portuguese.
- [Problemas](docs/problemas.md): the troubleshooting guide in Portuguese.
- Board illustrations (`static/board-atom.svg`, `static/board-stick.svg`). They are drawings, not
  photos; real installation photos still go in the slots listed in
  [docs/photos/README.md](docs/photos/README.md).

No firmware changes: 0.2.15 behaves exactly like 0.2.14.

## [0.2.14] - 2026-11-02
### Added
- **Auto Update** (web-installer firmware only): a configuration switch, on by default and kept
  across reboots. Once a night at 03:MM local time (random minute) the device checks GitHub and
  installs a newer release through the existing **Firmware Update** entity. Never in the first
  10 minutes after boot; every decision is logged under `auto_update`. Adopted builds are
  unchanged and keep **Update Available**.
- [Troubleshooting & FAQ](docs/troubleshooting.md): no AC link, Stick boot pin, not found in Home
  Assistant, updates and the Device Builder, web page, Wi-Fi reset and re-flash, logs.
- [Home Assistant](docs/home-assistant.md): a dashboard card (built-in cards only) and an
  **update notification blueprint**
  ([`blueprints/automation/esphome_ac_update_available.yaml`](blueprints/automation/esphome_ac_update_available.yaml))
  with a one-click import button.

### Changed
- **Web installer, simpler:** a "You should see" line for each step (screen or LED state per
  board), USB driver help on the install step (CH9102 for the M5StickC Plus2, FTDI for the ATOM
  Lite, charge-only cables), the six "coming soon" brands replaced by one "Other brands – help us
  test" link, and the 5 V / 12 V multimeter check moved into an optional explainer. Slots for
  real installation photos appear as photos are added ([list](docs/photos/README.md)).

## [0.2.13] - 2026-10-29
### Added
- **AC Firmware Version** (diagnostic): the AC's reply to the `07` version request at boot.
  The reply layout is not confirmed yet, so it is decoded best effort (text if printable,
  otherwise the protocol word from bytes 18/19 plus the raw bytes) and the raw frame is logged.
- **AC Bus Checksum Errors** / **AC Bus Framing Errors** (diagnostic, total increasing): RS-485
  frame error counters since boot, published only when they change (checked every 10 s).
- Hidden by default (diagnostic, enable in Home Assistant if needed): **Humidity Set Point**
  (status byte 22), **AC Panel Unit** (Celsius/Fahrenheit), **AC Capabilities** (ProductType
  reply as a list) and **AC Device Type** (`0A` type pair).

## [0.2.12] - 2026-10-29
### Changed
- **Adopted devices update with just *Install*, no YAML edits.** Adoption (`dashboard_import`)
  now imports `@stable`, a branch CI moves to each release tag after the release is published.
  The `hisense_ac` component stays pinned to `v<project_version>` inside the package, so YAML
  and component always come from the same release. The Device Builder may take up to 1 day to
  see a new release (git cache); *Clean Build Files* forces it. Existing `@vX.Y.Z` lines keep
  working; switch them to `@stable` once.

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

[Unreleased]: https://github.com/skydoy/ESPHome_AC/compare/v0.3.5...HEAD
[0.3.5]: https://github.com/skydoy/ESPHome_AC/compare/v0.3.4...v0.3.5
[0.3.4]: https://github.com/skydoy/ESPHome_AC/compare/v0.3.3...v0.3.4
[0.3.3]: https://github.com/skydoy/ESPHome_AC/compare/v0.3.2...v0.3.3
[0.3.2]: https://github.com/skydoy/ESPHome_AC/compare/v0.3.1...v0.3.2
[0.3.1]: https://github.com/skydoy/ESPHome_AC/compare/v0.3.0...v0.3.1
[0.3.0]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.18...v0.3.0
[0.2.18]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.17...v0.2.18
[0.2.17]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.16...v0.2.17
[0.2.16]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.15...v0.2.16
[0.2.15]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.14...v0.2.15
[0.2.14]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.13...v0.2.14
[0.2.13]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.12...v0.2.13
[0.2.12]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.11...v0.2.12
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

# Changelog

All notable changes to this project are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/spec/v2.0.0.html). Each `v*` tag publishes a
[GitHub Release](https://github.com/skydoy/ESPHome_AC/releases) whose notes are the matching
section below, with the factory and OTA images attached.

## [Unreleased]

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

[Unreleased]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.4...HEAD
[0.2.4]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.3...v0.2.4
[0.2.3]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.2...v0.2.3
[0.2.2]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.1...v0.2.2
[0.2.1]: https://github.com/skydoy/ESPHome_AC/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/skydoy/ESPHome_AC/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/skydoy/ESPHome_AC/releases/tag/v0.1.0

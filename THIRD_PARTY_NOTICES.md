# Third-party notices

This repository's firmware, configs, installer and docs are under
[PolyForm Noncommercial 1.0.0](LICENSE) (Copyright (c) 2026 Tiago Febra / skydoy).

The Hisense RS-485 component is an **original implementation**. No source code was copied from
the projects below; they were used as protocol documentation and real-world behaviour notes.
Full detail: [docs/configuration.md → Credits and provenance](docs/configuration.md#credits-and-provenance)
and [brands/hisense/protocol.md → Sources](brands/hisense/protocol.md#sources).

## Protocol references (MIT)
- [AndrewDemsDS/hisense-w41h1](https://github.com/AndrewDemsDS/hisense-w41h1) (MIT):
  W41H1 link bring-up, frame templates, 160-byte status map, power calibration.
- [akrabi/hisense_ac_esphome](https://github.com/akrabi/hisense_ac_esphome) (MIT per its README):
  protocol documentation and command layout.

Their MIT licence applies to *their* repositories. Because this project did not copy their
source, the MIT text is not reproduced here; see each upstream repo for the full notice.

## Protocol facts only (no code copied)
Used as documentation of status bytes / behaviour only:

- [Druidblack/AC-Hisense](https://github.com/Druidblack/AC-Hisense) (150-byte frames, power)
- [pio2398/W4G2](https://github.com/pio2398/W4G2) (GPL-3.0; status byte notes — their code is
  **not** included here)
- [straga/scrivo_project](https://github.com/straga/scrivo_project)
- vins.vins (4pda forum posts)
- [zoffypal/esphome_hisence_ac](https://github.com/zoffypal/esphome_hisence_ac)
- [pslawinski/esphome_airconintl](https://github.com/pslawinski/esphome_airconintl)
- [artshevchenko/AC-Hi](https://github.com/artshevchenko/AC-Hi)

## Not included
Adapter ideas for other AC brands listed in [adapters/README.md](adapters/README.md) point at
upstream projects for future testers. No code from those projects is in this repository.

Hisense is a trademark of its owner. This is an independent community project, not affiliated
with it.

# Open questions

These are gaps already written in the repository. This page does not fill them in.

| Topic | What the repository says | Where |
|---|---|---|
| ATOM Lite + Tail485 on a real unit | Checked automatically, not hardware-tested. | [Compatibility](compatibility.md) |
| Power on the maintainer's W41H1 | Decoded as `4.15 × byte 55²`. Coarse at low load. Not compared with a meter. | [Compatibility](compatibility.md#aeh-w41h1-maintainers-unit) |
| Indoor coil temperature, byte 21, on that W41H1 | Stayed at room temperature during a short Cool run, so the byte may mean something else on that unit. | Same section, and [protocol.md](https://github.com/skydoy/ESPHome_AC/blob/main/brands/hisense/protocol.md) |
| Supply voltage, current, humidity on that unit | Not reported so far. Hidden until a real value arrives. | [Compatibility](compatibility.md#aeh-w41h1-maintainers-unit) |
| Current | Estimated as `P / (V × 0.95)`, not described as a direct measurement. | [protocol.md](https://github.com/skydoy/ESPHome_AC/blob/main/brands/hisense/protocol.md) |
| AC Firmware Version (`07` reply) | No reply captured yet. Layout unconfirmed. Best-effort text. Raw frame logged at DEBUG. | [protocol.md](https://github.com/skydoy/ESPHome_AC/blob/main/brands/hisense/protocol.md) |
| Unknown status-frame bytes | Identify logs payload offsets that are not decoded. | [protocol.md](https://github.com/skydoy/ESPHome_AC/blob/main/brands/hisense/protocol.md) |
| iFeel | Only on infrared (Kelon 168-bit). Not implemented on this bus. | [protocol.md](https://github.com/skydoy/ESPHome_AC/blob/main/brands/hisense/protocol.md) |
| Capacity (BTU/h) and "electricity reporting mode" | Not on the RS-485 bus. Called out as cloud-side compared with ConnectLife. | [Configuration](configuration.md#entities) |
| Plug pin order | Varies by model. A hardware note gives a typical 5 V order and says to check it. Measure. | [Wiring](wiring.md#1-measure-the-plug) |
| Other brands | Not supported. Waiting for testers with real hardware. | [CONTRIBUTING.md](https://github.com/skydoy/ESPHome_AC/blob/main/CONTRIBUTING.md) |
| Arduino framework | The component notes ESP-IDF. Arduino is untested. | [Configuration](configuration.md#repository-layout) |
| Photos inside a unit | Slots are named. No photo files are in the repository. | [docs/photos/README.md](https://github.com/skydoy/ESPHome_AC/blob/main/docs/photos/README.md) |
| Models only reported with other firmware | Listed as candidates, not confirmed here. | [Compatibility](compatibility.md#reported-working-with-other-firmware-untested-here) |

## Security policy version table

[SECURITY.md](https://github.com/skydoy/ESPHome_AC/blob/main/SECURITY.md) says only the latest release is supported, and its table says latest `0.2.x`. [CHANGELOG.md](https://github.com/skydoy/ESPHome_AC/blob/main/CHANGELOG.md) has a released section `0.3.9` dated 2026-11-09. This preview does not decide which versions are supported and does not edit the policy.

# Open questions

These are gaps already written in the repository. This page does not fill them in.

| Topic | What the repository says | Where |
|---|---|---|
| ATOM Lite + Tail485 on a real unit | Checked automatically, not hardware-tested. | [Compatibility](compatibility.md) |
| Indoor coil temperature, byte 21, on that W41H1 | Stayed at room temperature during a short Cool run, so the byte may mean something else on that unit. | Same section, and [protocol.md](https://github.com/skydoy/ESPHome_AC/blob/main/brands/hisense/protocol.md) |
| Supply voltage and humidity on that unit | Not reported so far. Hidden until a real value arrives. | [Compatibility](compatibility.md#aeh-w41h1-maintainers-unit) |
| AC Firmware Version (`07` reply) | No reply captured yet. Layout unconfirmed. Best-effort text. Raw frame logged at DEBUG. | [protocol.md](https://github.com/skydoy/ESPHome_AC/blob/main/brands/hisense/protocol.md) |
| Unknown status-frame bytes | Identify logs payload offsets that are not decoded. | [protocol.md](https://github.com/skydoy/ESPHome_AC/blob/main/brands/hisense/protocol.md) |
| iFeel | Only on infrared (Kelon 168-bit). Not implemented on this bus. | [protocol.md](https://github.com/skydoy/ESPHome_AC/blob/main/brands/hisense/protocol.md) |
| **Cooling output (BTU/h)** | Pending. Heating output is bytes 55–56 of a long status frame (150 or 160 bytes), big-endian BTU/h, published in Heat mode. Those bytes are heating only. Next step: run the AC in Cool with **Byte Hunt** on and the device's web page open for a few hours, download the CSV and attach it to an issue. | [Home Assistant](home-assistant.md#heating-output) |
| Nameplate rated capacity (BTU/h) and "electricity reporting mode" | Not found on the RS-485 bus. The running heating output is decoded; the nameplate rating and the cloud setting are not. | [Configuration](configuration.md#entities) |
| Plug pin order | Varies by model. A hardware note gives a typical 5 V order and says to check it. Measure. | [Wiring](wiring.md#1-measure-the-plug) |
| Other brands | Not supported. Waiting for testers with real hardware. | [CONTRIBUTING.md](https://github.com/skydoy/ESPHome_AC/blob/main/CONTRIBUTING.md) |
| Arduino framework | The component notes ESP-IDF. Arduino is untested. | [Configuration](configuration.md#repository-layout) |
| Photos inside a unit | Slots are named. No photo files are in the repository. | [docs/photos/README.md](https://github.com/skydoy/ESPHome_AC/blob/main/docs/photos/README.md) |
| Models only reported with other firmware | Listed as candidates, not confirmed here. | [Compatibility](compatibility.md#reported-working-with-other-firmware-untested-here) |


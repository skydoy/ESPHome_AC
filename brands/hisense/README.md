# Brand: Hisense (supported)

Hisense and Hisense-OEM split air conditioners whose indoor unit has a port for an
AEH-W4x Wi-Fi module talk the "XM" protocol (`F4 F5 … F4 FB` frames) over **RS-485, 9600 8N1**.

| Item | Value |
|---|---|
| Status | Supported (component builds, protocol unit tested; field reports welcome) |
| Package | [`hisense-ac.yaml`](hisense-ac.yaml) |
| Component | [`components/hisense_ac`](../../components/hisense_ac) (MIT) |
| Adapter | RS-485 transceiver, e.g. M5Stack Hat RS485 on an M5StickC Plus2 |
| Wiring | [docs/wiring.md](../../docs/wiring.md), [hardware.md](hardware.md) |
| Protocol | [protocol.md](protocol.md) |
| Device config | [`devices/hisense-m5stickc-plus2.yaml`](../../devices/hisense-m5stickc-plus2.yaml) |

## Wi-Fi modules known to use this bus
AEH-W4A1, AEH-W4B1, AEH-W4E1, AEH-W4G1, AEH-W4G2 and **AEH-W41H1**. They share the same 4-pin
connector (+5 V, A, B, GND) and protocol.

### AEH-W41H1 notes
- The stock module runs a link bring-up (device type `0A`, version `07`, then a `1E` heartbeat
  every cycle) before and while polling status. The component does the same with
  `handshake: true` (default) and falls back to plain polling if the AC never answers it.
- Its indoor units return long **160-byte** status frames with voltage and power data; 82- and
  150-byte frames of older units are also decoded.
- Known working setup reported publicly: [AndrewDemsDS/hisense-w41h1](https://github.com/AndrewDemsDS/hisense-w41h1)
  (ESP32, daily use).
- The module is rated 5 V / 450 mA; the M5StickC Plus2 + RS485 Hat stays well within that.

## Generic ids provided (used by `core/screen/*`)
`ac_climate`, `ac_link`, `ac_link_status`, `ac_fault`, `ac_fault_codes`, `ac_outdoor_temperature`.

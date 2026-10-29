# Compatibility

Hisense indoor units whose Wi-Fi module sits on a **4-pin RS-485 connector** (5 V or 12 V,
GND, A, B) speak the same "XM" bus at 9600 8N1, so they *should* work. "Should" is not
"does": this page separates the two.

**Tried it?** [Send a compatibility report](https://github.com/skydoy/ESPHome_AC/issues/new?template=compatibility_report.yml),
whether it works or not. Reports move models up this page.

Status key:
- ✅ **Confirmed:** works with this firmware (link to the report).
- 🧪 **In testing:** being tested with this firmware.
- 🔶 **Reported elsewhere:** reported working with *other* firmware on the same bus; untested here.
- ❌ **Not working:** confirmed not to work with this firmware.

## With this firmware

| Brand | Indoor unit | Wi-Fi module | Supply | Status | Notes |
|---|---|---|---|---|---|
| Hisense | maintainer's unit | AEH-W41H1 | 5 V | ✅ Working (maintainer-tested, v0.2.2) | [Details below](#aeh-w41h1-maintainers-unit) |

Boards: the maintainer tested the M5StickC Plus2 + RS485 Hat. The ATOM Lite + Tail485 build
(v0.2.7) uses the same protocol code but is not yet hardware-tested; reports
welcome.

### AEH-W41H1 (maintainer's unit)
Tested 2026-10-28 on an M5StickC Plus2 + RS485 Hat, powered from the 5 V (X) wire.

Confirmed (reported state and physically):
- Link handshake: AC Link connected, fault codes OK, 160-byte status frames.
- Modes Off / Cool / Heat / Fan only / Dry / Auto offered; Cool at 24 °C and 22 °C.
- Fan Auto → Low → High → Auto; vertical swing on/off; power off.
- Compressor frequency (28–30 Hz running, 0 off), outdoor temperature, outdoor coil
  temperature and room temperature all plausible.

Pending calibration (help welcome, see [debug logging](configuration.md#raw-frame-logging)):
- **Power:** decodes as `4.15 × b55²` like the reference, so at low load it moves in coarse
  steps (b55 = 4 → 66.4 W, 5 → 103.75 W). Not yet compared with a meter.
- **Indoor coil temperature** (byte 21): stayed at room temperature (27–28 °C) during about
  2 minutes of Cool, so the byte may mean something else on this unit.
- **Supply voltage, current, humidity:** this unit has not reported them so far; they are
  hidden by default and stay "unknown".

## Reported working with other firmware (untested here)

These units have been reported working with other open-source firmware that drives the same
bus. They are good candidates, not guarantees.

| Brand | Indoor unit | Wi-Fi module | Source |
|---|---|---|---|
| Hisense | CITY DC Inverter AS-13UW4RYRCM04G / AS-13UW4RYRCM04W | | [Druidblack/AC-Hisense](https://github.com/Druidblack/AC-Hisense) |
| Hisense | SILVER CRYSTAL SUPER DC Inverter AS-13UW4RVETG01 | | [Druidblack/AC-Hisense](https://github.com/Druidblack/AC-Hisense) |
| Hisense | Free Match multi-split 4AMW81U4RJC | | [Druidblack/AC-Hisense](https://github.com/Druidblack/AC-Hisense) |
| Newtek | NT-77HSDC12 | | [Druidblack/AC-Hisense](https://github.com/Druidblack/AC-Hisense) |
| Ballu | iGreen Pro DC BSAGI-07HN8, BSAGI-12HN8, BSAGI-18HN8_V4 | | [Druidblack/AC-Hisense](https://github.com/Druidblack/AC-Hisense) |
| Ballu | Platinum DC BSEI-09HN8_V3 | | [Druidblack/AC-Hisense](https://github.com/Druidblack/AC-Hisense) |
| Hisense | AST-12UW4RVETG00A | AEH-W4E1 | [akrabi/hisense_ac_esphome](https://github.com/akrabi/hisense_ac_esphome/blob/master/doc/hardware/COMPATIBLE_DEVICES.md) |
| Tornado | TOP-INV-120A (WIFI), also TOP-INV-140A / 180A | AEH-W4F1 | [akrabi/hisense_ac_esphome](https://github.com/akrabi/hisense_ac_esphome/blob/master/doc/hardware/COMPATIBLE_DEVICES.md) |
| Tornado | MULTI-12A (WIFI), AST-09UW4RVETV00D | AEH-W4G2 | [akrabi/hisense_ac_esphome](https://github.com/akrabi/hisense_ac_esphome/blob/master/doc/hardware/COMPATIBLE_DEVICES.md) |
| ACOND | ASTI-09UW4RVEDC00 | AEH-W4B1 | [akrabi/hisense_ac_esphome](https://github.com/akrabi/hisense_ac_esphome/blob/master/doc/hardware/COMPATIBLE_DEVICES.md) |
| Hisense | model not stated | AEH-W4G2 (reflashed in place) | [pio2398/W4G2](https://github.com/pio2398/W4G2) |
| Hisense | maintainer's units, model not stated | AEH-W41H1 | [AndrewDemsDS/hisense-w41h1](https://github.com/AndrewDemsDS/hisense-w41h1) |
| not stated (China market) | KFR-26GW | | [zoffypal/esphome_hisence_ac](https://github.com/zoffypal/esphome_hisence_ac) (v1/v2) |
| not stated (China market) | KFR-72GW | | [zoffypal/esphome_hisence_ac](https://github.com/zoffypal/esphome_hisence_ac) (v3; RS-485 writes unreliable, uses IR for commands) |

## Wi-Fi modules on this bus
AEH-W4A1, AEH-W4B1, AEH-W4E1, AEH-W4F1, AEH-W4G1, AEH-W4G2 and AEH-W41H1 all sit on the same
4-pin connector. **Measure the supply pin before wiring**: some units give 5 V, others 12 V
(see the [wiring guide](wiring.md)). Rebadged units (Tornado, ACOND, Ballu, Newtek…) often use
the same electronics.

## How to check your unit in a minute
1. It pairs with the **ConnectLife** app, an older Hisense app, or a rebadged equivalent.
2. The Wi-Fi module is a small dongle behind the front panel; its part number starts with **AEH-W4**.
3. Its connector has **4 pins**. Measure 5 V or 12 V between the supply pin and GND.

If all three are true, it is a good candidate. See [brands/hisense/README.md](../brands/hisense/README.md)
for module notes.

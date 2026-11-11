# Compatibility

**Is my AC supported?** Find your brand below. Not listed, or tried it?
**[Report my AC](https://github.com/skydoy/ESPHome_AC/issues/new?template=compatibility_report.yml)**
(2 minutes, no coding). Reports move models up this page, including "does not work".

Hisense indoor units whose Wi-Fi module sits on a **4-pin RS-485 connector** (5 V or 12 V, GND, A, B) speak the same "XM" bus at 9600 8N1, so they *should* work. "Should" is not "does". This page separates the two.

Status key:

- ✅ **Confirmed:** works with this firmware.
- 🧪 **In testing:** being tested with this firmware.
- 🔶 **Reported elsewhere:** reported working with *other* firmware on the same bus; untested here.
- ❌ **Not working:** confirmed not to work with this firmware. None are listed yet.
- ❔ **Untested:** expected to work (same bus), nobody has tried it with this firmware yet.

## With this firmware

| Brand | Indoor unit | Wi-Fi module | Board tested | Supply | Status | Notes |
|---|---|---|---|---|---|---|
| Hisense | maintainer's unit | AEH-W41H1 | M5StickC Plus2 + RS485 Hat | 5 V | ✅ Works (2026-10-29) | Mode, temperature, fan, vertical swing confirmed; power, voltage, current, humidity readings partial. [Details](#aeh-w41h1-maintainers-unit). Reported by [@skydoy](https://github.com/skydoy). |
| Hisense | any | AEH-W41H1 / AEH-W4x | ATOM Lite + Tail485 | 5 V / 12 V | ❔ Untested | Same protocol code, not yet tried on a real unit. |

The maintainer tested the M5StickC Plus2 + RS485 Hat. The ATOM Lite + Tail485 build uses the same protocol code and is checked automatically, but it is not hardware-tested. Reports are welcome.

### AEH-W41H1 (maintainer's unit)

Tested 2026-10-28 and 2026-10-29 (commands) on an M5StickC Plus2 + RS485 Hat, powered from the 5 V (X) wire.

Confirmed (reported state and physically):

- Link handshake: AC Link connected, fault codes OK, 160-byte status frames.
- Modes Off / Cool / Heat / Fan only / Dry / Auto offered; Cool at 24 °C and 22 °C.
- Fan Auto → Low → High → Auto; vertical swing on/off; power off.
- Compressor frequency (28–30 Hz running, 0 off), outdoor temperature, outdoor coil temperature and room temperature all plausible.

Pending calibration (see [raw frame logging](configuration.md#raw-frame-logging)):

- **Power:** decodes as `4.15 × b55²` like the reference, so at low load it moves in coarse steps (b55 = 4 → 66.4 W, 5 → 103.75 W). Not yet compared with a meter.
- **Indoor coil temperature** (byte 21): stayed at room temperature (27–28 °C) during about 2 minutes of Cool, so the byte may mean something else on this unit.
- **Supply voltage, current, humidity:** this unit has not reported them so far. They stay hidden by default and read "unknown".

## Reported working with other firmware (untested here)

These units have been reported working with other open-source firmware that drives the same bus. They are candidates, not guarantees.

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

AEH-W4A1, AEH-W4B1, AEH-W4E1, AEH-W4F1, AEH-W4G1, AEH-W4G2 and AEH-W41H1 all sit on the same 4-pin connector. **Measure the supply pin before wiring.** Some units give 5 V, others 12 V ([wiring](wiring.md)). Rebadged units (Tornado, ACOND, Ballu, Newtek and others) often use the same electronics.

## Report from the device (AC Identity)

After the board links to the AC it runs a **read-only identify** (device type, firmware version, capabilities, and one full status frame). Open `http://<device-ip>/`:

1. Check **AC Identity** (compact summary, also in the INFO log).
2. Open or copy **Report my AC**. It is a short link on the report Worker that redirects to the prefilled GitHub issue form (identity + firmware). The full GitHub URL is only in the INFO log so the device web UI stays readable.
3. Fill the remaining fields (brand/model, board, result) and press **Submit**.

You can press **Identify AC** anytime to re-run the read-only queries and refresh the link. The short **Report my AC** link still lands on GitHub for you to fill brand and model. The Worker behaviour is described in [worker/README.md](https://github.com/skydoy/ESPHome_AC/blob/main/worker/README.md). This page does not repeat deploy steps.

### Automatic compatibility report (optional)

With **Auto report** on (the default), after identify completes and Wi-Fi is up the device POSTs once: `identity`, `firmware`, `board`, and a **sha256 of the Wi-Fi MAC** as `device_id` (never the raw MAC). Optional `status_hex` may be included. No IP is stored. Turn **Auto report** off in Home Assistant or on the device web page to disable it. The same identity is only sent again when it changes. Control of the AC stays local. This is an optional anonymous compatibility signal, not cloud control.

## Check your unit in a minute

1. It pairs with the **ConnectLife** app, an older Hisense app, or a rebadged equivalent.
2. The Wi-Fi module is a small dongle behind the front panel. Its part number starts with **AEH-W4**.
3. Its connector has **4 pins**. Measure 5 V or 12 V between the supply pin and GND.

If all three are true, it is a good candidate. Module notes: [brands/hisense/README.md](https://github.com/skydoy/ESPHome_AC/blob/main/brands/hisense/README.md).

What is still unknown: [Open questions](open-questions.md). Repository copy: [docs/compatibility.md](https://github.com/skydoy/ESPHome_AC/blob/main/docs/compatibility.md).

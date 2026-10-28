# Physical-layer adapters

The ESP32 never connects directly to the AC. Each AC family needs its own physical adapter
between the ESP's 3.3 V UART and the AC's service/Wi-Fi port. This table lists what this
project supports and what it could support next.

**Only Hisense is implemented.** The other rows are listed so people with those units know
where to look today and can volunteer as testers. The links point to existing upstream projects;
no code from them is included in this repository.

| AC family | Port / bus | Adapter needed | Status here | Existing upstream projects |
|---|---|---|---|---|
| Hisense / Hisense OEM (AEH-W4x, AEH-W41H1 modules) | 4-pin Wi-Fi module port, RS-485 9600 8N1, +5 V supply | RS-485 transceiver (M5Stack Hat RS485, or MAX485/SP485 with auto direction) | **Supported** | This repo; [AndrewDemsDS/hisense-w41h1](https://github.com/AndrewDemsDS/hisense-w41h1), [akrabi/hisense_ac_esphome](https://github.com/akrabi/hisense_ac_esphome) |
| Mitsubishi Electric | CN105, TTL UART 2400 8E1, 5 V and 12 V on the connector | 5 V ↔ 3.3 V level shifting UART (CN105 cable) | Not implemented - looking for testers | [echavet/MitsubishiCN105ESPHome](https://github.com/echavet/MitsubishiCN105ESPHome), [geoffdavis/esphome-mitsubishiheatpump](https://github.com/geoffdavis/esphome-mitsubishiheatpump) |
| Daikin (S21 port) | S21, TTL UART 2400 8E2, 5 V/14 V on the connector | 5 V ↔ 3.3 V level shifting UART | Not implemented - looking for testers | [joshbenner/esphome-daikin-s21](https://github.com/joshbenner/esphome-daikin-s21) |
| Daikin (P1/P2 bus, wired-remote bus) | P1/P2 two-wire bus (≈15 V DC, Home-Bus) | Dedicated P1/P2 transceiver (e.g. MM1192/XL1192 based board) | Not implemented - looking for testers | [Arnold-n/P1P2MQTT](https://github.com/Arnold-n/P1P2MQTT) |
| Midea and OEMs | USB-A Wi-Fi dongle port (OSK102/OSK103-type dongles), TTL UART 9600 8N1, 5 V | 5 V ↔ 3.3 V level shifting UART | Not implemented - looking for testers (ESPHome has a native `midea` climate platform) | [ESPHome Midea](https://esphome.io/components/climate/midea/), [dudanov/MideaUART](https://github.com/dudanov/MideaUART) |
| Toshiba | Wi-Fi adapter port, TTL UART 9600 8E1, 5 V | 5 V ↔ 3.3 V level shifting UART | Not implemented - looking for testers | [pedobry/esphome_toshiba_suzumi](https://github.com/pedobry/esphome_toshiba_suzumi) |
| Fujitsu | Wired-remote bus (single-wire, ≈12 V) | Dedicated LIN-style transceiver | Not implemented - looking for testers | [unreality/FujiHeatPump](https://github.com/unreality/FujiHeatPump) |
| Any brand | Infrared (fallback, one-way) | IR LED (the M5StickC Plus2 has one on G19) | Not implemented - looking for testers (ESPHome has native IR climate platforms) | [ESPHome IR remote climate](https://esphome.io/components/climate/climate_ir/), [crankyoldgit/IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266) |

Voltage levels, pin orders and baud rates above are summaries from the linked projects; always
confirm them on your own unit with a multimeter before connecting anything. See the safety notes
in [docs/wiring.md](../docs/wiring.md) and [docs/configuration.md](../docs/configuration.md#safety).

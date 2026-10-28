# Hardware

## Parts
- M5StickC Plus2 (ESP32-PICO-V3-02, 8 MB flash)
- M5Stack Hat RS485 (SKU U067: SP485EEN transceiver, automatic direction control via a
  transistor, AOZ1282CI 12 V -> 5 V buck converter)
- A 4-pin JST/XH-style plug matching the AC's Wi-Fi module port (or reuse the cable of the
  original AEH-W4x module)

## Compatible Wi-Fi module ports
The AEH-W4A1, W4B1, W4E1, W4G1, W4G2 and **AEH-W41H1** modules all use the same 4-pin port and
the same F4 F5 RS-485 protocol. Typical pin order on the module (check it!):

| Module pin | Signal |
|---|---|
| 1 | +5 V (the stock W41H1 is rated 5 V / 450 mA) |
| 2 | A (+) |
| 3 | B (−) |
| 4 | GND |

## Wiring
| AC Wi-Fi module port (MEASURE FIRST!) | Goes to |
|---|---|
| +5 V | M5StickC Plus2 **USB-C VBUS** (recommended) or header pin 8 "5VIN" |
| A (+) | Hat RS485 terminal pin 2 (A) |
| B (−) | Hat RS485 terminal pin 1 (B) |
| GND | Hat RS485 terminal pin 4 (GND) |
| — | Hat terminal pin 3 (+12 V): **leave unconnected** |

Hat -> Stick header: TX = G0, RX = G26, 5VOUT powers the SP485EEN, 5VIN = buck output.

How to check the plug with a multimeter (AC powered, module unplugged):
- GND is continuous with the chassis / cable shield.
- +5 V reads about 5 V to GND.
- At rest A−B reads a small positive differential voltage (+0.2 … +5 V).

If A and B are swapped nothing is damaged, the link just stays in "Searching": swap them.

## Caveats
- **Boot pin G0:** the hat's TX line (G0) drives a transistor base through 4.7 kΩ. If the
  Stick ever starts in download mode ("waiting for download" on the serial log, blank screen)
  when powered from the AC, power it once from USB, or add a 10 kΩ pull-up from G0 to 3.3 V.
  Test this before closing the AC panel.
- Do not enable the microphone (G0 is its clock) or anything else on G0/G26.
- G19 is the red LED and the IR transmitter; it is unused (see README "Ideas").
- Always switch the AC off at the breaker before opening the indoor unit.

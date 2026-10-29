# Hisense "XM" RS-485 bus protocol (F4 F5)

Notes for `components/hisense_ac/hisense_protocol.{h,cpp}`. Byte offsets are 0-based and refer
to the **decoded** frame (after removing byte stuffing). Everything here was written from
public protocol notes and bench observations; see "Sources" at the end.

## Physical layer
- RS-485 half duplex, 9600 baud, 8N1, on the 4-pin Wi-Fi module connector of the indoor unit:
  +5 V, A, B, GND (the AC is the bus master for power; the module only answers and polls).
- The module side is the master of the conversation: it sends a request, the AC replies.

## Framing
```
F4 F5 | dir | 40 | LEN | ... | class (13) | sub (14) | payload ... | CK_HI CK_LO | F4 FB
```
- `dir`: `00` module -> AC, `01` AC -> module. Only `01` frames are accepted (the auto-direction
  RS485 hat can echo our own transmission).
- Decoded length = `LEN + 9`.
- Checksum: 16-bit sum of bytes `2 .. n-5`, big endian, stored at `n-4`/`n-3`.
- Byte stuffing: every `F4` between the header and the `F4 FB` tail (checksum included) is sent
  twice on the wire.
- Module frames carry the device-type pair (from the `0A` reply, bytes 16/17) at bytes 7/8.

## Link bring-up (stock AEH-W41H1 behaviour, `handshake: true`)
1. `0A` device-type request (type pair 00 00) -> reply gives the type pair.
2. `07` version request (reply: see below).
3. Every cycle (default 1 s): `1E` heartbeat (byte 16 = `F0` until the AC answered once,
   then `B0`), queued commands, `66` status poll, and once a `66/40` ProductType
   (capabilities) poll. If the link is lost the cycle restarts with `0A`.

With `handshake: false` the component only sends status polls and commands (what older
community components do). After 5 failed `0A` attempts the component also falls back to
plain polling.

## Command frame (class `65`, 50 bytes decoded)
A zero byte means "leave unchanged". Most fields are `(value << 1) | 1`.

| Byte | Meaning |
|---|---|
| 16 | Fan: `01` auto, otherwise `((index + 4) << 1) | 1`, index 1..5 = low .. high |
| 17 | Sleep: `profile * 2 + 1` (0 profile = off) |
| 18 | Power (`0C` on / `04` off) OR mode nibble `((mode << 1) | 1) << 4` (fan 0, heat 1, cool 2, dry 3, auto 4) |
| 19 | Set point `(t << 1) | 1` in the panel unit |
| 23 | `04` = beep on acknowledge |
| 31 | `01` marker |
| 32 | Swing: vertical `C0` on / `40` off; horizontal `30` on / `10` off |
| 33 | Eco `30`/`10`, turbo `0C`/`04` |
| 35 | Quiet `30`/`10` |
| 36 | Display LED `C0`/`40` |
| 37 | +8 °C heating `03`/`01`; `14` also set when sending horizontal swing |

Eco, turbo, sleep, quiet and +8 °C are sent as separate frames spaced by
`special_command_gap` (default 8 s), "off" before "on", because several indoor units ignore a
second special flag that arrives too quickly.

## Status reply (class `66`, sub != `40`)
Short units send 82 bytes, W4G1-type units 150 bytes, the W41H1 160 bytes. Fields:

| Byte | Meaning |
|---|---|
| 16 | Fan: <= `01` auto, `02` quiet step, else `(v - 8) / 2` -> 1..5 |
| 17 | Sleep `>> 1` |
| 18 | Power `& 0C`, mode `(b >> 4) & 7` (>= 4 = auto) |
| 19 | Set point |
| 20 | Room temperature (signed) |
| 21 | Indoor coil temperature (signed; unconfirmed on the W41H1, read room temperature in Cool) |
| 22 / 23 | Humidity set point / indoor humidity (0 or > 100 = not available); `Humidity Set Point` sensor |
| 26 | bit 1: panel is in °F (`AC Panel Unit` text sensor) |
| 35 | bit 7 vertical swing, bit 6 horizontal swing, bit 4 aux heater, bit 2 eco, bit 1 turbo |
| 36 | bit 2 quiet |
| 37 | bit 7 display LED |
| 39 / 40 | Indoor / module fault bits |
| 41 / 42 | Compressor target / actual frequency (Hz) |
| 44 / 45 | Outdoor / outdoor coil temperature (signed °C; both `EC` = not valid yet) |
| 50 | Supply voltage (long frames only) |
| 55 / 56 | Power: 160-byte frames `4.15 * b55^2` W (bench fit, coarse at low load), 150-byte frames `b55:b56` W |
| 64 | Outdoor fault bits |
| 66 | Protection bits (bit 7 = +8 °C mode) |
| 77 | bit 0: +8 °C mode (alternative location) |

Current is estimated as `P / (V * 0.95)`.

The `66/40` ProductType reply (capabilities) is published as the `AC Capabilities` text sensor,
and the `0A` type pair as `AC Device Type` (`XX XX`).

## Version reply (class `07`, unconfirmed)
No reply has been captured yet. The stock W41H1 firmware (reverse engineered) reads a 16-bit
word of unknown meaning from bytes 16/17 and the mainboard protocol version, big endian, from
bytes 18/19 (it enables newer features from `0x0214`). The `AC Firmware Version` text sensor is
best effort:

- bytes 16.. are printable ASCII (trailing `00` ignored): that text;
- otherwise `Protocol 0xXXXX (raw ..)` with bytes 18/19 and up to 16 payload bytes in hex from
  byte 16, or `Raw ..` (from byte 14) if the reply is shorter.

The raw reply frame is always logged at DEBUG (`RX version frame`) and the decoded text at INFO.
Please attach that log to an issue so the layout can be confirmed.

## Bus error counters
`AC Bus Checksum Errors` counts complete frames with a bad checksum, `AC Bus Framing Errors`
frames dropped for a bad length, an overflow, a bad `F4` escape or a new header in the middle
of a frame. Both count since boot (total increasing).

## Not on the bus
- **iFeel** (room temperature measured at the remote) is only sent over infrared
  (Kelon 168-bit IR protocol). It is not implemented.

## Sources
- AndrewDemsDS/hisense-w41h1 (MIT): W41H1 link bring-up, frame templates, 160-byte status map,
  power calibration.
- akrabi/hisense_ac_esphome (MIT per README) and its protocol notes.
- Facts only (no code copied): Druidblack/AC-Hisense, pio2398/W4G2 (GPL-3.0), straga/scrivo_project,
  vins.vins (4pda forum), zoffypal/esphome_hisence_ac, pslawinski/esphome_airconintl.

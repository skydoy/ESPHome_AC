// Hisense / Hisense-OEM "XM" RS-485 bus protocol (the F4 F5 protocol spoken between the
// indoor unit and the AEH-W4x Wi-Fi modules: W4A1, W4B1, W4E1, W4G2, W41H1, ...).
//
// This file is platform independent (no ESPHome includes) so it can be unit tested on a
// host compiler. See brands/hisense/protocol.md for the byte map and the sources of every fact.
//
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace esphome {
namespace hisense_ac {
namespace proto {

// ---- Framing --------------------------------------------------------------------------------
//   F4 F5 | dir | 40 | LEN | ... | class (byte 13) | sub (byte 14) | payload | CK_HI CK_LO | F4 FB
// dir: 0x00 module -> AC, 0x01 AC -> module. Decoded length = LEN + 9.
// Checksum: 16-bit sum of bytes 2 .. n-5, big endian.
// Byte stuffing: every 0xF4 between the header and the end tag (checksum included) is doubled.

static constexpr uint8_t HEAD0 = 0xF4;
static constexpr uint8_t HEAD1 = 0xF5;
static constexpr uint8_t TAIL1 = 0xFB;
static constexpr uint8_t DIR_TO_AC = 0x00;
static constexpr uint8_t DIR_FROM_AC = 0x01;

static constexpr size_t MAX_FRAME = 200;      // decoded
static constexpr size_t MAX_WIRE = 2 * MAX_FRAME;

static constexpr size_t OFF_DIR = 2;
static constexpr size_t OFF_LEN = 4;
static constexpr size_t OFF_TYPE_HI = 7;      // device-type pair in module frames
static constexpr size_t OFF_TYPE_LO = 8;
static constexpr size_t OFF_CLASS = 13;
static constexpr size_t OFF_SUB = 14;

static constexpr uint8_t CLASS_VERSION = 0x07;
static constexpr uint8_t CLASS_DEVTYPE = 0x0A;
static constexpr uint8_t CLASS_LINK = 0x1E;     // heartbeat
static constexpr uint8_t CLASS_COMMAND = 0x65;
static constexpr uint8_t CLASS_STATUS = 0x66;
static constexpr uint8_t SUB_PRODUCT_TYPE = 0x40;

/// Sum of bytes [2, n-4) of a decoded frame.
uint16_t checksum(const uint8_t *frame, size_t len);
/// True when the decoded frame has a valid header, length, end tag and checksum.
bool frame_valid(const uint8_t *frame, size_t len);
/// Writes the checksum into bytes n-4/n-3 and the F4 FB tail into n-2/n-1.
void seal(uint8_t *frame, size_t len);
/// Byte stuffing for the wire. Returns the wire length, 0 if it does not fit.
size_t stuff(const uint8_t *frame, size_t len, uint8_t *out, size_t out_cap);

/// Incremental receiver: removes stuffing and resynchronises on noise. Feed every byte read
/// from the UART; when feed() returns true a complete, checksum-valid frame is in frame().
class FrameAssembler {
 public:
  bool feed(uint8_t b);
  const uint8_t *frame() const { return this->buf_; }
  size_t size() const { return this->len_; }
  uint32_t checksum_errors() const { return this->checksum_errors_; }
  uint32_t framing_errors() const { return this->framing_errors_; }
  void reset();

 protected:
  void restart_with_header_();
  uint8_t buf_[MAX_FRAME];
  size_t len_{0};
  size_t expected_{0};
  bool pending_f4_{false};
  bool synced_{false};
  bool got_f4_{false};
  uint32_t checksum_errors_{0};
  uint32_t framing_errors_{0};
};

// ---- Fixed module -> AC frames (decoded, unsealed templates are sealed at build time) ----------
/// 0x0A device-type request (link bring-up, step 1). Always uses type 00 00.
size_t build_devtype_request(uint8_t *out, size_t cap);
/// 0x07 version request (link bring-up, step 2).
size_t build_version_request(uint8_t t_hi, uint8_t t_lo, uint8_t *out, size_t cap);
/// 0x1E heartbeat. heard_ac: we already received a 0x1E reply from the AC.
size_t build_heartbeat(uint8_t t_hi, uint8_t t_lo, bool heard_ac, uint8_t *out, size_t cap);
/// 0x66 status poll (sub 0x00) or ProductType/capability poll (sub 0x40).
size_t build_status_request(uint8_t t_hi, uint8_t t_lo, uint8_t sub, uint8_t *out, size_t cap);

// ---- Command frame (class 0x65, 50 bytes decoded) --------------------------------------------
// A zero payload byte means "leave this setting unchanged"; most fields are (value << 1) | 1.
enum class Mode : uint8_t { FAN = 0, HEAT = 1, COOL = 2, DRY = 3, AUTO = 4 };
enum class Fan : uint8_t { AUTO = 0, LOW = 1, MEDIUM_LOW = 2, MEDIUM = 3, MEDIUM_HIGH = 4, HIGH = 5 };

struct Command {
  // Every field is optional: has_x == false leaves it untouched on the AC.
  bool has_power{false};
  bool power{false};
  bool has_mode{false};
  Mode mode{Mode::COOL};
  bool has_setpoint{false};
  uint8_t setpoint{24};  // in the unit the AC panel uses (C or F)
  bool has_fan{false};
  Fan fan{Fan::AUTO};
  bool has_vswing{false};
  bool vswing{false};
  bool has_hswing{false};
  bool hswing{false};
  bool has_eco{false};
  bool eco{false};
  bool has_turbo{false};
  bool turbo{false};
  bool has_quiet{false};
  bool quiet{false};
  bool has_display{false};
  bool display{false};
  bool has_heat8{false};
  bool heat8{false};
  bool has_sleep{false};
  uint8_t sleep{0};  // 0 = off, 1..4 = sleep profile
  bool beep{true};
};

static constexpr size_t COMMAND_LEN = 50;
/// Builds a decoded, sealed command frame into out[COMMAND_LEN].
void build_command(const Command &cmd, uint8_t t_hi, uint8_t t_lo, uint8_t *out);

// ---- Status frame decoding -----------------------------------------------------------------
struct Faults {
  uint8_t indoor{0};    // byte 39
  uint8_t module{0};    // byte 40
  uint8_t outdoor{0};   // byte 64
  uint8_t protect{0};   // byte 66 without bit 7 (bit 7 is the +8 C mode flag)
  bool any() const { return indoor || module || outdoor || protect; }
};

struct Status {
  size_t frame_len{0};
  bool power{false};
  Mode mode{Mode::COOL};
  Fan fan{Fan::AUTO};
  bool fan_quiet_step{false};  // fan byte reported the dedicated "quiet" step (0x02)
  uint8_t sleep{0};
  bool fahrenheit{false};
  uint8_t setpoint{0};         // panel unit
  int16_t room_temp{0};        // panel unit
  int16_t indoor_coil_temp{0};
  bool has_humidity{false};
  uint8_t humidity_setting{0};
  uint8_t humidity{0};
  bool vswing{false};
  bool hswing{false};
  bool aux_heater{false};
  bool eco{false};
  bool turbo{false};
  bool quiet{false};
  bool display{false};
  bool heat8{false};
  Faults faults;
  // Outdoor block, only when frame_len > 46
  bool has_outdoor{false};
  bool outdoor_valid{false};  // false right after mains power returns (bytes 44 and 45 = 0xEC)
  uint8_t compressor_target_hz{0};  // byte 41
  uint8_t compressor_hz{0};         // byte 42 (actual, per W41H1 bench data)
  int16_t outdoor_temp{0};          // byte 44, signed C
  int16_t outdoor_coil_temp{0};     // byte 45, signed C
  // Electrical block, only in long frames (>= 150 bytes decoded)
  bool has_electrical{false};
  uint8_t voltage{0};               // byte 50, volts
  uint8_t power_raw_hi{0};          // byte 55
  uint8_t power_raw_lo{0};          // byte 56
};

/// Minimum decoded length of a status frame this decoder accepts.
static constexpr size_t STATUS_MIN_LEN = 49;

/// Decodes a validated 0x66 status reply. Returns false if it is not one.
bool parse_status(const uint8_t *frame, size_t len, Status *out);

struct Capabilities {
  bool valid{false};
  bool cool_heat{false};
  bool power_save{false};
  bool purify{false};
  bool fan_mute{false};
  bool infinite_fan{false};
  bool heat8{false};
  bool humidity{false};
};
/// Decodes a ProductType (0x66 / sub 0x40) reply.
bool parse_capabilities(const uint8_t *frame, size_t len, Capabilities *out);

/// Decodes a 0x0A device-type reply: the type pair at bytes 16 and 17.
bool parse_devtype(const uint8_t *frame, size_t len, uint8_t *t_hi, uint8_t *t_lo);

/// Human readable list of active faults, "" when none. Comma separated.
std::string describe_faults(const Faults &f);

/// Power estimate in watts. Long (160-byte) W41H1 frames: 4.15 * b55^2 (bench calibrated);
/// 150-byte frames: big endian b55:b56 (W4G1 bench). Returns < 0 when unknown.
float power_watts(const Status &s);

/// "XX XX XX" for bytes [off, off + n) of a frame, clamped to len. Used by the frame logger.
std::string hex_bytes(const uint8_t *frame, size_t len, size_t off, size_t n);

inline float f_to_c(float f) { return (f - 32.0f) * 5.0f / 9.0f; }
inline float c_to_f(float c) { return c * 9.0f / 5.0f + 32.0f; }

}  // namespace proto
}  // namespace hisense_ac
}  // namespace esphome

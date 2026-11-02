// Hisense "XM" bus: frames that CHANGE the AC state or keep a control session alive
// (class 0x65 command, 0x1E heartbeat) and the command queue.
//
// Deliberately separate from the read-only protocol library (components/hisense_proto), so
// firmware that must never write to the AC (the setup / detection image, components/ac_setup)
// cannot link these functions. Platform independent; unit tested in tests/test_protocol.cpp.
//
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

#include <cstddef>
#include <cstdint>

#include "../hisense_proto/hisense_protocol.h"

namespace esphome {
namespace hisense_ac {
namespace proto {

// ---- Heartbeat (class 0x1E) ------------------------------------------------------------------
/// 0x1E heartbeat. heard_ac: we already received a 0x1E reply from the AC.
size_t build_heartbeat(uint8_t t_hi, uint8_t t_lo, bool heard_ac, uint8_t *out, size_t cap);

// ---- Command frame (class 0x65, 50 bytes decoded) --------------------------------------------
// A zero payload byte means "leave this setting unchanged"; most fields are (value << 1) | 1.
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

/// Which groups of settings a command carries (bit mask). Commands of one single group can be
/// merged while they wait in the queue; swing axes and special modes stay in frames of their own.
enum CommandGroup : uint8_t {
  GROUP_NONE = 0,
  GROUP_BASIC = 1 << 0,    // power, mode, set point, fan
  GROUP_VSWING = 1 << 1,
  GROUP_HSWING = 1 << 2,
  GROUP_DISPLAY = 1 << 3,
  GROUP_SPECIAL = 1 << 4,  // eco, turbo, quiet, heat 8C, sleep (need spacing on the AC)
};
uint8_t command_groups(const Command &c);
/// Copies every field set in `from` into `into` (the newer value wins); beep follows `from`.
void merge_command(Command &into, const Command &from);

/// Wrap-safe "now >= t" for millisecond timestamps.
inline bool time_reached(uint32_t now, uint32_t t) { return static_cast<int32_t>(now - t) >= 0; }

/// Fixed-size command queue (no heap). Plain commands of the same group are coalesced, so a burst
/// of set-point clicks costs one bus frame; special commands are spaced `special_gap` ms apart
/// because the AC ignores special modes that arrive too close together.
class CommandQueue {
 public:
  static constexpr size_t CAPACITY = 12;
  enum class Push : uint8_t { ADDED, MERGED, FULL };
  struct Item {
    Command cmd;
    uint32_t not_before{0};
    bool special{false};
  };

  Push push(const Command &cmd, bool special, uint32_t now, uint32_t special_gap);
  /// Removes and returns the first command whose time has come.
  bool pop_ready(uint32_t now, Item *out);
  bool has_ready(uint32_t now) const;
  size_t size() const { return this->len_; }
  bool empty() const { return this->len_ == 0; }
  void clear() { this->len_ = 0; }

 protected:
  Item items_[CAPACITY];
  size_t len_{0};
  uint32_t last_special_at_{0};
  bool any_special_sent_{false};
};

}  // namespace proto
}  // namespace hisense_ac
}  // namespace esphome

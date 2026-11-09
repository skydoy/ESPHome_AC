// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Implementation of the state-changing frames and the command queue. See hisense_command.h.
#include "hisense_command.h"

#include <cstring>

namespace esphome {
namespace hisense_ac {
namespace proto {

// ---- Heartbeat -------------------------------------------------------------------------------
size_t build_heartbeat(uint8_t t_hi, uint8_t t_lo, bool heard_ac, uint8_t *out, size_t cap) {
  uint8_t f[28] = {0xF4, 0xF5, 0x00, 0x40, 0x13, 0x00, 0x00, t_hi, t_lo, 0xFE, 0x01, 0x00, 0x00, CLASS_LINK,
                   0x00, 0x00, 0xB0, 0x80, 0x20, 0x00, 0x00, 0x00, 0x40, 0x00, 0, 0, 0, 0};
  // Byte 16 bit 6 set = "have not heard the AC's heartbeat yet".
  f[16] = heard_ac ? 0xB0 : 0xF0;
  return finish_frame(f, sizeof(f), out, cap);
}

// ---- Command ---------------------------------------------------------------------------------
static inline uint8_t onoff_(bool on, uint8_t on_v, uint8_t off_v) { return on ? on_v : off_v; }

void build_command(const Command &c, uint8_t t_hi, uint8_t t_lo, uint8_t *f) {
  std::memset(f, 0, COMMAND_LEN);
  const uint8_t head[16] = {0xF4, 0xF5, 0x00, 0x40, 0x29, 0x00, 0x00, t_hi,
                            t_lo, 0xFE, 0x01, 0x00, 0x00, CLASS_COMMAND, 0x00, 0x00};
  std::memcpy(f, head, sizeof(head));

  if (c.has_fan)
    f[16] = c.fan == Fan::AUTO ? 0x01 : static_cast<uint8_t>(((static_cast<uint8_t>(c.fan) + 4) << 1) | 1);
  if (c.has_sleep)
    f[17] = static_cast<uint8_t>(((c.sleep > 4 ? 4 : c.sleep) << 1) | 1);
  if (c.has_mode || c.has_power) {
    // Power bits: 0x0C = on, 0x04 = off. A mode change always means "on".
    const bool on = c.has_power ? c.power : true;
    uint8_t v = on ? 0x0C : 0x04;
    if (c.has_mode && on)
      v |= static_cast<uint8_t>(((static_cast<uint8_t>(c.mode) << 1) | 1) << 4);
    f[18] = v;
  }
  if (c.has_setpoint)
    f[19] = static_cast<uint8_t>((c.setpoint << 1) | 1);
  if (c.beep)
    f[23] = 0x04;
  f[31] = 0x01;  // "this is a command" marker; frames without it are accepted and ignored
  if (c.has_vswing)
    f[32] |= onoff_(c.vswing, 0xC0, 0x40);
  if (c.has_hswing) {
    f[32] |= onoff_(c.hswing, 0x30, 0x10);
    f[37] |= 0x14;
  }
  if (c.has_eco)
    f[33] |= onoff_(c.eco, 0x30, 0x10);
  if (c.has_turbo)
    f[33] |= onoff_(c.turbo, 0x0C, 0x04);
  if (c.has_quiet)
    f[35] = onoff_(c.quiet, 0x30, 0x10);
  if (c.has_display)
    // Hisense panel light is inverted: 0x40 turns it ON, 0xC0 turns it OFF (confirmed on a real unit).
    f[36] = onoff_(c.display, 0x40, 0xC0);
  if (c.has_heat8)
    f[37] |= onoff_(c.heat8, 0x03, 0x01);
  seal(f, COMMAND_LEN);
}

// ---- Command queue ---------------------------------------------------------------------------
uint8_t command_groups(const Command &c) {
  uint8_t g = GROUP_NONE;
  if (c.has_power || c.has_mode || c.has_setpoint || c.has_fan)
    g |= GROUP_BASIC;
  if (c.has_vswing)
    g |= GROUP_VSWING;
  if (c.has_hswing)
    g |= GROUP_HSWING;
  if (c.has_display)
    g |= GROUP_DISPLAY;
  if (c.has_eco || c.has_turbo || c.has_quiet || c.has_heat8 || c.has_sleep)
    g |= GROUP_SPECIAL;
  return g;
}

void merge_command(Command &into, const Command &from) {
#define HISENSE_MERGE(f) \
  if (from.has_##f) { \
    into.has_##f = true; \
    into.f = from.f; \
  }
  HISENSE_MERGE(power)
  HISENSE_MERGE(mode)
  HISENSE_MERGE(setpoint)
  HISENSE_MERGE(fan)
  HISENSE_MERGE(vswing)
  HISENSE_MERGE(hswing)
  HISENSE_MERGE(eco)
  HISENSE_MERGE(turbo)
  HISENSE_MERGE(quiet)
  HISENSE_MERGE(display)
  HISENSE_MERGE(heat8)
  HISENSE_MERGE(sleep)
#undef HISENSE_MERGE
  into.beep = from.beep;
}

static bool single_group(uint8_t g) { return g != 0 && (g & (g - 1)) == 0; }

CommandQueue::Push CommandQueue::push(const Command &cmd, bool special, uint32_t now, uint32_t special_gap) {
  if (!special && this->len_ > 0) {
    Item &last = this->items_[this->len_ - 1];
    const uint8_t g = command_groups(cmd);
    if (!last.special && single_group(g) && g != GROUP_SPECIAL && command_groups(last.cmd) == g) {
      merge_command(last.cmd, cmd);
      return Push::MERGED;
    }
  }
  if (this->len_ >= CAPACITY)
    return Push::FULL;
  Item q;
  q.cmd = cmd;
  q.special = special;
  q.not_before = now;
  if (special) {
    if (this->any_special_sent_ && !time_reached(now, this->last_special_at_ + special_gap))
      q.not_before = this->last_special_at_ + special_gap;
    for (size_t i = 0; i < this->len_; i++) {
      if (this->items_[i].special) {
        const uint32_t t = this->items_[i].not_before + special_gap;
        if (!time_reached(q.not_before, t))
          q.not_before = t;
      }
    }
  }
  this->items_[this->len_++] = q;
  return Push::ADDED;
}

bool CommandQueue::has_ready(uint32_t now) const {
  for (size_t i = 0; i < this->len_; i++)
    if (time_reached(now, this->items_[i].not_before))
      return true;
  return false;
}

bool CommandQueue::pop_ready(uint32_t now, Item *out) {
  for (size_t i = 0; i < this->len_; i++) {
    if (!time_reached(now, this->items_[i].not_before))
      continue;
    *out = this->items_[i];
    for (size_t j = i + 1; j < this->len_; j++)
      this->items_[j - 1] = this->items_[j];
    this->len_--;
    if (out->special) {
      this->last_special_at_ = now;
      this->any_special_sent_ = true;
    }
    return true;
  }
  return false;
}

}  // namespace proto
}  // namespace hisense_ac
}  // namespace esphome

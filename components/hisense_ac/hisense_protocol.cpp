// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
// Implementation of the platform independent protocol helpers. See hisense_protocol.h.
#include "hisense_protocol.h"

#include <cstring>

namespace esphome {
namespace hisense_ac {
namespace proto {

uint16_t checksum(const uint8_t *frame, size_t len) {
  uint16_t sum = 0;
  if (len < 9)
    return 0;
  for (size_t i = 2; i < len - 4; i++)
    sum = static_cast<uint16_t>(sum + frame[i]);
  return sum;
}

bool frame_valid(const uint8_t *frame, size_t len) {
  if (frame == nullptr || len < 9 || len > MAX_FRAME)
    return false;
  if (frame[0] != HEAD0 || frame[1] != HEAD1 || frame[len - 2] != HEAD0 || frame[len - 1] != TAIL1)
    return false;
  if (static_cast<size_t>(frame[OFF_LEN]) + 9 != len)
    return false;
  const uint16_t ck = static_cast<uint16_t>((frame[len - 4] << 8) | frame[len - 3]);
  return ck == checksum(frame, len);
}

void seal(uint8_t *frame, size_t len) {
  frame[0] = HEAD0;
  frame[1] = HEAD1;
  frame[OFF_LEN] = static_cast<uint8_t>(len - 9);
  const uint16_t ck = checksum(frame, len);
  frame[len - 4] = static_cast<uint8_t>(ck >> 8);
  frame[len - 3] = static_cast<uint8_t>(ck & 0xFF);
  frame[len - 2] = HEAD0;
  frame[len - 1] = TAIL1;
}

size_t stuff(const uint8_t *frame, size_t len, uint8_t *out, size_t out_cap) {
  size_t o = 0;
  for (size_t i = 0; i < len; i++) {
    const bool interior = i >= 2 && i < len - 2;
    const size_t need = (interior && frame[i] == HEAD0) ? 2 : 1;
    if (o + need > out_cap)
      return 0;
    out[o++] = frame[i];
    if (need == 2)
      out[o++] = frame[i];
  }
  return o;
}

// ---- Receiver -------------------------------------------------------------------------------
void FrameAssembler::reset() {
  this->len_ = 0;
  this->expected_ = 0;
  this->pending_f4_ = false;
  this->synced_ = false;
  this->got_f4_ = false;
}

void FrameAssembler::restart_with_header_() {
  this->buf_[0] = HEAD0;
  this->buf_[1] = HEAD1;
  this->len_ = 2;
  this->expected_ = 0;
  this->pending_f4_ = false;
  this->synced_ = true;
  this->got_f4_ = false;
}

bool FrameAssembler::feed(uint8_t b) {
  if (!this->synced_) {
    // Hunting for F4 F5.
    if (this->got_f4_ && b == HEAD1) {
      this->restart_with_header_();
      return false;
    }
    this->got_f4_ = (b == HEAD0);
    return false;
  }

  if (this->pending_f4_) {
    this->pending_f4_ = false;
    if (b == HEAD0) {
      // Escaped F4 data byte.
      if (this->len_ >= MAX_FRAME) {
        this->framing_errors_++;
        this->reset();
        return false;
      }
      this->buf_[this->len_++] = HEAD0;
    } else if (b == TAIL1) {
      // End tag.
      if (this->len_ + 2 > MAX_FRAME) {
        this->framing_errors_++;
        this->reset();
        return false;
      }
      this->buf_[this->len_++] = HEAD0;
      this->buf_[this->len_++] = TAIL1;
      const size_t n = this->len_;
      const bool length_ok = this->expected_ != 0 && n == this->expected_;
      this->synced_ = false;
      this->got_f4_ = false;
      if (!length_ok) {
        this->framing_errors_++;
        return false;
      }
      if (!frame_valid(this->buf_, n)) {
        this->checksum_errors_++;
        return false;
      }
      return true;
    } else if (b == HEAD1) {
      // A new frame started in the middle of this one: resynchronise on it.
      this->framing_errors_++;
      this->restart_with_header_();
      return false;
    } else {
      this->framing_errors_++;
      this->reset();
      return false;
    }
  } else if (b == HEAD0) {
    this->pending_f4_ = true;
    return false;
  } else {
    if (this->len_ >= MAX_FRAME) {
      this->framing_errors_++;
      this->reset();
      return false;
    }
    this->buf_[this->len_++] = b;
  }

  if (this->len_ == OFF_LEN + 1) {
    this->expected_ = static_cast<size_t>(this->buf_[OFF_LEN]) + 9;
    if (this->expected_ > MAX_FRAME) {
      this->framing_errors_++;
      this->reset();
      return false;
    }
  }
  if (this->expected_ != 0 && this->len_ > this->expected_) {
    this->framing_errors_++;
    this->reset();
  }
  return false;
}

// ---- Module -> AC frames ---------------------------------------------------------------------
static size_t finish_(uint8_t *f, size_t n, uint8_t *out, size_t cap) {
  if (cap < n)
    return 0;
  seal(f, n);
  std::memcpy(out, f, n);
  return n;
}

size_t build_devtype_request(uint8_t *out, size_t cap) {
  uint8_t f[20] = {0xF4, 0xF5, 0x00, 0x40, 0x0B, 0x00, 0x00, 0x00, 0x00, 0xFE,
                   0x01, 0x00, 0x00, CLASS_DEVTYPE, 0x04, 0x00, 0, 0, 0, 0};
  return finish_(f, sizeof(f), out, cap);
}

size_t build_version_request(uint8_t t_hi, uint8_t t_lo, uint8_t *out, size_t cap) {
  uint8_t f[20] = {0xF4, 0xF5, 0x00, 0x40, 0x0B, 0x00, 0x00, t_hi, t_lo, 0xFE,
                   0x01, 0x00, 0x00, CLASS_VERSION, 0x01, 0x00, 0, 0, 0, 0};
  return finish_(f, sizeof(f), out, cap);
}

size_t build_heartbeat(uint8_t t_hi, uint8_t t_lo, bool heard_ac, uint8_t *out, size_t cap) {
  uint8_t f[28] = {0xF4, 0xF5, 0x00, 0x40, 0x13, 0x00, 0x00, t_hi, t_lo, 0xFE, 0x01, 0x00, 0x00, CLASS_LINK,
                   0x00, 0x00, 0xB0, 0x80, 0x20, 0x00, 0x00, 0x00, 0x40, 0x00, 0, 0, 0, 0};
  // Byte 16 bit 6 set = "have not heard the AC's heartbeat yet".
  f[16] = heard_ac ? 0xB0 : 0xF0;
  return finish_(f, sizeof(f), out, cap);
}

size_t build_status_request(uint8_t t_hi, uint8_t t_lo, uint8_t sub, uint8_t *out, size_t cap) {
  uint8_t f[21] = {0xF4, 0xF5, 0x00, 0x40, 0x0C, 0x00, 0x00, t_hi, t_lo, 0xFE, 0x01,
                   0x00, 0x00, CLASS_STATUS, sub, 0x00, 0x00, 0, 0, 0, 0};
  return finish_(f, sizeof(f), out, cap);
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
    f[36] = onoff_(c.display, 0xC0, 0x40);
  if (c.has_heat8)
    f[37] |= onoff_(c.heat8, 0x03, 0x01);
  seal(f, COMMAND_LEN);
}

// ---- Status ----------------------------------------------------------------------------------
static inline int16_t s8_(uint8_t v) { return static_cast<int16_t>(static_cast<int8_t>(v)); }

bool parse_status(const uint8_t *f, size_t len, Status *s) {
  if (s == nullptr || !frame_valid(f, len) || len < STATUS_MIN_LEN)
    return false;
  if (f[OFF_DIR] != DIR_FROM_AC || f[OFF_CLASS] != CLASS_STATUS || f[OFF_SUB] == SUB_PRODUCT_TYPE)
    return false;
  Status r;
  r.frame_len = len;

  const uint8_t fan = f[16];
  if (fan <= 0x01) {
    r.fan = Fan::AUTO;
  } else if (fan == 0x02) {
    r.fan = Fan::AUTO;
    r.fan_quiet_step = true;
  } else {
    int idx = (static_cast<int>(fan) - 0x08) / 2;
    if (idx < 1)
      idx = 1;
    if (idx > 5)
      idx = 5;
    r.fan = static_cast<Fan>(idx);
  }
  r.sleep = static_cast<uint8_t>(f[17] >> 1);
  r.power = (f[18] & 0x0C) != 0;
  const uint8_t m = (f[18] >> 4) & 0x07;
  r.mode = m >= 4 ? Mode::AUTO : static_cast<Mode>(m);
  r.setpoint = f[19];
  r.room_temp = s8_(f[20]);
  r.indoor_coil_temp = s8_(f[21]);
  r.humidity_setting = f[22];
  r.humidity = f[23];
  r.has_humidity = f[23] > 0 && f[23] <= 100;
  r.fahrenheit = (f[26] & 0x02) != 0;
  r.vswing = (f[35] & 0x80) != 0;
  r.hswing = (f[35] & 0x40) != 0;
  r.aux_heater = (f[35] & 0x10) != 0;
  r.eco = (f[35] & 0x04) != 0;
  r.turbo = (f[35] & 0x02) != 0;
  r.quiet = (f[36] & 0x04) != 0;
  r.display = (f[37] & 0x80) != 0;
  r.faults.indoor = f[39];
  r.faults.module = f[40];

  // Bytes before the checksum (len - 4) are payload.
  const size_t payload_end = len - 4;
  if (payload_end > 45) {
    r.has_outdoor = true;
    r.compressor_target_hz = f[41];
    r.compressor_hz = f[42];
    r.outdoor_temp = s8_(f[44]);
    r.outdoor_coil_temp = s8_(f[45]);
    r.outdoor_valid = !(f[44] == 0xEC && f[45] == 0xEC);
  }
  if (payload_end > 66) {
    r.faults.outdoor = f[64];
    r.faults.protect = static_cast<uint8_t>(f[66] & 0x7F);
    r.heat8 = (f[66] & 0x80) != 0;
  }
  if (payload_end > 77 && (f[77] & 0x01))
    r.heat8 = true;
  if (len >= 150) {
    r.has_electrical = true;
    r.voltage = f[50];
    r.power_raw_hi = f[55];
    r.power_raw_lo = f[56];
  }
  *s = r;
  return true;
}

bool parse_capabilities(const uint8_t *f, size_t len, Capabilities *c) {
  if (c == nullptr || !frame_valid(f, len) || len < 45)
    return false;
  if (f[OFF_DIR] != DIR_FROM_AC || f[OFF_CLASS] != CLASS_STATUS || f[OFF_SUB] != SUB_PRODUCT_TYPE)
    return false;
  Capabilities r;
  r.valid = true;
  r.cool_heat = (f[18] & 0x80) != 0;
  r.power_save = (f[23] & 0x40) != 0;
  r.purify = (f[23] & 0x08) != 0;
  r.fan_mute = (f[24] & 0x40) != 0;
  r.infinite_fan = (f[25] & 0x08) != 0;
  r.heat8 = (f[26] & 0x80) != 0 || (len > 43 && (f[39] & 0x04) != 0);
  r.humidity = (f[32] & 0x01) != 0;
  *c = r;
  return true;
}

bool parse_devtype(const uint8_t *f, size_t len, uint8_t *t_hi, uint8_t *t_lo) {
  if (!frame_valid(f, len) || len < 22)
    return false;
  if (f[OFF_DIR] != DIR_FROM_AC || f[OFF_CLASS] != CLASS_DEVTYPE)
    return false;
  if (f[16] == 0 && f[17] == 0)
    return false;
  *t_hi = f[16];
  *t_lo = f[17];
  return true;
}

// ---- Faults ----------------------------------------------------------------------------------
struct BitName {
  uint8_t mask;
  const char *name;
};

static void append_bits_(std::string &out, uint8_t v, const BitName *names, size_t n, const char *prefix) {
  uint8_t known = 0;
  for (size_t i = 0; i < n; i++) {
    known |= names[i].mask;
    if (v & names[i].mask) {
      if (!out.empty())
        out += ", ";
      out += names[i].name;
    }
  }
  const uint8_t unknown = static_cast<uint8_t>(v & ~known);
  if (unknown) {
    char buf[24];
    static const char HEX[] = "0123456789ABCDEF";
    size_t p = 0;
    for (const char *q = prefix; *q && p < sizeof(buf) - 4; q++)
      buf[p++] = *q;
    buf[p++] = HEX[unknown >> 4];
    buf[p++] = HEX[unknown & 0x0F];
    buf[p] = 0;
    if (!out.empty())
      out += ", ";
    out += buf;
  }
}

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

std::string describe_faults(const Faults &f) {
  static const BitName INDOOR[] = {
      {0x80, "Indoor temperature sensor"}, {0x40, "Indoor coil sensor"}, {0x20, "Humidity sensor"},
      {0x10, "Water tank full"},           {0x08, "Indoor fan motor"},   {0x04, "Grille protection"},
      {0x02, "Zero-cross detection"},      {0x01, "Indoor/outdoor communication"},
  };
  static const BitName MODULE[] = {
      {0x80, "Display board"}, {0x40, "Keypad"}, {0x20, "Wi-Fi module"}, {0x10, "Electrical"}, {0x08, "Indoor EEPROM"},
  };
  static const BitName OUTDOOR[] = {
      {0x40, "Outdoor EEPROM"},
      {0x20, "Outdoor coil sensor"},
      {0x10, "Discharge sensor"},
      {0x08, "Outdoor temperature sensor"},
  };
  static const BitName PROTECT[] = {
      {0x10, "Overheat/overcool protection"},
      {0x02, "High discharge temperature"},
  };
  std::string out;
  append_bits_(out, f.indoor, INDOOR, sizeof(INDOOR) / sizeof(INDOOR[0]), "B39:0x");
  append_bits_(out, f.module, MODULE, sizeof(MODULE) / sizeof(MODULE[0]), "B40:0x");
  append_bits_(out, f.outdoor, OUTDOOR, sizeof(OUTDOOR) / sizeof(OUTDOOR[0]), "B64:0x");
  append_bits_(out, f.protect, PROTECT, sizeof(PROTECT) / sizeof(PROTECT[0]), "B66:0x");
  return out;
}

std::string hex_bytes(const uint8_t *f, size_t len, size_t off, size_t n) {
  static const char HEX[] = "0123456789ABCDEF";
  std::string out;
  if (f == nullptr || off >= len)
    return out;
  const size_t end = (n > len - off) ? len : off + n;
  out.reserve((end - off) * 3);
  for (size_t i = off; i < end; i++) {
    if (i != off)
      out += ' ';
    out += HEX[f[i] >> 4];
    out += HEX[f[i] & 0x0F];
  }
  return out;
}

float power_watts(const Status &s) {
  if (!s.has_electrical)
    return -1.0f;
  if (s.frame_len >= 160) {
    const float raw = s.power_raw_hi;
    return 4.15f * raw * raw;
  }
  return static_cast<float>((s.power_raw_hi << 8) | s.power_raw_lo);
}

}  // namespace proto
}  // namespace hisense_ac
}  // namespace esphome

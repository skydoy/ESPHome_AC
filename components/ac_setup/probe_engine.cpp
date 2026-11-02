// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#include "probe_engine.h"

#include <cstdio>
#include <cstring>

namespace esphome {
namespace ac_setup {

namespace proto = hisense_ac::proto;

const UartProfile PROFILES[NUM_PROFILES] = {
    {"9600-8N1", 9600, 'N'},  {"9600-8E1", 9600, 'E'}, {"4800-8N1", 4800, 'N'},
    {"19200-8N1", 19200, 'N'}, {"2400-8E1", 2400, 'E'},
};

const ProbeDriver HISENSE_XM = {"hisense_xm", "Hisense / Hisense-OEM (XM F4F5 bus)", "rs485", "hisense"};

// ---- Allow-list ---------------------------------------------------------------------------
bool tx_allowed(const uint8_t *wire, size_t len) {
  if (wire == nullptr || len < 20 || len > proto::MAX_WIRE)
    return false;
  proto::FrameAssembler fa;
  size_t frames = 0;
  size_t end = 0;
  for (size_t i = 0; i < len; i++) {
    if (fa.feed(wire[i])) {
      frames++;
      end = i + 1;
    }
  }
  // Exactly one complete, checksum-valid frame that spans the whole buffer.
  if (frames != 1 || end != len || wire[0] != proto::HEAD0 || wire[1] != proto::HEAD1)
    return false;
  const uint8_t *f = fa.frame();
  const size_t n = fa.size();
  if (n < 20 || f[proto::OFF_DIR] != proto::DIR_TO_AC)
    return false;
  const uint8_t cls = f[proto::OFF_CLASS];
  if (cls == proto::CLASS_DEVTYPE || cls == proto::CLASS_VERSION)
    return true;
  if (cls == proto::CLASS_STATUS && f[proto::OFF_SUB] == 0x00)
    return true;
  return false;
}

// ---- Helpers ------------------------------------------------------------------------------
std::string hex_spaced(const uint8_t *d, size_t n) {
  static const char *H = "0123456789ABCDEF";
  std::string s;
  s.reserve(n * 3);
  for (size_t i = 0; i < n; i++) {
    if (i)
      s += ' ';
    s += H[d[i] >> 4];
    s += H[d[i] & 0x0F];
  }
  return s;
}

static std::string hex_compact(const uint8_t *d, size_t n) {
  static const char *H = "0123456789ABCDEF";
  std::string s;
  s.reserve(n * 2);
  for (size_t i = 0; i < n; i++) {
    s += H[d[i] >> 4];
    s += H[d[i] & 0x0F];
  }
  return s;
}

static std::string base64(const uint8_t *d, size_t n) {
  static const char *B = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string s;
  s.reserve((n + 2) / 3 * 4);
  for (size_t i = 0; i < n; i += 3) {
    const uint32_t v = (uint32_t(d[i]) << 16) | (i + 1 < n ? uint32_t(d[i + 1]) << 8 : 0) | (i + 2 < n ? d[i + 2] : 0);
    s += B[(v >> 18) & 63];
    s += B[(v >> 12) & 63];
    s += i + 1 < n ? B[(v >> 6) & 63] : '=';
    s += i + 2 < n ? B[v & 63] : '=';
  }
  return s;
}

std::string json_escape(const std::string &s) {
  std::string o;
  o.reserve(s.size() + 2);
  for (char c : s) {
    switch (c) {
      case '"':
        o += "\\\"";
        break;
      case '\\':
        o += "\\\\";
        break;
      case '\n':
        o += "\\n";
        break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          char b[8];
          std::snprintf(b, sizeof(b), "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(c)));
          o += b;
        } else {
          o += c;
        }
    }
  }
  return o;
}

std::string url_encode(const std::string &s) {
  static const char *H = "0123456789ABCDEF";
  std::string o;
  o.reserve(s.size() * 3);
  for (char ch : s) {
    const unsigned char c = static_cast<unsigned char>(ch);
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' ||
        c == '.' || c == '~') {
      o += static_cast<char>(c);
    } else {
      o += '%';
      o += H[c >> 4];
      o += H[c & 0x0F];
    }
  }
  return o;
}

size_t unstuff(const uint8_t *wire, size_t n, uint8_t *out, size_t cap) {
  if (n < 4 || wire[0] != proto::HEAD0 || wire[1] != proto::HEAD1)
    return 0;
  size_t o = 0;
  for (size_t i = 0; i < n && o < cap; i++) {
    out[o++] = wire[i];
    // Interior F4 F4 is one escaped F4. The header (0,1) and the F4 FB tail are not escaped.
    if (i >= 2 && wire[i] == proto::HEAD0 && i + 1 < n && wire[i + 1] == proto::HEAD0)
      i++;
  }
  return o;
}

static std::string fmt_u(uint32_t v) {
  char b[12];
  std::snprintf(b, sizeof(b), "%u", static_cast<unsigned>(v));
  return b;
}

static std::string fmt_cls(uint8_t c) {
  char b[8];
  std::snprintf(b, sizeof(b), "0x%02X", c);
  return b;
}

static std::string fmt_t(uint32_t ms) {
  char b[24];
  std::snprintf(b, sizeof(b), "+%u.%03us", static_cast<unsigned>(ms / 1000), static_cast<unsigned>(ms % 1000));
  return b;
}

const char *chunk_dir_str(ChunkDir d) {
  switch (d) {
    case ChunkDir::TX:
      return "tx";
    case ChunkDir::RX:
      return "rx";
    case ChunkDir::ECHO:
      return "echo";
  }
  return "?";
}

// ---- Capture ------------------------------------------------------------------------------
static uint16_t rd16(const uint8_t *p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
static void wr16(uint8_t *p, uint16_t v) {
  p[0] = static_cast<uint8_t>(v);
  p[1] = static_cast<uint8_t>(v >> 8);
}
static uint32_t rd32(const uint8_t *p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
static void wr32(uint8_t *p, uint32_t v) {
  for (int i = 0; i < 4; i++)
    p[i] = static_cast<uint8_t>(v >> (8 * i));
}

void Capture::clear() {
  this->used_ = 0;
  this->count_ = 0;
  this->last_off_ = 0;
  this->last_t_ = 0;
  this->dropped_ = 0;
}

void Capture::drop_oldest_() {
  if (this->count_ == 0)
    return;
  const size_t sz = HDR + rd16(this->buf_ + 6);
  std::memmove(this->buf_, this->buf_ + sz, this->used_ - sz);
  this->used_ -= sz;
  this->count_--;
  this->dropped_++;
  this->last_off_ = this->last_off_ >= sz ? this->last_off_ - sz : 0;
}

void Capture::add(uint32_t t_ms, ChunkDir dir, uint8_t profile, const uint8_t *data, size_t len, uint32_t gap_ms) {
  while (len > 0) {
    bool extend = false;
    if (this->count_ > 0) {
      const uint8_t *h = this->buf_ + this->last_off_;
      extend = h[4] == static_cast<uint8_t>(dir) && h[5] == profile && t_ms - this->last_t_ <= gap_ms &&
               rd16(h + 6) < 512;
    }
    if (!extend) {
      while (this->used_ + HDR + 1 > CAP)
        this->drop_oldest_();
      this->last_off_ = this->used_;
      uint8_t *h = this->buf_ + this->used_;
      wr32(h, t_ms);
      h[4] = static_cast<uint8_t>(dir);
      h[5] = profile;
      wr16(h + 6, 0);
      this->used_ += HDR;
      this->count_++;
    }
    // Room for at least one byte: drop old chunks (never the current one).
    while (this->used_ + 1 > CAP && this->count_ > 1)
      this->drop_oldest_();
    size_t room = CAP - this->used_;
    uint8_t *h = this->buf_ + this->last_off_;
    size_t room_chunk = 512 - rd16(h + 6);
    size_t n = len < room ? len : room;
    n = n < room_chunk ? n : room_chunk;
    if (n == 0) {
      // Current chunk is full or the arena is exhausted by it: force a new chunk.
      if (this->count_ == 1 && room == 0)
        return;  // cannot happen with a 512-byte chunk cap, kept as a guard
      this->last_t_ = t_ms - gap_ms - 1;
      continue;
    }
    std::memcpy(this->buf_ + this->used_, data, n);
    this->used_ += n;
    wr16(h + 6, static_cast<uint16_t>(rd16(h + 6) + n));
    data += n;
    len -= n;
    this->last_t_ = t_ms;
  }
}

bool Capture::get(size_t i, ChunkView *out) const {
  if (i >= this->count_)
    return false;
  size_t off = 0;
  for (size_t k = 0; k < i; k++)
    off += HDR + rd16(this->buf_ + off + 6);
  const uint8_t *h = this->buf_ + off;
  out->t_ms = rd32(h);
  out->dir = static_cast<ChunkDir>(h[4]);
  out->profile = h[5];
  out->len = rd16(h + 6);
  out->data = h + HDR;
  return true;
}

// ---- Engine -------------------------------------------------------------------------------
const char *state_str(State s) {
  switch (s) {
    case State::IDLE: return "idle";
    case State::LISTEN: return "listen";
    case State::IDENTIFY: return "identify";
    case State::SCAN: return "scan";
    case State::CANDIDATE: return "candidate";
    case State::VALIDATE: return "validate";
    case State::VALIDATE_OK: return "validate_ok";
    case State::VALIDATE_FAILED: return "validate_failed";
    case State::INSTALL: return "install";
    case State::INSTALL_FAILED: return "install_failed";
    case State::NO_INTERNET: return "no_internet";
    case State::REBOOT: return "reboot";
    case State::UNKNOWN: return "unknown";
    case State::NO_REPLY: return "no_reply";
    case State::BUS_BUSY: return "bus_busy";
  }
  return "?";
}

const char *step_str(Step s) {
  switch (s) {
    case Step::NONE: return "";
    case Step::P1_LISTEN: return "passive listen";
    case Step::P2_DEVTYPE: return "0x0A request";
    case Step::P3_STATUS: return "0x66/00 request";
    case Step::P4_VERSION: return "0x07 request";
    case Step::P5_SCAN: return "passive scan";
    case Step::VALIDATE_STATUS: return "0x66/00 validation";
  }
  return "?";
}

bool Engine::busy() const {
  switch (this->state_) {
    case State::LISTEN:
    case State::IDENTIFY:
    case State::SCAN:
    case State::VALIDATE:
    case State::INSTALL:
    case State::REBOOT:
      return true;
    default:
      return false;
  }
}

uint32_t Engine::char_ms_() const {
  const UartProfile &p = PROFILES[this->profile_];
  const uint32_t bits = p.parity == 'N' ? 10 : 11;
  return (bits * 1000 + p.baud - 1) / p.baud;
}

uint32_t Engine::start(uint32_t now, bool passive_only) {
  if (this->busy())
    return 0;
  this->cap_.clear();
  this->fa_ = proto::FrameAssembler();
  this->probe_id_ = this->next_probe_id_++;
  this->t0_ = now;
  this->passive_only_ = passive_only;
  this->frames_sent_ = 0;
  this->refused_ = 0;
  this->detected_ = false;
  this->via_status_ = false;
  this->bus_busy_ = false;
  this->echo_seen_ = false;
  this->garbage_after_tx_ = false;
  this->foreign_frames_ = 0;
  this->t_hi_ = this->t_lo_ = 0;
  this->has_status_ = false;
  this->status_ = proto::Status{};
  this->has_version_ = false;
  this->version_text_.clear();
  this->n_evidence_ = 0;
  this->any_rx_ = false;
  this->any_tx_ = false;
  this->waiting_reply_ = false;
  this->install_pct_ = 0;
  this->install_msg_.clear();
  this->note_ = "";
  for (auto &s : this->stats_)
    s = ProfileStats{};
  this->profile_ = 0;
  this->bus_->configure(PROFILES[0]);
  // P0: flush whatever was buffered before the probe.
  uint8_t junk[64];
  while (this->bus_->read(junk, sizeof(junk)) > 0) {
  }
  this->set_state_(now, State::LISTEN, Step::P1_LISTEN);
  return this->probe_id_;
}

void Engine::cancel(uint32_t now) {
  if (this->state_ == State::INSTALL || this->state_ == State::REBOOT)
    return;  // an OTA in progress cannot be cancelled safely from here
  this->waiting_reply_ = false;
  if (this->profile_ != 0) {
    this->profile_ = 0;
    this->bus_->configure(PROFILES[0]);
  }
  this->set_state_(now, State::IDLE, Step::NONE);
}

bool Engine::confirm(uint32_t now, uint32_t probe_id, const char *driver) {
  if (this->state_ != State::CANDIDATE && this->state_ != State::VALIDATE_FAILED)
    return false;
  if (!this->detected_ || probe_id != this->probe_id_ || driver == nullptr ||
      std::strcmp(driver, HISENSE_XM.id) != 0)
    return false;
  this->set_state_(now, State::VALIDATE, Step::VALIDATE_STATUS);
  this->begin_step_(now, Step::VALIDATE_STATUS);
  return true;
}

void Engine::set_install_state(State s, uint8_t pct, const char *msg) {
  this->install_pct_ = pct;
  this->install_msg_ = msg != nullptr ? msg : "";
  if (s != this->state_) {
    this->state_ = s;
    this->step_ = Step::NONE;
  }
  this->seq_++;
}

void Engine::set_state_(uint32_t now, State s, Step st) {
  this->state_ = s;
  this->step_ = st;
  this->state_since_ = now;
  this->step_start_ = now;
  this->seq_++;
}

void Engine::begin_step_(uint32_t now, Step st) {
  this->step_ = st;
  this->step_start_ = now;
  this->attempt_ = 0;
  this->waiting_reply_ = false;
  this->next_send_at_ = now;
  switch (st) {
    case Step::P2_DEVTYPE:
      this->attempts_max_ = P2_ATTEMPTS;
      break;
    case Step::P3_STATUS:
      this->attempts_max_ = P3_ATTEMPTS;
      break;
    case Step::P4_VERSION:
      this->attempts_max_ = P4_ATTEMPTS;
      break;
    case Step::VALIDATE_STATUS:
      this->attempts_max_ = VALIDATE_ATTEMPTS;
      break;
    default:
      this->attempts_max_ = 0;
  }
  this->seq_++;
}

bool Engine::send_(uint32_t now, const uint8_t *wire, size_t len) {
  if (!tx_allowed(wire, len)) {
    this->refused_++;
    return false;
  }
  if (this->profile_ != 0) {
    this->refused_++;  // active frames only on the documented profile
    return false;
  }
  if (this->frames_sent_ >= MAX_FRAMES)
    return false;
  if (this->any_tx_ && now - this->last_tx_ < MIN_TX_SPACING_MS)
    return false;
  this->bus_->write(wire, len);
  this->cap_.add(now - this->t0_, ChunkDir::TX, this->profile_, wire, len, 0);
  this->frames_sent_++;
  this->any_tx_ = true;
  this->last_tx_ = now;
  // RX bytes until our frame has left the wire (+ margin) are classified as echo.
  this->tx_busy_until_ = now + static_cast<uint32_t>(len) * this->char_ms_() + 10;
  return true;
}

void Engine::send_request_(uint32_t now) {
  if (static_cast<int32_t>(now - this->next_send_at_) < 0)
    return;
  // Each frame goes out only after >= 50 ms of bus idle.
  if (this->any_rx_ && now - this->last_rx_ < IDLE_PER_FRAME_MS) {
    if (now - this->step_start_ > IDLE_WAIT_MAX_MS) {
      if (this->state_ == State::VALIDATE) {
        this->step_failed_(now);
      } else {
        this->bus_busy_ = true;
        this->finish_identify_(now);
      }
    }
    return;
  }
  uint8_t f[32];
  size_t n = 0;
  switch (this->step_) {
    case Step::P2_DEVTYPE:
      n = proto::build_devtype_request(f, sizeof(f));
      break;
    case Step::P3_STATUS:
      n = proto::build_status_request(0x00, 0x00, 0x00, f, sizeof(f));
      break;
    case Step::P4_VERSION:
      n = proto::build_version_request(this->t_hi_, this->t_lo_, f, sizeof(f));
      break;
    case Step::VALIDATE_STATUS:
      n = proto::build_status_request(this->t_hi_, this->t_lo_, 0x00, f, sizeof(f));
      break;
    default:
      return;
  }
  uint8_t wire[64];
  const size_t wn = proto::stuff(f, n, wire, sizeof(wire));
  if (wn == 0 || !this->send_(now, wire, wn)) {
    if (this->frames_sent_ >= MAX_FRAMES || this->refused_ > 0)
      this->step_failed_(now);  // budget exhausted or refused: never retry a refused frame
    return;
  }
  this->last_tx_class_ = f[proto::OFF_CLASS];
  this->attempt_++;
  this->waiting_reply_ = true;
  this->deadline_ = now + REPLY_TIMEOUT_MS;
  this->rx_after_tx_ = 0;
  this->frames_this_window_ = 0;
  this->seq_++;
}

void Engine::step_failed_(uint32_t now) {
  this->waiting_reply_ = false;
  switch (this->step_) {
    case Step::P2_DEVTYPE:
      this->begin_step_(now, Step::P3_STATUS);
      break;
    case Step::P3_STATUS:
      this->finish_identify_(now);
      break;
    case Step::P4_VERSION:
      this->finish_identify_(now);
      break;
    case Step::VALIDATE_STATUS:
      this->set_state_(now, State::VALIDATE_FAILED, Step::NONE);
      break;
    default:
      break;
  }
}

void Engine::finish_identify_(uint32_t now) {
  this->waiting_reply_ = false;
  if (this->detected_ && !this->bus_busy_) {
    this->set_state_(now, State::CANDIDATE, Step::NONE);
    return;
  }
  // Not detected: passive scan of the other profiles for the raw capture, then finish.
  this->profile_ = 1;
  this->bus_->configure(PROFILES[this->profile_]);
  this->set_state_(now, State::SCAN, Step::P5_SCAN);
}

void Engine::finish_probe_(uint32_t now) {
  if (this->profile_ != 0) {
    this->profile_ = 0;
    this->bus_->configure(PROFILES[0]);
  }
  uint32_t rx = 0;
  for (const auto &s : this->stats_)
    rx += s.rx_bytes;
  if (this->bus_busy_)
    this->set_state_(now, State::BUS_BUSY, Step::NONE);
  else if (rx > 0)
    this->set_state_(now, State::UNKNOWN, Step::NONE);
  else
    this->set_state_(now, State::NO_REPLY, Step::NONE);
}

void Engine::pump_rx_(uint32_t now) {
  uint8_t buf[128];
  for (;;) {
    const size_t n = this->bus_->read(buf, sizeof(buf));
    if (n == 0)
      break;
    // Bytes are captured and decoded only while a probe or a validation read runs.
    const bool active_state = this->state_ == State::LISTEN || this->state_ == State::IDENTIFY ||
                              this->state_ == State::SCAN || this->state_ == State::VALIDATE;
    const bool echo = this->any_tx_ && static_cast<int32_t>(now - this->tx_busy_until_) <= 0;
    if (active_state) {
      const uint32_t gap = this->char_ms_() * 4 > 20 ? this->char_ms_() * 4 : 20;
      this->cap_.add(now - this->t0_, echo ? ChunkDir::ECHO : ChunkDir::RX, this->profile_, buf, n, gap);
      if (echo) {
        this->stats_[this->profile_].echo_bytes += n;
        this->echo_seen_ = true;
      } else {
        this->stats_[this->profile_].rx_bytes += n;
        if (this->waiting_reply_)
          this->rx_after_tx_ += n;
      }
    }
    this->any_rx_ = true;
    this->last_rx_ = now;
    if (this->profile_ != 0 || !active_state)
      continue;
    for (size_t i = 0; i < n; i++) {
      if (this->fa_.feed(buf[i]))
        this->on_frame_(now, this->fa_.frame(), this->fa_.size());
    }
  }
}

void Engine::on_frame_(uint32_t now, const uint8_t *f, size_t n) {
  this->stats_[this->profile_].frames_ok++;
  const uint8_t dir = f[proto::OFF_DIR];
  const uint8_t cls = n > proto::OFF_CLASS ? f[proto::OFF_CLASS] : 0;
  if (this->state_ == State::LISTEN) {
    // Any valid frame we did not ask for means another master (or the stock module) is active.
    this->foreign_frames_++;
    return;
  }
  if (dir == proto::DIR_TO_AC) {
    if (this->waiting_reply_ && cls == this->last_tx_class_) {
      // Our own request heard back (auto-direction transceiver): echo, not a reply.
      this->echo_seen_ = true;
      const uint32_t wn = static_cast<uint32_t>(n);
      this->rx_after_tx_ = this->rx_after_tx_ > wn ? this->rx_after_tx_ - wn : 0;
    } else {
      this->foreign_frames_++;
    }
    return;
  }
  if (dir != proto::DIR_FROM_AC)
    return;
  if (!this->waiting_reply_) {
    this->foreign_frames_++;
    return;
  }
  this->frames_this_window_++;
  bool ok = false;
  switch (this->step_) {
    case Step::P2_DEVTYPE: {
      uint8_t hi = 0, lo = 0;
      if (proto::parse_devtype(f, n, &hi, &lo)) {
        ok = true;
        this->detected_ = true;
        this->t_hi_ = hi;
        this->t_lo_ = lo;
      }
      break;
    }
    case Step::P3_STATUS:
    case Step::VALIDATE_STATUS: {
      proto::Status st;
      if (proto::parse_status(f, n, &st)) {
        ok = true;
        this->status_ = st;
        this->has_status_ = true;
        if (this->step_ == Step::P3_STATUS) {
          this->detected_ = true;
          this->via_status_ = true;
          this->note_ = "no 0x0A reply (unit without handshake)";
        }
      }
      break;
    }
    case Step::P4_VERSION: {
      proto::Version v;
      if (proto::parse_version(f, n, &v)) {
        ok = true;
        this->has_version_ = true;
        this->version_text_ = v.text;
      }
      break;
    }
    default:
      break;
  }
  if (this->n_evidence_ < MAX_EVIDENCE) {
    Evidence &e = this->evidence_[this->n_evidence_++];
    e.tx_class = this->last_tx_class_;
    e.rx_class = cls;
    e.rx_len = static_cast<uint16_t>(n);
    e.valid = ok;
  }
  if (!ok)
    return;  // a valid frame of another class: keep waiting for ours until the timeout
  this->waiting_reply_ = false;
  switch (this->step_) {
    case Step::P2_DEVTYPE:
    case Step::P3_STATUS:
      this->begin_step_(now, Step::P4_VERSION);
      this->next_send_at_ = now + ATTEMPT_GAP_MS;
      break;
    case Step::P4_VERSION:
      this->finish_identify_(now);
      break;
    case Step::VALIDATE_STATUS:
      this->set_state_(now, State::VALIDATE_OK, Step::NONE);
      break;
    default:
      break;
  }
}

void Engine::tick(uint32_t now) {
  this->pump_rx_(now);
  switch (this->state_) {
    case State::LISTEN: {
      const uint32_t el = now - this->step_start_;
      if (el < P1_LISTEN_MS)
        break;
      if (this->foreign_frames_ > 0) {
        this->bus_busy_ = true;
        this->finish_identify_(now);
      } else if (this->passive_only_) {
        this->finish_identify_(now);
      } else if (!this->any_rx_ || now - this->last_rx_ >= IDLE_BEFORE_TX_MS) {
        this->state_ = State::IDENTIFY;
        this->begin_step_(now, Step::P2_DEVTYPE);
      } else if (el >= P1_LISTEN_MS + IDLE_WAIT_MAX_MS) {
        this->bus_busy_ = true;  // never got a 500 ms idle window: do not transmit
        this->finish_identify_(now);
      }
      break;
    }
    case State::IDENTIFY:
    case State::VALIDATE:
      if (this->waiting_reply_) {
        if (static_cast<int32_t>(now - this->deadline_) >= 0) {
          this->waiting_reply_ = false;
          if (this->rx_after_tx_ > 0 && this->frames_this_window_ == 0)
            this->garbage_after_tx_ = true;
          if (this->attempt_ >= this->attempts_max_) {
            this->step_failed_(now);
          } else {
            this->next_send_at_ = now + ATTEMPT_GAP_MS;
            this->seq_++;
          }
        }
      } else {
        this->send_request_(now);
      }
      break;
    case State::SCAN:
      if (now - this->step_start_ >= SCAN_MS) {
        if (this->profile_ + 1u < NUM_PROFILES) {
          this->profile_++;
          this->bus_->configure(PROFILES[this->profile_]);
          this->step_start_ = now;
          this->seq_++;
        } else {
          this->finish_probe_(now);
        }
      }
      break;
    case State::CANDIDATE:
    case State::VALIDATE_FAILED:
      if (now - this->state_since_ >= CANDIDATE_TIMEOUT_MS)
        this->set_state_(now, State::IDLE, Step::NONE);
      break;
    default:
      break;
  }
}

const char *Engine::outcome() const {
  if (this->detected_ && !this->bus_busy_)
    return "detected";
  switch (this->state_) {
    case State::BUS_BUSY: return "bus_busy";
    case State::UNKNOWN: return "unknown";
    case State::NO_REPLY: return "no_reply";
    case State::IDLE: return this->probe_id_ == 0 ? "none" : "cancelled";
    default: return "pending";
  }
}

bool Engine::hint_ab_swap() const { return !this->detected_ && this->garbage_after_tx_; }

// ---- Formatters ---------------------------------------------------------------------------
static const char *mode_str(proto::Mode m) {
  switch (m) {
    case proto::Mode::FAN: return "fan_only";
    case proto::Mode::HEAT: return "heat";
    case proto::Mode::COOL: return "cool";
    case proto::Mode::DRY: return "dry";
    case proto::Mode::AUTO: return "auto";
  }
  return "?";
}

static std::string type_str(uint8_t hi, uint8_t lo) { return hex_spaced(&hi, 1) + " " + hex_spaced(&lo, 1); }

std::string Engine::state_json() const {
  std::string s = "{\"seq\":" + fmt_u(this->seq_) + ",\"probe_id\":" + fmt_u(this->probe_id_) + ",\"state\":\"" +
                  state_str(this->state_) + "\",\"step\":\"" + step_str(this->step_) + "\",\"profile\":\"" +
                  PROFILES[this->profile_].name + "\"";
  if (this->attempts_max_ > 0 && this->step_ != Step::NONE && this->step_ != Step::P5_SCAN &&
      this->step_ != Step::P1_LISTEN)
    s += ",\"attempt\":" + fmt_u(this->attempt_) + ",\"of\":" + fmt_u(this->attempts_max_);
  s += ",\"outcome\":\"" + std::string(this->outcome()) + "\"";
  if (this->state_ == State::INSTALL || this->state_ == State::INSTALL_FAILED || this->state_ == State::NO_INTERNET ||
      this->state_ == State::REBOOT)
    s += ",\"pct\":" + fmt_u(this->install_pct_) + ",\"msg\":\"" + json_escape(this->install_msg_) + "\"";
  s += "}";
  return s;
}

std::string Engine::result_json(const char *fw, const char *board) const {
  std::string s = "{\"probe_id\":" + fmt_u(this->probe_id_) + ",\"state\":\"" + state_str(this->state_) +
                  "\",\"outcome\":\"" + this->outcome() + "\"";
  if (this->detected_) {
    s += ",\"driver\":\"" + std::string(HISENSE_XM.id) + "\",\"label\":\"" + json_escape(HISENSE_XM.label) +
         "\",\"identity\":{\"type\":\"" + type_str(this->t_hi_, this->t_lo_) + "\"}";
    s += ",\"confidence\":\"" +
         std::string(this->via_status_ ? "checksum-valid reply to status request"
                                       : "checksum-valid reply to ID request") +
         "\"";
  } else {
    s += ",\"driver\":null,\"identity\":null";
  }
  s += ",\"note\":\"" + json_escape(this->note_) + "\"";
  s += ",\"ac_version\":";
  s += this->has_version_ ? "\"" + json_escape(this->version_text_) + "\"" : std::string("null");
  s += ",\"status\":";
  if (this->has_status_) {
    const auto &st = this->status_;
    s += "{\"len\":" + fmt_u(static_cast<uint32_t>(st.frame_len)) + ",\"power\":" + (st.power ? "true" : "false") +
         ",\"mode\":\"" + mode_str(st.mode) + "\",\"setpoint\":" + fmt_u(st.setpoint) +
         ",\"room\":" + std::to_string(st.room_temp) + ",\"unit\":\"" + (st.fahrenheit ? "F" : "C") + "\"}";
  } else {
    s += "null";
  }
  s += ",\"evidence\":[";
  for (size_t i = 0; i < this->n_evidence_; i++) {
    const Evidence &e = this->evidence_[i];
    if (i)
      s += ",";
    s += "{\"profile\":\"" + std::string(PROFILES[0].name) + "\",\"tx_class\":\"" + fmt_cls(e.tx_class) +
         "\",\"rx_class\":\"" + fmt_cls(e.rx_class) + "\",\"rx_len\":" + fmt_u(e.rx_len) +
         ",\"valid\":" + (e.valid ? "true" : "false") + "}";
  }
  uint32_t rx = 0;
  for (const auto &st : this->stats_)
    rx += st.rx_bytes;
  s += "],\"diagnostics\":{\"echo_seen\":" + std::string(this->echo_seen_ ? "true" : "false") +
       ",\"rx_bytes\":" + fmt_u(rx) +
       ",\"uart_errors\":null,\"frame_checksum_errors\":" + fmt_u(this->fa_.checksum_errors()) +
       ",\"frame_framing_errors\":" + fmt_u(this->fa_.framing_errors()) +
       ",\"frames_sent\":" + fmt_u(this->frames_sent_) + ",\"foreign_frames\":" + fmt_u(this->foreign_frames_) +
       ",\"capture_chunks_dropped\":" + fmt_u(this->cap_.dropped_chunks()) + ",\"profiles\":[";
  for (size_t i = 0; i < NUM_PROFILES; i++) {
    if (i)
      s += ",";
    s += "{\"profile\":\"" + std::string(PROFILES[i].name) + "\",\"rx_bytes\":" + fmt_u(this->stats_[i].rx_bytes) +
         ",\"echo_bytes\":" + fmt_u(this->stats_[i].echo_bytes) +
         ",\"frames_ok\":" + fmt_u(this->stats_[i].frames_ok) + ",\"active\":" + (i == 0 ? "true" : "false") + "}";
  }
  s += "]},\"hints\":[";
  bool first = true;
  auto hint = [&](const char *h) {
    if (!first)
      s += ",";
    first = false;
    s += "\"" + std::string(h) + "\"";
  };
  if (this->hint_ab_swap())
    hint("ab_swap_likely");
  if (!this->detected_ && this->echo_seen_)
    hint("echo_only");
  if (!this->detected_ && !this->echo_seen_ && rx == 0 && this->state_ == State::NO_REPLY)
    hint("silent_bus");
  s += "],\"fw\":\"" + json_escape(fw) + "\",\"board\":\"" + json_escape(board) + "\"}";
  return s;
}

std::string Engine::capture_json() const {
  std::string s = "{\"format\":1,\"dropped\":" + fmt_u(this->cap_.dropped_chunks()) + ",\"chunks\":[";
  ChunkView v{};
  for (size_t i = 0; this->cap_.get(i, &v); i++) {
    if (i)
      s += ",";
    s += "{\"t\":" + fmt_u(v.t_ms) + ",\"dir\":\"" + chunk_dir_str(v.dir) + "\",\"profile\":\"" +
         PROFILES[v.profile < NUM_PROFILES ? v.profile : 0].name + "\",\"hex\":\"" + hex_compact(v.data, v.len) +
         "\",\"b64\":\"" + base64(v.data, v.len) + "\"}";
  }
  s += "]}";
  return s;
}

static void hex_rows(std::string &s, const char *prefix, const uint8_t *d, size_t n) {
  for (size_t off = 0; off < n; off += 16) {
    char b[16];
    std::snprintf(b, sizeof(b), "%s%04X  ", prefix, static_cast<unsigned>(off));
    s += b;
    s += hex_spaced(d + off, n - off < 16 ? n - off : 16);
    s += "\n";
  }
}

std::string Engine::capture_text(const char *fw, const char *board) const {
  std::string s = "ESPHome-AC setup capture\nfw=" + std::string(fw) + " board=" + board +
                  " outcome=" + this->outcome() + " probe_id=" + fmt_u(this->probe_id_);
  if (this->detected_)
    s += " type=" + type_str(this->t_hi_, this->t_lo_);
  if (this->hint_ab_swap())
    s += " hints=ab_swap_likely";
  s += "\n";
  for (size_t i = 0; i < NUM_PROFILES; i++)
    s += PROFILES[i].name + std::string(": rx=") + fmt_u(this->stats_[i].rx_bytes) + " echo=" +
         fmt_u(this->stats_[i].echo_bytes) + " frames=" + fmt_u(this->stats_[i].frames_ok) + "\n";
  s += "frame_checksum_errors=" + fmt_u(this->fa_.checksum_errors()) +
       " frame_framing_errors=" + fmt_u(this->fa_.framing_errors()) + "\n\n";
  ChunkView v{};
  for (size_t i = 0; this->cap_.get(i, &v); i++) {
    char head[80];
    const char *dir = v.dir == ChunkDir::TX ? "TX  " : v.dir == ChunkDir::RX ? "RX  " : "ECHO";
    std::snprintf(head, sizeof(head), "t=%s  %-9s  %s (%u bytes)", fmt_t(v.t_ms).c_str(),
                  PROFILES[v.profile < NUM_PROFILES ? v.profile : 0].name, dir, static_cast<unsigned>(v.len));
    s += head;
    uint8_t dec[proto::MAX_WIRE];
    const size_t dn = unstuff(v.data, v.len, dec, sizeof(dec));
    if (dn >= 9) {
      const bool ck = proto::frame_valid(dec, dn);
      s += "  [header ok | checksum ";
      s += ck ? "ok" : "BAD";
      char d[12];
      std::snprintf(d, sizeof(d), " | dir %02X]", dec[proto::OFF_DIR]);
      s += d;
    }
    s += "\n";
    hex_rows(s, "  wire ", v.data, v.len);
    if (dn > 0 && dn != v.len)
      hex_rows(s, "  dec  ", dec, dn);
  }
  return s;
}

std::string Engine::report_url(const char *fw, const char *board, size_t max_len) const {
  static const char *BASE = "https://github.com/skydoy/ESPHome_AC/issues/new?template=compatibility_report.yml";
  const std::string outcome = this->outcome();
  std::string url = BASE;
  url += "&title=" + url_encode("[Compatibility] setup probe: " + outcome + " on " + board);
  url += "&firmware=" + url_encode(fw);
  if (outcome != "detected")
    url += "&result=" + url_encode("Does not work");
  std::string notes = "Setup probe result: " + outcome + "\nBoard: " + board + "\nFirmware: " + fw + "\n";
  if (this->detected_)
    notes += "Detected: " + std::string(HISENSE_XM.label) + ", type " + type_str(this->t_hi_, this->t_lo_) + "\n";
  if (this->hint_ab_swap())
    notes += "Hint: ab_swap_likely\n";
  uint32_t rx = 0;
  for (const auto &st : this->stats_)
    rx += st.rx_bytes;
  notes += "Echo seen: " + std::string(this->echo_seen_ ? "yes" : "no") + ", RX bytes: " + fmt_u(rx) + "\n";
  notes += "Please attach the downloaded capture JSON for the full capture.\n\n";
  const std::string tail = "```\n";
  std::string hex = "```\n";
  size_t hex_raw = 0;
  const std::string prefix = url + "&notes=";
  size_t used = prefix.size() + url_encode(notes).size() + url_encode(hex).size() + url_encode(tail).size();
  ChunkView v{};
  bool truncated = false;
  for (size_t i = 0; this->cap_.get(i, &v); i++) {
    std::string line = fmt_t(v.t_ms) + " " + PROFILES[v.profile < NUM_PROFILES ? v.profile : 0].name + " " +
                       chunk_dir_str(v.dir) + " " + hex_spaced(v.data, v.len) + "\n";
    const size_t enc = url_encode(line).size();
    if (hex_raw + line.size() > 1536 || used + enc > max_len) {
      truncated = true;
      break;
    }
    hex += line;
    hex_raw += line.size();
    used += enc;
  }
  if (truncated) {
    const std::string more = "(truncated)\n";
    if (used + url_encode(more).size() <= max_len)
      hex += more;
  }
  hex += tail;
  return prefix + url_encode(notes + hex);
}

}  // namespace ac_setup
}  // namespace esphome

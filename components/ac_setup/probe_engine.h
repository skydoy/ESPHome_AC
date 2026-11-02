// Setup-mode probe engine: platform-independent, so the host tests in tests/ can drive it.
//
// It owns the probe state machine, the transmit allow-list, the raw-capture ring and the
// text / JSON / report-URL formatters. It talks to the bus only through the small Bus
// interface below; the ESPHome glue (ac_setup.cpp) maps that onto a uart::UARTComponent.
//
// Bus safety: the only frames this engine can put on the wire are built from the read-only
// requests in hisense_proto (0x0A device type, 0x66/00 status, 0x07 version), and every
// frame is checked against the allow-list again in send_() before it is written. The write
// builders (0x65 command, 0x1E heartbeat) live in components/hisense_ac and are not linked.
//
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "../hisense_proto/hisense_protocol.h"

namespace esphome {
namespace ac_setup {

struct UartProfile {
  const char *name;  // "9600-8N1"
  uint32_t baud;
  char parity;       // 'N' or 'E'
};

// Profile 0 is the only one active frames are ever sent on. The rest are passive-only
// diagnostics (spec 8.2 P5); they are not a claim about any brand's settings.
static constexpr size_t NUM_PROFILES = 5;
extern const UartProfile PROFILES[NUM_PROFILES];

class Bus {
 public:
  virtual ~Bus() = default;
  virtual void configure(const UartProfile &p) = 0;
  virtual size_t read(uint8_t *buf, size_t cap) = 0;
  virtual void write(const uint8_t *data, size_t len) = 0;
};

struct ProbeDriver {
  const char *id;
  const char *label;
  const char *phys;
  const char *image_key;
};
extern const ProbeDriver HISENSE_XM;

// Allow-list (spec 11): a frame may be sent only if it is a module->AC frame (dir 00) of
// class 0x0A, 0x66 with sub 0x00, or 0x07, with a valid checksum. `wire` is the stuffed
// frame as it would go on the bus.
bool tx_allowed(const uint8_t *wire, size_t len);

// ---- Raw capture ------------------------------------------------------------------------
enum class ChunkDir : uint8_t { TX = 0, RX = 1, ECHO = 2 };
const char *chunk_dir_str(ChunkDir d);

struct ChunkView {
  uint32_t t_ms;   // relative to probe start
  ChunkDir dir;
  uint8_t profile;
  const uint8_t *data;
  uint16_t len;
};

// A ring of chunks in a fixed 4 KiB byte arena. Each chunk is stored as an 8-byte header
// followed by its bytes; the oldest chunks are dropped when the arena is full.
class Capture {
 public:
  static constexpr size_t CAP = 4096;
  static constexpr size_t HDR = 8;
  void clear();
  // Appends bytes to the current chunk, or starts a new one when dir/profile changes or the
  // gap since the last byte exceeds gap_ms.
  void add(uint32_t t_ms, ChunkDir dir, uint8_t profile, const uint8_t *data, size_t len, uint32_t gap_ms);
  size_t count() const { return this->count_; }
  bool get(size_t i, ChunkView *out) const;  // 0 = oldest
  uint32_t dropped_chunks() const { return this->dropped_; }
  size_t used() const { return this->used_; }

 protected:
  void drop_oldest_();
  uint8_t buf_[CAP];
  size_t used_{0};
  size_t count_{0};
  size_t last_off_{0};  // offset of the last chunk's header
  uint32_t last_t_{0};
  uint32_t dropped_{0};
};

// ---- Engine -----------------------------------------------------------------------------
enum class State : uint8_t {
  IDLE,
  LISTEN,
  IDENTIFY,
  SCAN,
  CANDIDATE,
  VALIDATE,
  VALIDATE_OK,
  VALIDATE_FAILED,
  INSTALL,
  INSTALL_FAILED,
  NO_INTERNET,
  REBOOT,
  UNKNOWN,
  NO_REPLY,
  BUS_BUSY,
};
const char *state_str(State s);

enum class Step : uint8_t { NONE, P1_LISTEN, P2_DEVTYPE, P3_STATUS, P4_VERSION, P5_SCAN, VALIDATE_STATUS };
const char *step_str(Step s);

struct ProfileStats {
  uint32_t rx_bytes{0};
  uint32_t echo_bytes{0};
  uint32_t frames_ok{0};
};

struct Evidence {
  uint8_t tx_class{0};
  uint8_t rx_class{0};
  uint16_t rx_len{0};
  bool valid{false};
};

class Engine {
 public:
  static constexpr uint32_t P1_LISTEN_MS = 3000;
  static constexpr uint32_t IDLE_BEFORE_TX_MS = 500;   // after P1 traffic
  static constexpr uint32_t IDLE_WAIT_MAX_MS = 3000;   // give up -> bus_busy
  static constexpr uint32_t IDLE_PER_FRAME_MS = 50;
  static constexpr uint32_t REPLY_TIMEOUT_MS = 1000;
  static constexpr uint32_t ATTEMPT_GAP_MS = 300;
  static constexpr uint32_t MIN_TX_SPACING_MS = 300;
  static constexpr uint32_t MAX_FRAMES = 20;
  static constexpr uint32_t SCAN_MS = 2000;
  static constexpr uint32_t CANDIDATE_TIMEOUT_MS = 5 * 60 * 1000;
  static constexpr uint8_t P2_ATTEMPTS = 3;
  static constexpr uint8_t P3_ATTEMPTS = 2;
  static constexpr uint8_t P4_ATTEMPTS = 1;
  static constexpr uint8_t VALIDATE_ATTEMPTS = 3;

  explicit Engine(Bus *bus) : bus_(bus) {}

  // Returns the new probe id, or 0 when a probe or install is already running (409).
  uint32_t start(uint32_t now, bool passive_only);
  // Valid only in CANDIDATE / VALIDATE_FAILED, for the detected driver and probe id.
  bool confirm(uint32_t now, uint32_t probe_id, const char *driver);
  void cancel(uint32_t now);
  void tick(uint32_t now);

  // Install phase is driven by the ESPHome glue; the engine only records it.
  void set_install_state(State s, uint8_t pct, const char *msg);

  State state() const { return this->state_; }
  Step step() const { return this->step_; }
  uint32_t seq() const { return this->seq_; }
  uint32_t probe_id() const { return this->probe_id_; }
  bool busy() const;
  bool detected() const { return this->detected_; }
  uint8_t type_hi() const { return this->t_hi_; }
  uint8_t type_lo() const { return this->t_lo_; }
  uint32_t frames_sent() const { return this->frames_sent_; }
  uint32_t refused_frames() const { return this->refused_; }
  const Capture &capture() const { return this->cap_; }
  const char *outcome() const;
  bool hint_ab_swap() const;
  bool echo_seen() const { return this->echo_seen_; }
  uint8_t install_pct() const { return this->install_pct_; }
  const hisense_ac::proto::Status &status() const { return this->status_; }
  bool has_status() const { return this->has_status_; }

  // Test hook: the engine refuses to write anything tx_allowed() rejects. Returns false if
  // the frame was refused or rate-limited.
  bool send_raw_for_test(uint32_t now, const uint8_t *wire, size_t len) { return this->send_(now, wire, len); }

  // ---- Formatters (all bounded; no secrets are ever part of the engine's data) ----
  std::string state_json() const;
  std::string result_json(const char *fw, const char *board) const;
  std::string capture_json() const;
  std::string capture_text(const char *fw, const char *board) const;
  std::string report_url(const char *fw, const char *board, size_t max_len = 6000) const;

 protected:
  void set_state_(uint32_t now, State s, Step st);
  void begin_step_(uint32_t now, Step st);
  bool send_(uint32_t now, const uint8_t *wire, size_t len);
  void send_request_(uint32_t now);
  void on_frame_(uint32_t now, const uint8_t *f, size_t n);
  void step_failed_(uint32_t now);
  void finish_identify_(uint32_t now);
  void finish_probe_(uint32_t now);
  void pump_rx_(uint32_t now);
  uint32_t char_ms_() const;

  Bus *bus_;
  Capture cap_;
  hisense_ac::proto::FrameAssembler fa_;
  State state_{State::IDLE};
  Step step_{Step::NONE};
  uint32_t seq_{0};
  uint32_t probe_id_{0};
  uint32_t next_probe_id_{1};
  uint32_t t0_{0};
  uint32_t step_start_{0};
  uint32_t state_since_{0};
  uint32_t last_rx_{0};
  bool any_rx_{false};
  uint32_t last_tx_{0};
  bool any_tx_{false};
  uint32_t tx_busy_until_{0};
  uint32_t deadline_{0};
  bool waiting_reply_{false};
  uint8_t attempt_{0};
  uint8_t attempts_max_{0};
  uint32_t next_send_at_{0};
  uint8_t profile_{0};
  bool passive_only_{false};
  uint32_t frames_sent_{0};
  uint32_t refused_{0};
  // Results.
  bool detected_{false};
  bool via_status_{false};
  bool bus_busy_{false};
  bool echo_seen_{false};
  bool garbage_after_tx_{false};
  uint32_t foreign_frames_{0};
  uint8_t t_hi_{0}, t_lo_{0};
  bool has_status_{false};
  hisense_ac::proto::Status status_{};
  bool has_version_{false};
  std::string version_text_;
  uint32_t rx_after_tx_{0};
  uint32_t frames_this_window_{0};
  ProfileStats stats_[NUM_PROFILES];
  static constexpr size_t MAX_EVIDENCE = 8;
  Evidence evidence_[MAX_EVIDENCE];
  size_t n_evidence_{0};
  uint8_t last_tx_class_{0};
  uint8_t install_pct_{0};
  std::string install_msg_;
  const char *note_{""};
};

// Helpers shared with the glue.
std::string json_escape(const std::string &s);
std::string url_encode(const std::string &s);
std::string hex_spaced(const uint8_t *d, size_t n);
// Removes the F4 stuffing from a wire frame that starts with F4 F5 (for decoded offsets).
size_t unstuff(const uint8_t *wire, size_t n, uint8_t *out, size_t cap);

}  // namespace ac_setup
}  // namespace esphome

// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#include "hisense_ac.h"

#include <cinttypes>
#include <cmath>
#include <cstring>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace hisense_ac {

static const char *const TAG = "hisense_ac";

// Silence between the end of one transaction and the next frame we put on the bus.
static const uint32_t INTER_FRAME_GAP_MS = 40;
// How long the climate entity ignores status frames after a command (the AC needs a moment).
static const uint32_t COMMAND_SETTLE_MS = 3000;
// Consecutive unanswered status polls before the link is declared lost.
static const uint8_t MAX_MISSED_POLLS = 5;
static const uint8_t BOOT_TRIES = 5;
static const uint8_t MAX_COMMANDS_PER_CYCLE = 2;
// Sensors are re-published at least this often even if unchanged.
static const uint32_t SENSOR_REFRESH_MS = 60000;

static inline bool reached(uint32_t now, uint32_t t) { return static_cast<int32_t>(now - t) >= 0; }

// ---- Switches ---------------------------------------------------------------------------------
#ifdef USE_SWITCH
void HisenseDisplaySwitch::write_state(bool state) {
  this->parent_->set_display(state);
  this->publish_state(state);
}

void HisenseBeepSwitch::setup() {
  auto initial = this->get_initial_state_with_restore_mode();
  bool on = initial.has_value() ? *initial : this->parent_->get_beep();
  this->parent_->set_beep_runtime(on);
  this->publish_state(on);
}

void HisenseBeepSwitch::write_state(bool state) {
  this->parent_->set_beep_runtime(state);
  this->publish_state(state);
}

void HisenseFrameLogSwitch::setup() {
  auto initial = this->get_initial_state_with_restore_mode();
  bool on = initial.has_value() ? *initial : this->parent_->get_debug_frames();
  this->parent_->set_debug_frames_runtime(on);
  this->publish_state(on);
}

void HisenseFrameLogSwitch::write_state(bool state) {
  this->parent_->set_debug_frames_runtime(state);
  this->publish_state(state);
}
#endif

#ifdef USE_SWITCH
void HisenseFeatureSwitch::write_state(bool state) {
  this->parent_->set_feature(this->feature_, state);
  this->publish_state(state);
}
#endif
#ifdef USE_SELECT
void HisenseSleepSelect::control(size_t index) {
  this->parent_->set_sleep_option(static_cast<uint8_t>(index));
  this->publish_state(index);
}
#endif
#ifdef USE_BUTTON
void HisenseLightButton::press_action() { this->parent_->set_display(this->on_); }
#endif

// ---- Dedicated feature controls ---------------------------------------------------------------
void HisenseAC::set_feature(Feature f, bool on) {
  auto call = this->make_call();
  if (f == Feature::QUIET) {
    call.set_fan_mode(on ? climate::CLIMATE_FAN_QUIET : climate::CLIMATE_FAN_AUTO);
  } else {
    const climate::ClimatePreset p = f == Feature::ECO ? climate::CLIMATE_PRESET_ECO : climate::CLIMATE_PRESET_BOOST;
    const bool active = !this->has_custom_preset() && this->preset.has_value() && *this->preset == p;
    if (on)
      call.set_preset(p);
    else if (active)
      call.set_preset(climate::CLIMATE_PRESET_NONE);
    else
      return;
  }
  call.perform();
}

void HisenseAC::set_sleep_option(uint8_t profile) {
  const bool sleeping = !this->has_custom_preset() && this->preset.has_value() &&
                        *this->preset == climate::CLIMATE_PRESET_SLEEP;
  if (profile == 0) {
    if (sleeping) {
      auto call = this->make_call();
      call.set_preset(climate::CLIMATE_PRESET_NONE);
      call.perform();
    }
    return;
  }
  if (profile > 4)
    profile = 4;
  this->sleep_profile_ = profile;
  if (sleeping) {
    // Already in sleep: switch profile directly.
    proto::Command c = this->base_cmd_();
    c.has_sleep = true;
    c.sleep = profile;
    c.beep = true;
    this->enqueue_(c, true);
    this->hold_until_ = millis() + COMMAND_SETTLE_MS;
    return;
  }
  auto call = this->make_call();
  call.set_preset(climate::CLIMATE_PRESET_SLEEP);
  call.perform();
}

void HisenseAC::sync_controls_(const proto::Status &s) {
#ifdef USE_SWITCH
  for (auto *sw : this->feature_switches_) {
    bool v = false;
    switch (sw->get_feature()) {
      case Feature::ECO:
        v = s.eco;
        break;
      case Feature::TURBO:
        v = s.turbo;
        break;
      case Feature::QUIET:
        v = s.quiet || s.fan_quiet_step;
        break;
    }
    if (!s.power)
      v = false;
    if (!sw->has_state() || sw->state != v)
      sw->publish_state(v);
  }
#endif
#ifdef USE_SELECT
  if (this->sleep_select_ != nullptr) {
    size_t idx = (s.power && s.sleep > 0) ? (s.sleep > 4 ? 4 : s.sleep) : 0;
    auto cur = this->sleep_select_->active_index();
    if (!cur.has_value() || *cur != idx)
      this->sleep_select_->publish_state(idx);
  }
#endif
  (void) s;
}

void HisenseAC::sync_controls_from_climate_() {
  // Optimistic: reflect a climate change on the companion switches and select immediately,
  // instead of waiting for the AC to confirm it (COMMAND_SETTLE_MS later).
  const bool on = this->mode != climate::CLIMATE_MODE_OFF;
  const bool has_preset = this->preset.has_value() && !this->has_custom_preset();
#ifdef USE_SWITCH
  for (auto *sw : this->feature_switches_) {
    bool v = false;
    switch (sw->get_feature()) {
      case Feature::ECO:
        v = has_preset && *this->preset == climate::CLIMATE_PRESET_ECO;
        break;
      case Feature::TURBO:
        v = has_preset && *this->preset == climate::CLIMATE_PRESET_BOOST;
        break;
      case Feature::QUIET:
        v = this->fan_mode.has_value() && *this->fan_mode == climate::CLIMATE_FAN_QUIET;
        break;
    }
    v = v && on;
    if (!sw->has_state() || sw->state != v)
      sw->publish_state(v);
  }
#endif
#ifdef USE_SELECT
  if (this->sleep_select_ != nullptr) {
    const bool sleeping = on && has_preset && *this->preset == climate::CLIMATE_PRESET_SLEEP;
    const size_t idx = sleeping ? this->sleep_profile_ : 0;
    auto cur = this->sleep_select_->active_index();
    if (!cur.has_value() || *cur != idx)
      this->sleep_select_->publish_state(idx);
  }
#endif
  (void) on;
  (void) has_preset;
}

// ---- Raw frame logging ------------------------------------------------------------------------
void HisenseAC::set_debug_frames_runtime(bool on) {
  if (on && !this->debug_frames_)
    this->status_dumped_ = false;  // dump the next status frame right away
  this->debug_frames_ = on;
  ESP_LOGI(TAG, "Raw frame logging %s", on ? "ON (DEBUG level)" : "off");
}

void HisenseAC::log_frame_(const char *what, const uint8_t *f, size_t len) {
  // 16 bytes per line keeps every line well inside the logger buffer.
  ESP_LOGD(TAG, "%s frame: class %02X sub %02X, %u bytes", what, f[proto::OFF_CLASS], f[proto::OFF_SUB],
           (unsigned) len);
  for (size_t off = 0; off < len; off += 16)
    ESP_LOGD(TAG, "  %03u: %s", (unsigned) off, proto::hex_bytes(f, len, off, 16).c_str());
  if (f[proto::OFF_CLASS] == proto::CLASS_STATUS && f[proto::OFF_SUB] != proto::SUB_PRODUCT_TYPE && len > 46) {
    // The bytes still being calibrated, in decimal (temperatures signed).
    auto b = [&](size_t i) -> int { return i < len ? static_cast<int>(f[i]) : -1; };
    auto sb = [&](size_t i) -> int { return i < len ? static_cast<int>(static_cast<int8_t>(f[i])) : -999; };
    ESP_LOGD(TAG, "  key: room b20=%d coil b21=%d b22=%d b23=%d | comp b41=%d b42=%d b43=%d | out b44=%d b45=%d",
             sb(20), sb(21), b(22), b(23), b(41), b(42), b(43), sb(44), sb(45));
    if (len >= 150)
      ESP_LOGD(TAG, "  key: V b50=%d b51=%d | P b55=%d b56=%d b60=%d b71=%d | b144=%d b145=%d", b(50), b(51), b(55),
               b(56), b(60), b(71), b(144), b(145));
  }
}

// ---- Setup / config ---------------------------------------------------------------------------
void HisenseAC::setup() {
  this->set_supported_custom_fan_modes({FAN_MEDIUM_LOW, FAN_MEDIUM_HIGH});
  this->set_supported_custom_presets({PRESET_HEAT_8C});
  this->mode = climate::CLIMATE_MODE_OFF;
  this->target_temperature = 24.0f;
  this->current_temperature = NAN;
  this->set_fan_mode_(climate::CLIMATE_FAN_AUTO);
  this->swing_mode = climate::CLIMATE_SWING_OFF;
  this->set_preset_(climate::CLIMATE_PRESET_NONE);
  this->action = climate::CLIMATE_ACTION_OFF;

  this->step_ = this->handshake_ ? Step::BOOT_DEVTYPE : Step::CYCLE_BEGIN;
  // Give the AC time to boot when both are powered from the same 5 V rail.
  this->next_at_ = millis() + 2000;
  this->set_link_(LinkState::SEARCHING);
}

void HisenseAC::dump_config() {
  ESP_LOGCONFIG(TAG, "Hisense AC (RS-485 F4F5 protocol):");
  ESP_LOGCONFIG(TAG, "  Link handshake (0A/07/1E): %s", YESNO(this->handshake_));
  ESP_LOGCONFIG(TAG, "  Horizontal swing: %s", YESNO(this->horizontal_swing_));
  ESP_LOGCONFIG(TAG, "  Sleep profile: %u", this->sleep_profile_);
  ESP_LOGCONFIG(TAG, "  Bus cycle: %" PRIu32 " ms, reply timeout: %" PRIu32 " ms", this->cycle_ms_,
                this->reply_timeout_ms_);
  ESP_LOGCONFIG(TAG, "  Special command gap: %" PRIu32 " ms", this->special_gap_ms_);
  ESP_LOGCONFIG(TAG, "  Device type pair: %02X %02X", this->type_hi_, this->type_lo_);
  ESP_LOGCONFIG(TAG, "  Raw frame logging: %s (status every %" PRIu32 " ms)", ONOFF(this->debug_frames_),
                this->debug_interval_ms_);
}

const char *HisenseAC::get_link_state_text() const {
  switch (this->link_) {
    case LinkState::SEARCHING:
      return "Searching";
    case LinkState::HANDSHAKE:
      return "Handshake";
    case LinkState::CONNECTED:
      return "Connected";
    case LinkState::LOST:
    default:
      return "No response";
  }
}

void HisenseAC::set_link_(LinkState s) {
  const bool changed = s != this->link_;
  if (changed && this->link_ == LinkState::LOST && s == LinkState::CONNECTED) {
    // Back from an outage: ask for capabilities again if they never arrived and refresh every
    // entity with the first status frame.
    this->product_tries_ = 0;
    this->faults_published_ = false;
    this->last_sensor_force_ = millis() - SENSOR_REFRESH_MS;
  }
  this->link_ = s;
#ifdef USE_BINARY_SENSOR
  if (this->connected_ != nullptr && (changed || !this->connected_->has_state()))
    this->connected_->publish_state(s == LinkState::CONNECTED);
#endif
#ifdef USE_TEXT_SENSOR
  if (this->link_text_ != nullptr && (changed || !this->link_text_->has_state()))
    this->link_text_->publish_state(this->get_link_state_text());
#endif
  if (changed)
    ESP_LOGI(TAG, "Link: %s", this->get_link_state_text());
}

// ---- Bus --------------------------------------------------------------------------------------
void HisenseAC::loop() {
  this->read_uart_();
  const uint32_t now = millis();
  if (this->waiting_) {
    if (!reached(now, this->sent_at_ + this->reply_timeout_ms_))
      return;
    this->transaction_done_(false, now);
    return;
  }
  if (!reached(now, this->next_at_))
    return;
  this->run_step_(now);
}

void HisenseAC::read_uart_() {
  // Bulk reads (one driver call per chunk instead of per byte), bounded so a noisy line cannot
  // starve the main loop.
  uint8_t buf[64];
  size_t budget = 512;
  while (budget > 0) {
    size_t n = this->available();
    if (n == 0)
      break;
    if (n > sizeof(buf))
      n = sizeof(buf);
    if (n > budget)
      n = budget;
    if (!this->read_array(buf, n))
      break;
    budget -= n;
    for (size_t i = 0; i < n; i++) {
      if (this->rx_.feed(buf[i]))
        this->handle_frame_(this->rx_.frame(), this->rx_.size());
    }
  }
}

void HisenseAC::handle_frame_(const uint8_t *f, size_t len) {
  // Our own transmission can be echoed by auto-direction transceivers: only AC frames count.
  if (f[proto::OFF_DIR] != proto::DIR_FROM_AC)
    return;
  this->frames_ok_++;
  const uint8_t cls = f[proto::OFF_CLASS];
  ESP_LOGV(TAG, "RX class %02X sub %02X len %u", cls, f[proto::OFF_SUB], (unsigned) len);
  if (this->debug_frames_ && cls == proto::CLASS_STATUS) {
    const uint32_t now = millis();
    // Capability replies are rare: always log them. Status replies: at most one per interval.
    if (f[proto::OFF_SUB] == proto::SUB_PRODUCT_TYPE) {
      this->log_frame_("RX capabilities", f, len);
    } else if (!this->status_dumped_ || reached(now, this->last_status_dump_ + this->debug_interval_ms_)) {
      this->status_dumped_ = true;
      this->last_status_dump_ = now;
      this->log_frame_("RX status", f, len);
    }
  }

  switch (cls) {
    case proto::CLASS_DEVTYPE: {
      uint8_t hi, lo;
      if (proto::parse_devtype(f, len, &hi, &lo)) {
        if (hi != this->type_hi_ || lo != this->type_lo_ || !this->handshake_ok_)
          ESP_LOGI(TAG, "Device type %02X %02X", hi, lo);
        this->type_hi_ = hi;
        this->type_lo_ = lo;
        this->handshake_ok_ = true;
      }
      break;
    }
    case proto::CLASS_LINK:
      this->heard_ac_ = true;
      break;
    case proto::CLASS_STATUS:
      if (f[proto::OFF_SUB] == proto::SUB_PRODUCT_TYPE) {
        proto::Capabilities c;
        if (proto::parse_capabilities(f, len, &c)) {
          this->caps_ = c;
          ESP_LOGI(TAG, "Capabilities: heat/cool=%d power-save=%d purify=%d fan-mute=%d 8C-heat=%d humidity=%d",
                   c.cool_heat, c.power_save, c.purify, c.fan_mute, c.heat8, c.humidity);
        }
      } else {
        proto::Status s;
        if (proto::parse_status(f, len, &s))
          this->apply_status_(s);
      }
      break;
    default:
      break;
  }

  if (this->waiting_ && (this->expect_class_ == 0 || this->expect_class_ == cls))
    this->transaction_done_(true, millis());
}

void HisenseAC::send_(const uint8_t *frame, size_t len, uint8_t expect_class, uint32_t now) {
  uint8_t wire[proto::MAX_WIRE];
  const size_t n = proto::stuff(frame, len, wire, sizeof(wire));
  if (n == 0)
    return;
  // The frame fits the UART hardware FIFO, so write_array() returns at once. No flush(): it would
  // block the main loop for the whole transmission (~50 ms for a command at 9600 baud). The reply
  // timeout starts when the last byte has left the wire instead (10 bits per byte).
  this->write_array(wire, n);
  this->waiting_ = true;
  this->expect_class_ = expect_class;
  uint32_t baud = this->parent_->get_baud_rate();
  if (baud == 0)
    baud = 9600;
  this->sent_at_ = now + static_cast<uint32_t>((n * 10000UL + baud - 1) / baud);
}

void HisenseAC::run_step_(uint32_t now) {
  uint8_t f[proto::COMMAND_LEN];
  size_t n;
  switch (this->step_) {
    case Step::BOOT_DEVTYPE:
      if (this->boot_tries_ == 0)
        this->set_link_(LinkState::HANDSHAKE);
      n = proto::build_devtype_request(f, sizeof(f));
      this->send_(f, n, proto::CLASS_DEVTYPE, now);
      return;
    case Step::BOOT_VERSION:
      n = proto::build_version_request(this->type_hi_, this->type_lo_, f, sizeof(f));
      this->send_(f, n, 0, now);
      return;
    case Step::CYCLE_BEGIN:
      this->cycle_start_ = now;
      this->cycle_commands_ = 0;
      if (!this->handshake_) {
        this->step_ = Step::COMMAND;
      } else if (this->link_ == LinkState::LOST) {
        this->step_ = Step::RECOVER;
      } else {
        this->step_ = Step::HEARTBEAT;
      }
      this->run_step_(now);
      return;
    case Step::RECOVER:
      // One device-type request per cycle while the AC is silent (it may have rebooted).
      this->heard_ac_ = false;
      n = proto::build_devtype_request(f, sizeof(f));
      this->send_(f, n, proto::CLASS_DEVTYPE, now);
      return;
    case Step::HEARTBEAT:
      n = proto::build_heartbeat(this->type_hi_, this->type_lo_, this->heard_ac_, f, sizeof(f));
      this->send_(f, n, 0, now);
      return;
    case Step::COMMAND: {
      proto::CommandQueue::Item q;
      if (this->cycle_commands_ >= MAX_COMMANDS_PER_CYCLE || !this->queue_.pop_ready(now, &q)) {
        this->step_ = Step::STATUS;
        this->run_step_(now);
        return;
      }
      this->cycle_commands_++;
      proto::build_command(q.cmd, this->type_hi_, this->type_lo_, f);
      ESP_LOGD(TAG, "TX command%s (%u queued)", q.special ? " (special)" : "", (unsigned) this->queue_.size());
      if (this->debug_frames_) {
        this->log_frame_("TX command", f, proto::COMMAND_LEN);
        this->status_dumped_ = false;  // show the AC's answer to this command too
      }
      this->hold_until_ = now + COMMAND_SETTLE_MS;
      this->send_(f, proto::COMMAND_LEN, 0, now);
      return;
    }
    case Step::STATUS:
      n = proto::build_status_request(this->type_hi_, this->type_lo_, 0x00, f, sizeof(f));
      this->send_(f, n, proto::CLASS_STATUS, now);
      return;
    case Step::PRODUCT:
      if (this->caps_.valid || this->product_tries_ >= 3 || this->link_ != LinkState::CONNECTED) {
        this->step_ = Step::PACE;
        this->run_step_(now);
        return;
      }
      this->product_tries_++;
      n = proto::build_status_request(this->type_hi_, this->type_lo_, proto::SUB_PRODUCT_TYPE, f, sizeof(f));
      this->send_(f, n, proto::CLASS_STATUS, now);
      return;
    case Step::PACE:
      this->step_ = Step::CYCLE_BEGIN;
      this->next_at_ = this->cycle_start_ + this->cycle_ms_;
      // A command that arrived during this cycle starts the next one right away.
      if (reached(now, this->next_at_) || this->queue_.has_ready(now))
        this->next_at_ = now;
      return;
  }
}

void HisenseAC::transaction_done_(bool replied, uint32_t now) {
  this->waiting_ = false;
  this->next_at_ = now + INTER_FRAME_GAP_MS;
  this->bus_free_at_ = this->next_at_;
  switch (this->step_) {
    case Step::BOOT_DEVTYPE:
      if (replied && this->handshake_ok_) {
        this->step_ = Step::BOOT_VERSION;
      } else if (++this->boot_tries_ >= BOOT_TRIES) {
        ESP_LOGW(TAG, "No device-type reply; falling back to plain status polling");
        this->step_ = Step::CYCLE_BEGIN;
      } else {
        this->next_at_ = now + 500;
      }
      break;
    case Step::BOOT_VERSION:
      this->step_ = Step::CYCLE_BEGIN;
      break;
    case Step::RECOVER:
      this->step_ = Step::HEARTBEAT;
      break;
    case Step::HEARTBEAT:
      this->step_ = Step::COMMAND;
      break;
    case Step::COMMAND:
      this->step_ = Step::COMMAND;  // maybe one more, then STATUS
      break;
    case Step::STATUS:
      if (replied) {
        this->missed_polls_ = 0;
        this->set_link_(LinkState::CONNECTED);
      } else if (this->missed_polls_ < 255 && ++this->missed_polls_ >= MAX_MISSED_POLLS) {
        if (this->link_ != LinkState::LOST)
          ESP_LOGW(TAG, "AC stopped answering status polls (check A/B wiring and the 5 V supply)");
        this->set_link_(LinkState::LOST);
      } else if (this->link_ == LinkState::HANDSHAKE || this->link_ == LinkState::SEARCHING) {
        if (this->missed_polls_ >= MAX_MISSED_POLLS)
          this->set_link_(LinkState::LOST);
      }
      this->step_ = Step::PRODUCT;
      break;
    case Step::PRODUCT:
      this->step_ = Step::PACE;
      break;
    default:
      break;
  }
}

bool HisenseAC::enqueue_(const proto::Command &cmd, bool special) {
  const uint32_t now = millis();
  // Special modes (sleep, quiet, eco/turbo, 8C) are spaced special_gap apart by the queue because
  // the AC ignores them if they arrive too close together. Plain commands of one kind coalesce.
  switch (this->queue_.push(cmd, special, now, this->special_gap_ms_)) {
    case proto::CommandQueue::Push::FULL:
      ESP_LOGW(TAG, "Command queue full, dropping command");
      return false;
    case proto::CommandQueue::Push::MERGED:
      ESP_LOGV(TAG, "Command merged into the queued one");
      break;
    case proto::CommandQueue::Push::ADDED:
      break;
  }
  // Fast path: when the bus is idle between polling cycles, start the next cycle now instead of
  // waiting up to cycle_ms for it, so a tap in Home Assistant reaches the AC within ~100 ms.
  if (!this->waiting_ && this->step_ == Step::CYCLE_BEGIN && this->queue_.has_ready(now))
    this->next_at_ = reached(now, this->bus_free_at_) ? now : this->bus_free_at_;
  return true;
}

proto::Command HisenseAC::base_cmd_() const {
  proto::Command c;
  c.beep = this->beep_;
  return c;
}

void HisenseAC::set_display(bool on) {
  proto::Command c = this->base_cmd_();
  c.has_display = true;
  c.display = on;
  this->enqueue_(c, false);
}

uint8_t HisenseAC::to_panel_temp_(float c) const {
  float v = this->status_.fahrenheit ? proto::c_to_f(c) : c;
  long r = lroundf(v);
  if (r < 0)
    r = 0;
  if (r > 120)
    r = 120;
  return static_cast<uint8_t>(r);
}

float HisenseAC::from_panel_temp_(int16_t v) const {
  return this->status_.fahrenheit ? proto::f_to_c(static_cast<float>(v)) : static_cast<float>(v);
}

float HisenseAC::get_outdoor_temperature() const {
  if (!this->have_status_ || !this->status_.has_outdoor || !this->status_.outdoor_valid)
    return NAN;
  return static_cast<float>(this->status_.outdoor_temp);
}

// ---- Climate ----------------------------------------------------------------------------------
climate::ClimateTraits HisenseAC::traits() {
  climate::ClimateTraits t;
  t.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE | climate::CLIMATE_SUPPORTS_ACTION);
  t.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_COOL, climate::CLIMATE_MODE_HEAT,
                         climate::CLIMATE_MODE_DRY, climate::CLIMATE_MODE_FAN_ONLY, climate::CLIMATE_MODE_AUTO});
  t.set_supported_fan_modes({climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_QUIET, climate::CLIMATE_FAN_LOW,
                             climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH});
  if (this->horizontal_swing_) {
    t.set_supported_swing_modes({climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL,
                                 climate::CLIMATE_SWING_HORIZONTAL, climate::CLIMATE_SWING_BOTH});
  } else {
    t.set_supported_swing_modes({climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL});
  }
  t.set_supported_presets({climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_ECO, climate::CLIMATE_PRESET_BOOST,
                           climate::CLIMATE_PRESET_SLEEP});
  t.set_visual_min_temperature(16.0f);
  t.set_visual_max_temperature(32.0f);
  t.set_visual_target_temperature_step(1.0f);
  t.set_visual_current_temperature_step(1.0f);
  return t;
}

static proto::Mode to_proto_mode(climate::ClimateMode m) {
  switch (m) {
    case climate::CLIMATE_MODE_HEAT:
      return proto::Mode::HEAT;
    case climate::CLIMATE_MODE_DRY:
      return proto::Mode::DRY;
    case climate::CLIMATE_MODE_FAN_ONLY:
      return proto::Mode::FAN;
    case climate::CLIMATE_MODE_AUTO:
    case climate::CLIMATE_MODE_HEAT_COOL:
      return proto::Mode::AUTO;
    case climate::CLIMATE_MODE_COOL:
    default:
      return proto::Mode::COOL;
  }
}

// The special features that a preset maps to.
struct PresetFlags {
  bool eco{false};
  bool turbo{false};
  bool sleep{false};
  bool heat8{false};
};

void HisenseAC::control(const climate::ClimateCall &call) {
  const uint32_t now = millis();
  proto::Command normal = this->base_cmd_();
  bool has_normal = false;
  climate::ClimateMode new_mode = this->mode;

  // ---- Mode / power ----
  if (call.get_mode().has_value()) {
    new_mode = *call.get_mode();
    normal.has_power = true;
    normal.power = new_mode != climate::CLIMATE_MODE_OFF;
    if (normal.power) {
      normal.has_mode = true;
      normal.mode = to_proto_mode(new_mode);
    }
    has_normal = true;
    this->mode = new_mode;
  }

  // ---- Setpoint ----
  if (call.get_target_temperature().has_value()) {
    float t = *call.get_target_temperature();
    if (t < 16.0f)
      t = 16.0f;
    if (t > 32.0f)
      t = 32.0f;
    normal.has_setpoint = true;
    normal.setpoint = this->to_panel_temp_(t);
    has_normal = true;
    this->target_temperature = roundf(t);
  }

  // ---- Fan ----
  const bool was_quiet = this->fan_mode.has_value() && *this->fan_mode == climate::CLIMATE_FAN_QUIET;
  bool want_quiet = was_quiet;
  if (call.get_fan_mode().has_value()) {
    const climate::ClimateFanMode fm = *call.get_fan_mode();
    want_quiet = fm == climate::CLIMATE_FAN_QUIET;
    if (!want_quiet) {
      normal.has_fan = true;
      switch (fm) {
        case climate::CLIMATE_FAN_LOW:
          normal.fan = proto::Fan::LOW;
          break;
        case climate::CLIMATE_FAN_MEDIUM:
          normal.fan = proto::Fan::MEDIUM;
          break;
        case climate::CLIMATE_FAN_HIGH:
          normal.fan = proto::Fan::HIGH;
          break;
        default:
          normal.fan = proto::Fan::AUTO;
          break;
      }
      has_normal = true;
    }
    this->set_fan_mode_(fm);
  } else if (call.has_custom_fan_mode()) {
    const StringRef name = call.get_custom_fan_mode();
    want_quiet = false;
    normal.has_fan = true;
    normal.fan = name == FAN_MEDIUM_LOW ? proto::Fan::MEDIUM_LOW : proto::Fan::MEDIUM_HIGH;
    has_normal = true;
    this->set_custom_fan_mode_(name.c_str());
  }

  if (has_normal)
    this->enqueue_(normal, false);

  // ---- Swing (one axis per frame) ----
  if (call.get_swing_mode().has_value()) {
    const climate::ClimateSwingMode cur = this->swing_mode;
    const climate::ClimateSwingMode nxt = *call.get_swing_mode();
    const bool cur_v = cur == climate::CLIMATE_SWING_VERTICAL || cur == climate::CLIMATE_SWING_BOTH;
    const bool cur_h = cur == climate::CLIMATE_SWING_HORIZONTAL || cur == climate::CLIMATE_SWING_BOTH;
    const bool new_v = nxt == climate::CLIMATE_SWING_VERTICAL || nxt == climate::CLIMATE_SWING_BOTH;
    const bool new_h = nxt == climate::CLIMATE_SWING_HORIZONTAL || nxt == climate::CLIMATE_SWING_BOTH;
    if (new_v != cur_v || nxt == climate::CLIMATE_SWING_OFF) {
      proto::Command c = this->base_cmd_();
      c.has_vswing = true;
      c.vswing = new_v;
      this->enqueue_(c, false);
    }
    if (this->horizontal_swing_ && (new_h != cur_h || (nxt == climate::CLIMATE_SWING_OFF && cur_h))) {
      proto::Command c = this->base_cmd_();
      c.has_hswing = true;
      c.hswing = new_h;
      this->enqueue_(c, false);
    }
    this->swing_mode = nxt;
  }

  // ---- Presets and quiet (special commands, spaced apart) ----
  PresetFlags cur;
  if (this->has_custom_preset()) {
    cur.heat8 = true;
  } else if (this->preset.has_value()) {
    cur.eco = *this->preset == climate::CLIMATE_PRESET_ECO;
    cur.turbo = *this->preset == climate::CLIMATE_PRESET_BOOST;
    cur.sleep = *this->preset == climate::CLIMATE_PRESET_SLEEP;
  }
  PresetFlags want = cur;
  bool preset_changed = false;
  if (call.get_preset().has_value()) {
    const climate::ClimatePreset p = *call.get_preset();
    want = PresetFlags();
    want.eco = p == climate::CLIMATE_PRESET_ECO;
    want.turbo = p == climate::CLIMATE_PRESET_BOOST;
    want.sleep = p == climate::CLIMATE_PRESET_SLEEP;
    preset_changed = true;
    this->set_preset_(p);
  } else if (call.has_custom_preset()) {
    want = PresetFlags();
    want.heat8 = true;
    preset_changed = true;
    if (new_mode != climate::CLIMATE_MODE_HEAT)
      ESP_LOGW(TAG, "'%s' is only accepted by the AC in HEAT mode", PRESET_HEAT_8C);
    this->set_custom_preset_(PRESET_HEAT_8C);
  }
  // A power-off command clears the special modes on the AC by itself.
  if (new_mode == climate::CLIMATE_MODE_OFF) {
    preset_changed = false;
    want_quiet = false;
  }

  auto special = [this](void (*fill)(proto::Command &, bool), bool on) {
    proto::Command c = this->base_cmd_();
    fill(c, on);
    this->enqueue_(c, true);
  };
  if (preset_changed) {
    // Switch off first, then on (turbo and eco are mutually exclusive on the AC).
    if (cur.eco && !want.eco)
      special([](proto::Command &c, bool on) { c.has_eco = true; c.eco = on; }, false);
    if (cur.turbo && !want.turbo)
      special([](proto::Command &c, bool on) { c.has_turbo = true; c.turbo = on; }, false);
    if (cur.sleep && !want.sleep)
      special([](proto::Command &c, bool on) { c.has_sleep = true; c.sleep = 0; c.beep = true; (void) on; }, false);
    if (cur.heat8 && !want.heat8)
      special([](proto::Command &c, bool on) { c.has_heat8 = true; c.heat8 = on; }, false);
    if (want.eco && !cur.eco)
      special([](proto::Command &c, bool on) { c.has_eco = true; c.eco = on; }, true);
    if (want.turbo && !cur.turbo)
      special([](proto::Command &c, bool on) { c.has_turbo = true; c.turbo = on; }, true);
    if (want.sleep && !cur.sleep) {
      proto::Command c = this->base_cmd_();
      c.has_sleep = true;
      c.sleep = this->sleep_profile_;
      c.beep = true;
      this->enqueue_(c, true);
    }
    if (want.heat8 && !cur.heat8)
      special([](proto::Command &c, bool on) { c.has_heat8 = true; c.heat8 = on; }, true);
  }
  if (want_quiet != was_quiet && new_mode != climate::CLIMATE_MODE_OFF)
    special([](proto::Command &c, bool on) { c.has_quiet = true; c.quiet = on; }, want_quiet);

  this->hold_until_ = now + COMMAND_SETTLE_MS;
  this->publish_state();
  this->sync_controls_from_climate_();
}

void HisenseAC::apply_status_(const proto::Status &s) {
  const bool unit_changed = !this->have_status_ || s.fahrenheit != this->status_.fahrenheit;
  this->status_ = s;
  this->have_status_ = true;
  if (unit_changed)
    ESP_LOGI(TAG, "Status frame %u bytes, panel unit %s", (unsigned) s.frame_len, s.fahrenheit ? "F" : "C");

  this->publish_sensors_(s);

#ifdef USE_SWITCH
  if (this->display_switch_ != nullptr && this->queue_.empty() &&
      (!this->display_switch_->state != !s.display || !this->display_switch_->has_state()))
    if (reached(millis(), this->hold_until_))
      this->display_switch_->publish_state(s.display);
#endif

  const uint32_t now = millis();
  if (!this->queue_.empty() || !reached(now, this->hold_until_))
    return;

  this->sync_controls_(s);

  // ---- Climate entity ----
  bool changed = false;
  climate::ClimateMode m = climate::CLIMATE_MODE_OFF;
  if (s.power) {
    switch (s.mode) {
      case proto::Mode::HEAT:
        m = climate::CLIMATE_MODE_HEAT;
        break;
      case proto::Mode::COOL:
        m = climate::CLIMATE_MODE_COOL;
        break;
      case proto::Mode::DRY:
        m = climate::CLIMATE_MODE_DRY;
        break;
      case proto::Mode::FAN:
        m = climate::CLIMATE_MODE_FAN_ONLY;
        break;
      case proto::Mode::AUTO:
      default:
        m = climate::CLIMATE_MODE_AUTO;
        break;
    }
  }
  if (m != this->mode) {
    this->mode = m;
    changed = true;
  }
  if (s.setpoint > 0) {
    const float t = roundf(this->from_panel_temp_(s.setpoint));
    if (t != this->target_temperature) {
      this->target_temperature = t;
      changed = true;
    }
  }
  const float room = this->from_panel_temp_(s.room_temp);
  if (std::isnan(this->current_temperature) || room != this->current_temperature) {
    this->current_temperature = room;
    changed = true;
  }

  // Fan
  if (s.quiet || s.fan_quiet_step) {
    changed |= this->set_fan_mode_(climate::CLIMATE_FAN_QUIET);
  } else {
    switch (s.fan) {
      case proto::Fan::LOW:
        changed |= this->set_fan_mode_(climate::CLIMATE_FAN_LOW);
        break;
      case proto::Fan::MEDIUM_LOW:
        changed |= this->set_custom_fan_mode_(FAN_MEDIUM_LOW);
        break;
      case proto::Fan::MEDIUM:
        changed |= this->set_fan_mode_(climate::CLIMATE_FAN_MEDIUM);
        break;
      case proto::Fan::MEDIUM_HIGH:
        changed |= this->set_custom_fan_mode_(FAN_MEDIUM_HIGH);
        break;
      case proto::Fan::HIGH:
        changed |= this->set_fan_mode_(climate::CLIMATE_FAN_HIGH);
        break;
      case proto::Fan::AUTO:
      default:
        changed |= this->set_fan_mode_(climate::CLIMATE_FAN_AUTO);
        break;
    }
  }

  // Swing
  climate::ClimateSwingMode sw = climate::CLIMATE_SWING_OFF;
  const bool h = this->horizontal_swing_ && s.hswing;
  if (s.vswing && h)
    sw = climate::CLIMATE_SWING_BOTH;
  else if (s.vswing)
    sw = climate::CLIMATE_SWING_VERTICAL;
  else if (h)
    sw = climate::CLIMATE_SWING_HORIZONTAL;
  if (sw != this->swing_mode) {
    this->swing_mode = sw;
    changed = true;
  }

  // Preset (one at a time in Home Assistant; priority: 8C > boost > eco > sleep)
  if (s.heat8) {
    changed |= this->set_custom_preset_(PRESET_HEAT_8C);
  } else if (s.turbo) {
    changed |= this->set_preset_(climate::CLIMATE_PRESET_BOOST);
  } else if (s.eco) {
    changed |= this->set_preset_(climate::CLIMATE_PRESET_ECO);
  } else if (s.sleep > 0) {
    changed |= this->set_preset_(climate::CLIMATE_PRESET_SLEEP);
  } else {
    changed |= this->set_preset_(climate::CLIMATE_PRESET_NONE);
  }

  // Action
  climate::ClimateAction a = climate::CLIMATE_ACTION_OFF;
  if (s.power) {
    const bool running = s.has_outdoor && s.compressor_hz > 0;
    switch (m) {
      case climate::CLIMATE_MODE_COOL:
        a = running ? climate::CLIMATE_ACTION_COOLING : climate::CLIMATE_ACTION_IDLE;
        break;
      case climate::CLIMATE_MODE_HEAT:
        a = running ? climate::CLIMATE_ACTION_HEATING : climate::CLIMATE_ACTION_IDLE;
        break;
      case climate::CLIMATE_MODE_DRY:
        a = running ? climate::CLIMATE_ACTION_DRYING : climate::CLIMATE_ACTION_IDLE;
        break;
      case climate::CLIMATE_MODE_FAN_ONLY:
        a = climate::CLIMATE_ACTION_FAN;
        break;
      default:
        if (!running)
          a = climate::CLIMATE_ACTION_IDLE;
        else
          a = s.indoor_coil_temp < s.room_temp ? climate::CLIMATE_ACTION_COOLING : climate::CLIMATE_ACTION_HEATING;
        break;
    }
  }
  if (a != this->action) {
    this->action = a;
    changed = true;
  }

  if (changed)
    this->publish_state();
}

void HisenseAC::publish_sensors_(const proto::Status &s) {
  const uint32_t now = millis();
  const bool force = reached(now, this->last_sensor_force_ + SENSOR_REFRESH_MS);
  if (force)
    this->last_sensor_force_ = now;
  // Faults change rarely: binary and text entities are only touched when the fault bytes change.
  const bool faults_changed = !this->faults_published_ || s.faults != this->last_faults_;
  this->last_faults_ = s.faults;
  this->faults_published_ = true;
  (void) force;
  (void) faults_changed;
#ifdef USE_SENSOR
  auto pub = [force](sensor::Sensor *sn, float v) {
    if (sn == nullptr)
      return;
    const float old = sn->get_raw_state();
    const bool same = (std::isnan(old) && std::isnan(v)) || (!std::isnan(old) && !std::isnan(v) && fabsf(old - v) < 0.01f);
    if (force || !same || !sn->has_state())
      sn->publish_state(v);
  };
  const bool od = s.has_outdoor && s.outdoor_valid;
  pub(this->indoor_coil_temp_, this->from_panel_temp_(s.indoor_coil_temp));
  pub(this->outdoor_temp_, od ? static_cast<float>(s.outdoor_temp) : NAN);
  pub(this->outdoor_coil_temp_, od ? static_cast<float>(s.outdoor_coil_temp) : NAN);
  pub(this->compressor_hz_, s.has_outdoor ? static_cast<float>(s.compressor_hz) : NAN);
  pub(this->compressor_target_hz_, s.has_outdoor ? static_cast<float>(s.compressor_target_hz) : NAN);
  // Humidity, voltage and current are optional: many units never report them. Until a real value
  // arrives nothing is published, so they stay "unknown" instead of flapping NaN updates.
  const bool hum_ok = s.has_humidity && (!this->caps_.valid || this->caps_.humidity);
  this->seen_humidity_ |= hum_ok;
  if (this->seen_humidity_)
    pub(this->humidity_, hum_ok ? static_cast<float>(s.humidity) : NAN);
  const float watts = proto::power_watts(s);
  const bool watts_ok = watts >= 0 && watts <= 6000.0f;
  const bool volts_ok = s.has_electrical && s.voltage >= 50;
  this->seen_voltage_ |= volts_ok;
  if (this->seen_voltage_) {
    pub(this->voltage_, volts_ok ? static_cast<float>(s.voltage) : NAN);
    pub(this->current_, (volts_ok && watts_ok) ? watts / (s.voltage * 0.95f) : NAN);
  }
  pub(this->power_, watts_ok ? watts : NAN);
#endif
#ifdef USE_BINARY_SENSOR
  if (this->fault_ != nullptr && (!this->fault_->has_state() || this->fault_->state != s.faults.any()))
    this->fault_->publish_state(s.faults.any());
  if (this->aux_heater_ != nullptr && (!this->aux_heater_->has_state() || this->aux_heater_->state != s.aux_heater))
    this->aux_heater_->publish_state(s.aux_heater);
  for (auto &fs : this->fault_sensors_) {
    if (!faults_changed && fs.sensor->has_state())
      continue;
    uint8_t b = 0;
    switch (fs.group) {
      case FaultGroup::INDOOR:
        b = s.faults.indoor;
        break;
      case FaultGroup::MODULE:
        b = s.faults.module;
        break;
      case FaultGroup::OUTDOOR:
        b = s.faults.outdoor;
        break;
      case FaultGroup::PROTECT:
        b = s.faults.protect;
        break;
    }
    const bool on = (b & fs.mask) != 0;
    if (!fs.sensor->has_state() || fs.sensor->state != on)
      fs.sensor->publish_state(on);
  }
#endif
#ifdef USE_TEXT_SENSOR
  // describe_faults() builds a string: only do it when the fault bytes actually changed.
  if (this->fault_text_ != nullptr && (faults_changed || !this->fault_text_->has_state())) {
    std::string d = proto::describe_faults(s.faults);
    if (d.empty())
      d = "OK";
    if (!this->fault_text_->has_state() || this->fault_text_->state != d)
      this->fault_text_->publish_state(d);
  }
#endif
}

}  // namespace hisense_ac
}  // namespace esphome

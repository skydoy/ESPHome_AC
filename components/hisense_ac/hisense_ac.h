// ESPHome climate component for Hisense (and OEM) split air conditioners that use an
// AEH-W4x Wi-Fi module on the indoor unit's RS-485 "XM" bus (F4 F5 protocol).
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif

#include "hisense_protocol.h"

namespace esphome {
namespace hisense_ac {

static const char *const FAN_MEDIUM_LOW = "Medium-Low";
static const char *const FAN_MEDIUM_HIGH = "Medium-High";
static const char *const PRESET_HEAT_8C = "Heat 8C";

enum class LinkState : uint8_t { SEARCHING, HANDSHAKE, CONNECTED, LOST };

class HisenseAC;

#ifdef USE_SWITCH
class HisenseDisplaySwitch : public switch_::Switch, public Parented<HisenseAC> {
 protected:
  void write_state(bool state) override;
};
class HisenseBeepSwitch : public switch_::Switch, public Component, public Parented<HisenseAC> {
 public:
  void setup() override;
 protected:
  void write_state(bool state) override;
};
class HisenseFrameLogSwitch : public switch_::Switch, public Component, public Parented<HisenseAC> {
 public:
  void setup() override;
 protected:
  void write_state(bool state) override;
};
#endif

class HisenseAC : public Component, public climate::Climate, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // ---- Configuration ----
  void set_horizontal_swing(bool v) { this->horizontal_swing_ = v; }
  void set_sleep_profile(uint8_t v) { this->sleep_profile_ = v; }
  void set_handshake(bool v) { this->handshake_ = v; }
  void set_beep(bool v) { this->beep_ = v; }
  void set_reply_timeout(uint32_t v) { this->reply_timeout_ms_ = v; }
  void set_cycle_interval(uint32_t v) { this->cycle_ms_ = v; }
  void set_special_command_gap(uint32_t v) { this->special_gap_ms_ = v; }
  void set_debug_frames(bool v) { this->debug_frames_ = v; }
  void set_debug_frames_interval(uint32_t v) { this->debug_interval_ms_ = v; }
#ifdef USE_SENSOR
  void set_outdoor_temperature_sensor(sensor::Sensor *s) { this->outdoor_temp_ = s; }
  void set_indoor_coil_temperature_sensor(sensor::Sensor *s) { this->indoor_coil_temp_ = s; }
  void set_outdoor_coil_temperature_sensor(sensor::Sensor *s) { this->outdoor_coil_temp_ = s; }
  void set_compressor_frequency_sensor(sensor::Sensor *s) { this->compressor_hz_ = s; }
  void set_compressor_target_frequency_sensor(sensor::Sensor *s) { this->compressor_target_hz_ = s; }
  void set_humidity_sensor(sensor::Sensor *s) { this->humidity_ = s; }
  void set_voltage_sensor(sensor::Sensor *s) { this->voltage_ = s; }
  void set_power_sensor(sensor::Sensor *s) { this->power_ = s; }
  void set_current_sensor(sensor::Sensor *s) { this->current_ = s; }
#endif
#ifdef USE_BINARY_SENSOR
  void set_connected_binary_sensor(binary_sensor::BinarySensor *s) { this->connected_ = s; }
  void set_fault_binary_sensor(binary_sensor::BinarySensor *s) { this->fault_ = s; }
  void set_aux_heater_binary_sensor(binary_sensor::BinarySensor *s) { this->aux_heater_ = s; }
#endif
#ifdef USE_TEXT_SENSOR
  void set_fault_text_sensor(text_sensor::TextSensor *s) { this->fault_text_ = s; }
  void set_link_text_sensor(text_sensor::TextSensor *s) { this->link_text_ = s; }
#endif
#ifdef USE_SWITCH
  void set_display_switch(HisenseDisplaySwitch *s) { this->display_switch_ = s; }
  void set_beep_switch(HisenseBeepSwitch *s) { this->beep_switch_ = s; }
  void set_frame_log_switch(HisenseFrameLogSwitch *s) { this->frame_log_switch_ = s; }
#endif

  // ---- Runtime API (also handy from YAML lambdas, e.g. the status screen) ----
  void set_display(bool on);
  bool get_beep() const { return this->beep_; }
  void set_beep_runtime(bool on) { this->beep_ = on; }
  /// Raw frame logging (DEBUG level): status replies rate limited, commands always.
  bool get_debug_frames() const { return this->debug_frames_; }
  void set_debug_frames_runtime(bool on);
  bool is_connected() const { return this->link_ == LinkState::CONNECTED; }
  LinkState get_link_state() const { return this->link_; }
  const char *get_link_state_text() const;
  bool has_status() const { return this->have_status_; }
  const proto::Status &get_status() const { return this->status_; }
  bool has_fault() const { return this->have_status_ && this->status_.faults.any(); }
  /// Outdoor temperature in C, NAN if unknown.
  float get_outdoor_temperature() const;

 protected:
  // ---- Climate ----
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  // ---- Bus ----
  enum class Step : uint8_t {
    BOOT_DEVTYPE,
    BOOT_VERSION,
    CYCLE_BEGIN,
    RECOVER,
    HEARTBEAT,
    COMMAND,
    STATUS,
    PRODUCT,
    PACE,
  };
  struct Queued {
    proto::Command cmd;
    uint32_t not_before{0};
    bool special{false};
  };
  static constexpr size_t QUEUE_SIZE = 12;

  void read_uart_();
  void handle_frame_(const uint8_t *f, size_t len);
  void run_step_(uint32_t now);
  void send_(const uint8_t *frame, size_t len, uint8_t expect_class, uint32_t now);
  void transaction_done_(bool replied, uint32_t now);
  void set_link_(LinkState s);
  bool enqueue_(const proto::Command &cmd, bool special);
  proto::Command base_cmd_() const;
  void apply_status_(const proto::Status &s);
  void publish_sensors_(const proto::Status &s);
  uint8_t to_panel_temp_(float c) const;
  float from_panel_temp_(int16_t v) const;
  void log_frame_(const char *what, const uint8_t *f, size_t len);

  // Bus state
  proto::FrameAssembler rx_;
  Step step_{Step::BOOT_DEVTYPE};
  bool waiting_{false};
  uint8_t expect_class_{0};
  bool got_reply_{false};
  uint32_t sent_at_{0};
  uint32_t next_at_{0};
  uint32_t cycle_start_{0};
  uint8_t boot_tries_{0};
  uint8_t missed_polls_{0};
  uint8_t product_tries_{0};
  uint8_t cycle_commands_{0};
  uint8_t type_hi_{0x01};
  uint8_t type_lo_{0x01};
  bool heard_ac_{false};
  bool handshake_ok_{false};
  LinkState link_{LinkState::SEARCHING};
  Queued queue_[QUEUE_SIZE];
  size_t queue_len_{0};
  uint32_t last_special_at_{0};
  bool any_special_sent_{false};
  uint32_t hold_until_{0};
  uint32_t frames_ok_{0};

  // Configuration
  bool horizontal_swing_{false};
  uint8_t sleep_profile_{1};
  bool handshake_{true};
  bool beep_{true};
  uint32_t reply_timeout_ms_{1000};
  uint32_t cycle_ms_{1000};
  uint32_t special_gap_ms_{8000};
  bool debug_frames_{false};
  uint32_t debug_interval_ms_{10000};
  uint32_t last_status_dump_{0};
  bool status_dumped_{false};

  // Last known AC state
  proto::Status status_;
  bool have_status_{false};
  proto::Capabilities caps_;
  uint32_t last_sensor_force_{0};
  // Optional readings some units never report: stay "unknown" until a real value arrives.
  bool seen_humidity_{false};
  bool seen_voltage_{false};

#ifdef USE_SENSOR
  sensor::Sensor *outdoor_temp_{nullptr};
  sensor::Sensor *indoor_coil_temp_{nullptr};
  sensor::Sensor *outdoor_coil_temp_{nullptr};
  sensor::Sensor *compressor_hz_{nullptr};
  sensor::Sensor *compressor_target_hz_{nullptr};
  sensor::Sensor *humidity_{nullptr};
  sensor::Sensor *voltage_{nullptr};
  sensor::Sensor *power_{nullptr};
  sensor::Sensor *current_{nullptr};
#endif
#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *connected_{nullptr};
  binary_sensor::BinarySensor *fault_{nullptr};
  binary_sensor::BinarySensor *aux_heater_{nullptr};
#endif
#ifdef USE_TEXT_SENSOR
  text_sensor::TextSensor *fault_text_{nullptr};
  text_sensor::TextSensor *link_text_{nullptr};
#endif
#ifdef USE_SWITCH
  HisenseDisplaySwitch *display_switch_{nullptr};
  HisenseBeepSwitch *beep_switch_{nullptr};
  HisenseFrameLogSwitch *frame_log_switch_{nullptr};
#endif
};

}  // namespace hisense_ac
}  // namespace esphome

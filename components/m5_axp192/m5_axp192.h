// AXP192 power chip of the M5StickC Plus / Plus SE: power-on setup, LCD backlight, battery.
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/output/float_output.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace m5_axp192 {

class M5Axp192 : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  // After the I2C bus, before the display (the LCD logic supply comes from LDO3).
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  // 0 = LDO2 off, otherwise 0..1 mapped to 2.5..3.0 V.
  void set_backlight(float level);
  // Battery voltage in volts, NAN if the chip did not answer.
  float battery_voltage();

 protected:
  bool update_bits_(uint8_t reg, uint8_t clear_mask, uint8_t set_bits);
  bool ok_{false};
};

class M5Axp192Backlight : public output::FloatOutput {
 public:
  explicit M5Axp192Backlight(M5Axp192 *parent) : parent_(parent) {}

 protected:
  void write_state(float state) override { this->parent_->set_backlight(state); }
  M5Axp192 *parent_;
};

class M5Axp192Battery : public sensor::Sensor, public PollingComponent {
 public:
  explicit M5Axp192Battery(M5Axp192 *parent) : parent_(parent) {}
  void update() override { this->publish_state(this->parent_->battery_voltage()); }
  void dump_config() override;

 protected:
  M5Axp192 *parent_;
};

}  // namespace m5_axp192
}  // namespace esphome

// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#include "m5_axp192.h"

#include <cmath>

#include "esphome/core/log.h"

namespace esphome {
namespace m5_axp192 {

static const char *const TAG = "m5_axp192";

// AXP192 registers (X-Powers datasheet).
static const uint8_t REG_POWER_OUTPUT = 0x12;  // bit0 DCDC1, bit1 DCDC3, bit2 LDO2, bit3 LDO3, bit4 DCDC2, bit6 EXTEN
static const uint8_t REG_LDO23_VOLTAGE = 0x28;  // high nibble LDO2, low nibble LDO3: 1.8 V + 0.1 V/step
static const uint8_t REG_VBUS_IPSOUT = 0x30;
static const uint8_t REG_VOFF = 0x31;
static const uint8_t REG_CHARGE_1 = 0x33;
static const uint8_t REG_BACKUP_CHARGE = 0x35;
static const uint8_t REG_PEK = 0x36;
static const uint8_t REG_BATTERY_DETECT = 0x32;
static const uint8_t REG_TEMP_LIMIT = 0x39;
static const uint8_t REG_BAT_VOLTAGE_H = 0x78;
static const uint8_t REG_ADC_ENABLE_1 = 0x82;
static const uint8_t REG_ADC_RATE = 0x84;
static const uint8_t REG_GPIO0_CTRL = 0x90;
static const uint8_t REG_GPIO0_LDO_VOLTAGE = 0x91;

static const uint8_t LDO2_ENABLE = 1 << 2;
// Backlight range used by M5Stack for this LCD: 2.5 V (step 7) to 3.0 V (step 12).
static const uint8_t BACKLIGHT_MIN_STEP = 7;
static const uint8_t BACKLIGHT_MAX_STEP = 12;

void M5Axp192::setup() {
  // Same sequence as M5Stack's M5StickCPlus AXP192::begin().
  struct Write {
    uint8_t reg, value;
  };
  static const Write WRITES[] = {
      {REG_LDO23_VOLTAGE, 0xCC},      // LDO2 (backlight) and LDO3 (LCD logic) at 3.0 V
      {REG_ADC_RATE, 0xF2},           // ADC sample rate 200 Hz
      {REG_ADC_ENABLE_1, 0xFF},       // all ADCs on (battery voltage reading)
      {REG_CHARGE_1, 0xC0},           // charge to 4.2 V at 100 mA
      {REG_PEK, 0x0C},                // power key: 128 ms on, 4 s off
      {REG_GPIO0_LDO_VOLTAGE, 0xF0},  // GPIO0 LDO (microphone supply) 3.3 V
      {REG_GPIO0_CTRL, 0x02},         // GPIO0 as LDO
      {REG_VBUS_IPSOUT, 0x80},        // VBUS path without hold limit
      {REG_TEMP_LIMIT, 0xFC},         // battery temperature protection
      {REG_BACKUP_CHARGE, 0xA2},      // RTC backup battery charging
      {REG_BATTERY_DETECT, 0x46},     // battery detection on
  };
  for (const auto &w : WRITES) {
    if (!this->write_byte(w.reg, w.value)) {
      ESP_LOGE(TAG, "No answer from the AXP192 (register 0x%02X)", w.reg);
      this->mark_failed();
      return;
    }
  }
  // DCDC1 (ESP32), LDO2, LDO3 and EXTEN (5 V boost) on, DCDC2 off; keep DCDC3 as it was.
  // Power-off voltage 3.0 V.
  if (!this->update_bits_(REG_POWER_OUTPUT, 0x10, 0x4D) || !this->update_bits_(REG_VOFF, 0x07, 0x04)) {
    ESP_LOGE(TAG, "Could not configure the AXP192 power outputs");
    this->mark_failed();
    return;
  }
  this->ok_ = true;
}

void M5Axp192::dump_config() {
  ESP_LOGCONFIG(TAG, "AXP192 (M5StickC Plus / Plus SE):");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Setup failed: no LCD backlight, no battery reading");
  }
}

bool M5Axp192::update_bits_(uint8_t reg, uint8_t clear_mask, uint8_t set_bits) {
  uint8_t v;
  if (!this->read_byte(reg, &v))
    return false;
  return this->write_byte(reg, (uint8_t) ((v & ~clear_mask) | set_bits));
}

void M5Axp192::set_backlight(float level) {
  if (!this->ok_)
    return;
  if (level <= 0.0f) {
    this->update_bits_(REG_POWER_OUTPUT, LDO2_ENABLE, 0);
    return;
  }
  if (level > 1.0f)
    level = 1.0f;
  const uint8_t step =
      BACKLIGHT_MIN_STEP + (uint8_t) lroundf(level * (float) (BACKLIGHT_MAX_STEP - BACKLIGHT_MIN_STEP));
  this->update_bits_(REG_LDO23_VOLTAGE, 0xF0, (uint8_t) (step << 4));
  this->update_bits_(REG_POWER_OUTPUT, 0, LDO2_ENABLE);
}

float M5Axp192::battery_voltage() {
  uint8_t raw[2];
  if (!this->ok_ || !this->read_bytes(REG_BAT_VOLTAGE_H, raw, 2))
    return NAN;
  const uint16_t adc = ((uint16_t) raw[0] << 4) | (raw[1] & 0x0F);
  return adc * 1.1f / 1000.0f;
}

void M5Axp192Battery::dump_config() { LOG_SENSOR("", "AXP192 battery voltage", this); }

}  // namespace m5_axp192
}  // namespace esphome

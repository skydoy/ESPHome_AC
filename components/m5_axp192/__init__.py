"""AXP192 power chip of the M5StickC Plus / Plus SE (minimal, board specific).

On these Sticks the LCD backlight (LDO2), the LCD logic supply (LDO3), the microphone supply
(GPIO0 as LDO) and the 5 V boost (EXTEN) all come from the AXP192 on the internal I2C bus
(G21 SDA, G22 SCL, address 0x34). setup() writes the same power-on configuration as M5Stack's
own M5StickCPlus library (AXP192::begin), before the display starts. The "output" platform
drives the backlight and the "sensor" platform reads the battery voltage.

Not used by the M5StickC Plus2 (no AXP192: GPIO backlight, HOLD pin, ADC battery reading).

SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""

import esphome.codegen as cg
from esphome.components import i2c
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@skydoy"]
DEPENDENCIES = ["i2c"]
MULTI_CONF = False

CONF_M5_AXP192_ID = "m5_axp192_id"

m5_axp192_ns = cg.esphome_ns.namespace("m5_axp192")
M5Axp192 = m5_axp192_ns.class_("M5Axp192", cg.Component, i2c.I2CDevice)

CONFIG_SCHEMA = (
    cv.Schema({cv.GenerateID(): cv.declare_id(M5Axp192)})
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x34))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

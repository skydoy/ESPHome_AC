"""LCD backlight of the M5StickC Plus / Plus SE: AXP192 LDO2 as a float output.

0 switches LDO2 off; any other level maps to 2.5-3.0 V, the range M5Stack uses for this
backlight. Works with the light component like a PWM output does on the Plus2.

SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""

import esphome.codegen as cg
from esphome.components import output
import esphome.config_validation as cv
from esphome.const import CONF_ID

from . import CONF_M5_AXP192_ID, M5Axp192, m5_axp192_ns

DEPENDENCIES = ["m5_axp192"]

M5Axp192Backlight = m5_axp192_ns.class_("M5Axp192Backlight", output.FloatOutput)

CONFIG_SCHEMA = output.FLOAT_OUTPUT_SCHEMA.extend(
    {
        cv.Required(CONF_ID): cv.declare_id(M5Axp192Backlight),
        cv.GenerateID(CONF_M5_AXP192_ID): cv.use_id(M5Axp192),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_M5_AXP192_ID])
    var = cg.new_Pvariable(config[CONF_ID], parent)
    await output.register_output(var, config)

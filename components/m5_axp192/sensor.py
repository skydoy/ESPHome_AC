"""Battery voltage of the M5StickC Plus / Plus SE, read from the AXP192 ADC (1.1 mV/LSB).

SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""

import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_VOLTAGE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_VOLT,
)

from . import CONF_M5_AXP192_ID, M5Axp192, m5_axp192_ns

DEPENDENCIES = ["m5_axp192"]

M5Axp192Battery = m5_axp192_ns.class_("M5Axp192Battery", sensor.Sensor, cg.PollingComponent)

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        M5Axp192Battery,
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    )
    .extend({cv.GenerateID(CONF_M5_AXP192_ID): cv.use_id(M5Axp192)})
    .extend(cv.polling_component_schema("60s"))
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_M5_AXP192_ID])
    var = await sensor.new_sensor(config, parent)
    await cg.register_component(var, config)

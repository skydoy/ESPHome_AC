"""ESPHome climate platform for Hisense ACs on the RS-485 F4F5 bus.

SPDX-License-Identifier: MIT
"""

import esphome.codegen as cg
from esphome.components import binary_sensor, climate, sensor, switch, text_sensor, uart
import esphome.config_validation as cv
from esphome.const import (
    CONF_HUMIDITY,
    CONF_ID,
    CONF_VOLTAGE,
    CONF_POWER,
    CONF_CURRENT,
    DEVICE_CLASS_CONNECTIVITY,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_FREQUENCY,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_PROBLEM,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    ENTITY_CATEGORY_CONFIG,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_AMPERE,
    UNIT_CELSIUS,
    UNIT_HERTZ,
    UNIT_PERCENT,
    UNIT_VOLT,
    UNIT_WATT,
)

from . import hisense_ac_ns

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor", "switch"]

HisenseAC = hisense_ac_ns.class_("HisenseAC", climate.Climate, cg.Component, uart.UARTDevice)
HisenseDisplaySwitch = hisense_ac_ns.class_("HisenseDisplaySwitch", switch.Switch)
HisenseBeepSwitch = hisense_ac_ns.class_("HisenseBeepSwitch", switch.Switch, cg.Component)

CONF_HORIZONTAL_SWING = "horizontal_swing"
CONF_SLEEP_PROFILE = "sleep_profile"
CONF_HANDSHAKE = "handshake"
CONF_BEEP = "beep"
CONF_REPLY_TIMEOUT = "reply_timeout"
CONF_CYCLE_INTERVAL = "cycle_interval"
CONF_SPECIAL_COMMAND_GAP = "special_command_gap"

CONF_OUTDOOR_TEMPERATURE = "outdoor_temperature"
CONF_INDOOR_COIL_TEMPERATURE = "indoor_coil_temperature"
CONF_OUTDOOR_COIL_TEMPERATURE = "outdoor_coil_temperature"
CONF_COMPRESSOR_FREQUENCY = "compressor_frequency"
CONF_COMPRESSOR_TARGET_FREQUENCY = "compressor_target_frequency"
CONF_CONNECTED = "connected"
CONF_FAULT = "fault"
CONF_AUX_HEATER = "aux_heater"
CONF_FAULT_CODES = "fault_codes"
CONF_LINK_STATUS = "link_status"
CONF_DISPLAY = "display"
CONF_BEEP_SWITCH = "beep_switch"


def _temp(icon):
    return sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        icon=icon,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    )


def _freq(entity_category=cv.UNDEFINED):
    return sensor.sensor_schema(
        unit_of_measurement=UNIT_HERTZ,
        icon="mdi:sine-wave",
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_FREQUENCY,
        state_class=STATE_CLASS_MEASUREMENT,
        entity_category=entity_category,
    )


CONFIG_SCHEMA = (
    climate.climate_schema(HisenseAC)
    .extend(
        {
            cv.Optional(CONF_HORIZONTAL_SWING, default=False): cv.boolean,
            cv.Optional(CONF_SLEEP_PROFILE, default=1): cv.int_range(min=1, max=4),
            cv.Optional(CONF_HANDSHAKE, default=True): cv.boolean,
            cv.Optional(CONF_BEEP, default=True): cv.boolean,
            cv.Optional(CONF_REPLY_TIMEOUT, default="1000ms"): cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(min=cv.TimePeriod(milliseconds=200), max=cv.TimePeriod(milliseconds=3000)),
            ),
            cv.Optional(CONF_CYCLE_INTERVAL, default="1000ms"): cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(min=cv.TimePeriod(milliseconds=500), max=cv.TimePeriod(milliseconds=10000)),
            ),
            cv.Optional(CONF_SPECIAL_COMMAND_GAP, default="8s"): cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(min=cv.TimePeriod(milliseconds=1000), max=cv.TimePeriod(milliseconds=30000)),
            ),
            # Sensors
            cv.Optional(CONF_OUTDOOR_TEMPERATURE): _temp("mdi:home-thermometer-outline"),
            cv.Optional(CONF_INDOOR_COIL_TEMPERATURE): _temp("mdi:coolant-temperature"),
            cv.Optional(CONF_OUTDOOR_COIL_TEMPERATURE): _temp("mdi:coolant-temperature"),
            cv.Optional(CONF_COMPRESSOR_FREQUENCY): _freq(),
            cv.Optional(CONF_COMPRESSOR_TARGET_FREQUENCY): _freq(ENTITY_CATEGORY_DIAGNOSTIC),
            cv.Optional(CONF_HUMIDITY): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_HUMIDITY,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_POWER): sensor.sensor_schema(
                unit_of_measurement=UNIT_WATT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_POWER,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_CURRENT,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            # Binary sensors
            cv.Optional(CONF_CONNECTED): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_CONNECTIVITY,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_FAULT): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_PROBLEM,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_AUX_HEATER): binary_sensor.binary_sensor_schema(
                icon="mdi:heating-coil",
            ),
            # Text sensors
            cv.Optional(CONF_FAULT_CODES): text_sensor.text_sensor_schema(
                icon="mdi:alert-circle-outline",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_LINK_STATUS): text_sensor.text_sensor_schema(
                icon="mdi:lan-connect",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            # Switches
            cv.Optional(CONF_DISPLAY): switch.switch_schema(
                HisenseDisplaySwitch,
                icon="mdi:led-on",
                entity_category=ENTITY_CATEGORY_CONFIG,
            ),
            cv.Optional(CONF_BEEP_SWITCH): switch.switch_schema(
                HisenseBeepSwitch,
                icon="mdi:volume-high",
                entity_category=ENTITY_CATEGORY_CONFIG,
                default_restore_mode="RESTORE_DEFAULT_ON",
            ).extend(cv.COMPONENT_SCHEMA),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "hisense_ac", baud_rate=9600, require_tx=True, require_rx=True, data_bits=8, parity="NONE", stop_bits=1
)

_SENSORS = {
    CONF_OUTDOOR_TEMPERATURE: "set_outdoor_temperature_sensor",
    CONF_INDOOR_COIL_TEMPERATURE: "set_indoor_coil_temperature_sensor",
    CONF_OUTDOOR_COIL_TEMPERATURE: "set_outdoor_coil_temperature_sensor",
    CONF_COMPRESSOR_FREQUENCY: "set_compressor_frequency_sensor",
    CONF_COMPRESSOR_TARGET_FREQUENCY: "set_compressor_target_frequency_sensor",
    CONF_HUMIDITY: "set_humidity_sensor",
    CONF_VOLTAGE: "set_voltage_sensor",
    CONF_POWER: "set_power_sensor",
    CONF_CURRENT: "set_current_sensor",
}
_BINARY_SENSORS = {
    CONF_CONNECTED: "set_connected_binary_sensor",
    CONF_FAULT: "set_fault_binary_sensor",
    CONF_AUX_HEATER: "set_aux_heater_binary_sensor",
}
_TEXT_SENSORS = {
    CONF_FAULT_CODES: "set_fault_text_sensor",
    CONF_LINK_STATUS: "set_link_text_sensor",
}


async def to_code(config):
    var = await climate.new_climate(config)
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.set_horizontal_swing(config[CONF_HORIZONTAL_SWING]))
    cg.add(var.set_sleep_profile(config[CONF_SLEEP_PROFILE]))
    cg.add(var.set_handshake(config[CONF_HANDSHAKE]))
    cg.add(var.set_beep(config[CONF_BEEP]))
    cg.add(var.set_reply_timeout(config[CONF_REPLY_TIMEOUT].total_milliseconds))
    cg.add(var.set_cycle_interval(config[CONF_CYCLE_INTERVAL].total_milliseconds))
    cg.add(var.set_special_command_gap(config[CONF_SPECIAL_COMMAND_GAP].total_milliseconds))

    for key, setter in _SENSORS.items():
        if key in config:
            s = await sensor.new_sensor(config[key])
            cg.add(getattr(var, setter)(s))
    for key, setter in _BINARY_SENSORS.items():
        if key in config:
            s = await binary_sensor.new_binary_sensor(config[key])
            cg.add(getattr(var, setter)(s))
    for key, setter in _TEXT_SENSORS.items():
        if key in config:
            s = await text_sensor.new_text_sensor(config[key])
            cg.add(getattr(var, setter)(s))
    if CONF_DISPLAY in config:
        s = await switch.new_switch(config[CONF_DISPLAY])
        await cg.register_parented(s, var)
        cg.add(var.set_display_switch(s))
    if CONF_BEEP_SWITCH in config:
        s = await switch.new_switch(config[CONF_BEEP_SWITCH])
        await cg.register_component(s, config[CONF_BEEP_SWITCH])
        await cg.register_parented(s, var)
        cg.add(var.set_beep_switch(s))

"""GET /set handler for the room sensor and the solar grid-power input.

SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number, web_server_base
from esphome.components.web_server_base import CONF_WEB_SERVER_BASE_ID
from esphome.const import CONF_ID

DEPENDENCIES = ["web_server_base"]
AUTO_LOAD = ["web_server_base"]
CODEOWNERS = ["@skydoy"]

web_set_ns = cg.esphome_ns.namespace("web_set")
WebSet = web_set_ns.class_("WebSet", cg.Component)

CONF_ROOM_TEMPERATURE = "room_temperature"
CONF_GRID_POWER = "grid_power"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(WebSet),
        cv.GenerateID(CONF_WEB_SERVER_BASE_ID): cv.use_id(web_server_base.WebServerBase),
        cv.Optional(CONF_ROOM_TEMPERATURE): cv.use_id(number.Number),
        cv.Optional(CONF_GRID_POWER): cv.use_id(number.Number),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    base = await cg.get_variable(config[CONF_WEB_SERVER_BASE_ID])
    cg.add(var.set_web_server_base(base))
    if CONF_ROOM_TEMPERATURE in config:
        cg.add(var.set_room_number(await cg.get_variable(config[CONF_ROOM_TEMPERATURE])))
    if CONF_GRID_POWER in config:
        cg.add(var.set_grid_number(await cg.get_variable(config[CONF_GRID_POWER])))

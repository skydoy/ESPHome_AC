"""Setup firmware component: AC detection probe, /setup page and REST API, guided install.

Used only by the setup images (devices/setup-*.yaml). It probes the AC bus with a strict
read-only allow-list (Hisense 0x0A, 0x66/00 and 0x07 requests), keeps a raw capture for
compatibility reports, and installs the matching brand firmware over HTTPS through an
http_request update entity. It never compiles any frame that changes the AC state.

SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""

import gzip
from pathlib import Path

import esphome.codegen as cg
from esphome.components import ota, uart, update, web_server_base
from esphome.components.web_server_base import CONF_WEB_SERVER_BASE_ID
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@skydoy"]
DEPENDENCIES = ["uart", "wifi", "network"]
AUTO_LOAD = ["hisense_proto", "web_server_base"]

CONF_UART_ID = "uart_id"
CONF_UPDATE_ID = "update_id"
CONF_BOARD = "board"
CONF_FIRMWARE_VERSION = "firmware_version"
CONF_ALLOWED_ORIGIN = "allowed_firmware_origin"
CONF_PHYS = "phys"
CONF_PINS = "pins"
CONF_AUTO_START = "auto_start_delay"
CONF_PAGE_ID = "page_id"

ac_setup_ns = cg.esphome_ns.namespace("ac_setup")
AcSetup = ac_setup_ns.class_("AcSetup", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(AcSetup),
        cv.GenerateID(CONF_WEB_SERVER_BASE_ID): cv.use_id(web_server_base.WebServerBase),
        cv.Required(CONF_UART_ID): cv.use_id(uart.UARTComponent),
        cv.Required(CONF_UPDATE_ID): cv.use_id(update.UpdateEntity),
        cv.Required(CONF_BOARD): cv.string_strict,
        cv.Required(CONF_FIRMWARE_VERSION): cv.string_strict,
        cv.Optional(CONF_ALLOWED_ORIGIN, default="https://skydoy.github.io/ESPHome_AC/"): cv.url,
        cv.Optional(CONF_PHYS, default="rs485"): cv.one_of("rs485", "ttl", lower=True),
        cv.Optional(CONF_PINS, default=""): cv.string,
        cv.Optional(CONF_AUTO_START, default="10s"): cv.positive_time_period_milliseconds,
        cv.GenerateID(CONF_PAGE_ID): cv.declare_id(cg.uint8),
    }
).extend(cv.COMPONENT_SCHEMA)


def _page_gz() -> bytes:
    html = (Path(__file__).parent / "setup_page.html").read_bytes()
    # mtime=0 keeps the build reproducible.
    return gzip.compress(html, compresslevel=9, mtime=0)


async def to_code(config):
    base = await cg.get_variable(config[CONF_WEB_SERVER_BASE_ID])
    bus = await cg.get_variable(config[CONF_UART_ID])
    upd = await cg.get_variable(config[CONF_UPDATE_ID])
    var = cg.new_Pvariable(config[CONF_ID], base, bus, upd)
    await cg.register_component(var, config)

    cg.add(var.set_board(config[CONF_BOARD]))
    cg.add(var.set_firmware_version(config[CONF_FIRMWARE_VERSION]))
    cg.add(var.set_allowed_origin(config[CONF_ALLOWED_ORIGIN]))
    cg.add(var.set_phys(config[CONF_PHYS]))
    cg.add(var.set_pins(config[CONF_PINS]))
    cg.add(var.set_auto_start_delay(config[CONF_AUTO_START]))

    page = _page_gz()
    arr = cg.progmem_array(config[CONF_PAGE_ID], list(page))
    cg.add(var.set_page(arr, len(page)))

    # Install progress comes from the OTA global state listener.
    ota.request_ota_state_listeners()

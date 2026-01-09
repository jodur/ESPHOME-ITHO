import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import spi
from esphome.const import CONF_ID
from esphome import pins

CODEOWNERS = ["@jodur"]
DEPENDENCIES = []
AUTO_LOAD = ["fan"]

CONF_DEVICE_ID = "device_id"
CONF_DEVICE_NAME = "device_name"
CONF_REMOTE_IDS = "remote_ids"
CONF_REMOTE_ID = "remote_id"
CONF_ROOM_NAME = "room_name"
CONF_PIN_INTERRUPT = "interrupt_pin"

itho_fan_ns = cg.esphome_ns.namespace("itho_fan")
IthoFanHub = itho_fan_ns.class_("IthoFanHub", cg.Component)

REMOTE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_REMOTE_ID): cv.string,
        cv.Required(CONF_ROOM_NAME): cv.string,
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(IthoFanHub),
        cv.Required(CONF_DEVICE_ID): cv.string,
        cv.Optional(CONF_DEVICE_NAME, default="ESPHome"): cv.string,
        cv.Optional(CONF_REMOTE_IDS, default=[]): cv.ensure_list(REMOTE_SCHEMA),
        cv.Required(CONF_PIN_INTERRUPT): pins.gpio_input_pin_schema,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Set device ID
    device_id_parts = config[CONF_DEVICE_ID].split(",")
    if len(device_id_parts) == 3:
        cg.add(
            var.set_device_id(
                int(device_id_parts[0]),
                int(device_id_parts[1]),
                int(device_id_parts[2]),
            )
        )
    
    # Set device name
    cg.add(var.set_device_name(config[CONF_DEVICE_NAME]))

    # Set remote IDs
    for remote in config[CONF_REMOTE_IDS]:
        cg.add(var.add_remote_id(remote[CONF_REMOTE_ID], remote[CONF_ROOM_NAME]))

    # Set interrupt pin
    interrupt_pin = await cg.gpio_pin_expression(config[CONF_PIN_INTERRUPT])
    cg.add(var.set_interrupt_pin(interrupt_pin))

    # Add library dependencies
    # Note: SPI is standard Arduino library
    # ITHO-Lib is pulled from GitHub during compilation
    cg.add_library("SPI", None)
    cg.add_library("https://github.com/jodur/ITHO-Lib#NewLib", None)

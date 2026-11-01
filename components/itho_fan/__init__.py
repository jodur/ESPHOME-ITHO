import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cc1101
from esphome.const import CONF_ID

CODEOWNERS = ["@jodur"]
DEPENDENCIES = ["cc1101"]
AUTO_LOAD = ["fan", "sensor", "text_sensor"]

CONF_DEVICE_ID = "device_id"
CONF_DEVICE_NAME = "device_name"
CONF_REMOTE_IDS = "remote_ids"
CONF_REMOTE_ID = "remote_id"
CONF_ROOM_NAME = "room_name"
CONF_CC1101_ID = "cc1101_id"

itho_fan_ns = cg.esphome_ns.namespace("itho_fan")
IthoFanHub = itho_fan_ns.class_("IthoFanHub", cg.Component)


def validate_device_id(value):
    """Device ID: 3 decimal bytes, e.g. "10,87,81"."""
    value = cv.string(value)
    parts = value.split(",")
    if len(parts) != 3:
        raise cv.Invalid("device_id must be 3 comma separated numbers, e.g. \"10,87,81\"")
    for part in parts:
        cv.int_range(min=0, max=255)(cv.int_(part.strip()))
    return value


REMOTE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_REMOTE_ID): cv.string,
        cv.Required(CONF_ROOM_NAME): cv.string,
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(IthoFanHub),
        cv.Required(CONF_DEVICE_ID): validate_device_id,
        cv.Optional(CONF_DEVICE_NAME, default="ESPHome"): cv.string,
        cv.Optional(CONF_REMOTE_IDS, default=[]): cv.ensure_list(REMOTE_SCHEMA),
        cv.GenerateID(CONF_CC1101_ID): cv.use_id(cc1101.CC1101Component),
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

    # Use the official ESPHome cc1101 component as radio
    radio = await cg.get_variable(config[CONF_CC1101_ID])
    cg.add(var.set_radio(radio))

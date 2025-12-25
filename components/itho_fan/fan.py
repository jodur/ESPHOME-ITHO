import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan
from esphome.const import CONF_OUTPUT_ID, CONF_ID
from . import itho_fan_ns, IthoFanHub, CONF_DEVICE_ID

DEPENDENCIES = ["itho_fan"]

IthoFan = itho_fan_ns.class_("IthoFan", cg.Component, fan.Fan)

CONFIG_SCHEMA = fan.FAN_SCHEMA.extend(
    {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(IthoFan),
        cv.GenerateID(CONF_ID): cv.use_id(IthoFanHub),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await fan.register_fan(var, config)

    hub = await cg.get_variable(config[CONF_ID])
    cg.add(var.set_hub(hub))

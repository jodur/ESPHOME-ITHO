import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan
from esphome.const import CONF_ID
from . import itho_fan_ns, IthoFanHub

DEPENDENCIES = ["itho_fan"]
CONF_ITHO_HUB_ID = "itho_hub_id"

IthoFan = itho_fan_ns.class_("IthoFan", cg.Component, fan.Fan)

CONFIG_SCHEMA = cv.All(
    fan.fan_schema(IthoFan).extend(
        {
            cv.GenerateID(CONF_ITHO_HUB_ID): cv.use_id(IthoFanHub),
        }
    ).extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await fan.new_fan(config)
    await cg.register_component(var, config)

    hub = await cg.get_variable(config[CONF_ITHO_HUB_ID])
    cg.add(var.set_hub(hub))
    cg.add(hub.register_fan(var))

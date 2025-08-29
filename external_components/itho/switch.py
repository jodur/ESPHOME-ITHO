# ./components/itho/switch.py

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID, CONF_NAME, CONF_TYPE

CODEOWNERS = ["@jodur"]
DEPENDENCIES = []

itho_ns = cg.esphome_ns.namespace("itho")

FanSendLow = itho_ns.class_("FanSendLow", switch.Switch, cg.Component)
FanSendMedium = itho_ns.class_("FanSendMedium", switch.Switch, cg.Component)
FanSendHigh = itho_ns.class_("FanSendHigh", switch.Switch, cg.Component)
FanSendFull = itho_ns.class_("FanSendFull", switch.Switch, cg.Component)
FanSendIthoTimer1 = itho_ns.class_("FanSendIthoTimer1", switch.Switch, cg.Component)
FanSendIthoTimer2 = itho_ns.class_("FanSendIthoTimer2", switch.Switch, cg.Component)
FanSendIthoTimer3 = itho_ns.class_("FanSendIthoTimer3", switch.Switch, cg.Component)
FanSendIthoJoin = itho_ns.class_("FanSendIthoJoin", switch.Switch, cg.Component)

TYPES = {
    "fan_send_low": FanSendLow,
    "fan_send_medium": FanSendMedium,
    "fan_send_high": FanSendHigh,
    "fan_send_full": FanSendFull,
    "fan_send_timer1": FanSendIthoTimer1,
    "fan_send_timer2": FanSendIthoTimer2,
    "fan_send_timer3": FanSendIthoTimer3,
    "fan_send_join": FanSendIthoJoin,
}

# Per-type schemas
typed_schemas = {
    key: switch.SWITCH_SCHEMA.extend({
        cv.GenerateID(): cv.declare_id(cls),
    })
    for key, cls in TYPES.items()
}

CONFIG_SCHEMA = cv.typed_schema(
    typed_schemas,
    key=CONF_TYPE,
)

async def to_code(config):
    var_cls = TYPES[config[CONF_TYPE]]
    var = cg.new_Pvariable(config[CONF_ID], var_cls())
    await cg.register_component(var, config)
    await switch.register_switch(var, config)

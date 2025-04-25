# ./components/itho/text_sensor.py

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, CONF_NAME, CONF_TYPE

CODEOWNERS = ["@jodur"]
DEPENDENCIES = []

# Custom keys for sub-sensors
CONF_FANSPEED_ID = "fanspeed_id"
CONF_FANTIMER_ID = "fantimer_id"
CONF_LASTID_ID = "lastid_id"

itho_ns = cg.esphome_ns.namespace("itho")
FanRecv = itho_ns.class_("FanRecv", text_sensor.TextSensor, cg.PollingComponent)

TYPES = {
    "fan_recv": FanRecv,
}

# Per-type schemas
typed_schemas = {
    key: cv.Schema({
        cv.GenerateID(): cv.declare_id(cls),
        cv.Required(CONF_NAME): cv.string,
        cv.Required(CONF_FANSPEED_ID): cv.use_id(text_sensor.TextSensor),
        cv.Required(CONF_FANTIMER_ID): cv.use_id(text_sensor.TextSensor),
        cv.Required(CONF_LASTID_ID): cv.use_id(text_sensor.TextSensor),
    })
    for key, cls in TYPES.items()
}
# typed_schemas = {
#     key: text_sensor.text_sensor_schema(FanRecv).extend({
#         cv.GenerateID(): cv.declare_id(cls),
#     })
#     for key, cls in TYPES.items()
# }

CONFIG_SCHEMA = cv.typed_schema(
    typed_schemas,
    key=CONF_TYPE,
    default_type="fan_recv"
)

# CONFIG_SCHEMA = cv.typed_schema(
#     {
#         "fan_recv": text_sensor.text_sensor_schema(FanRecv).extend({
#             cv.GenerateID(): cv.declare_id(FanRecv),
#         })
#     },
#     key=CONF_TYPE,
#     default_type="fan_recv"
# )

ICON = "icon"

async def to_code(config):
#    var_cls = TYPES[config[CONF_TYPE]]
#    var = cg.new_Pvariable(config[CONF_ID], var_cls())
#    await cg.register_component(var, config)
#    await text_sensor.register_text_sensor(var, config)
    
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    fanspeed = await cg.get_variable(config[CONF_FANSPEED_ID])
    fantimer = await cg.get_variable(config[CONF_FANTIMER_ID])
    lastid = await cg.get_variable(config[CONF_LASTID_ID])

    cg.add(var.set_fanspeed(fanspeed))
    cg.add(var.set_fantimer(fantimer))
    cg.add(var.set_lastid(lastid))

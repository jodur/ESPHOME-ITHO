# ./components/itho/__init__.py

# import esphome.codegen as cg
# import esphome.config_validation as cv
# from esphome.components import switch, text_sensor
# from esphome.const import CONF_ID
# 
# CODEOWNERS = ["@jodur"]
# DEPENDENCIES = []
# 
# itho_ns = cg.esphome_ns.namespace("itho")
# 
# FanRecv = itho_ns.class_("FanRecv", cg.PollingComponent)
# FanSendLow = itho_ns.class_("FanSendLow", switch.Switch, cg.Component)
# FanSendMedium = itho_ns.class_("FanSendMedium", switch.Switch, cg.Component)
# FanSendHigh = itho_ns.class_("FanSendHigh", switch.Switch, cg.Component)
# 
# CONFIG_SCHEMA = cv.Schema({})
# 
# async def to_code(config):
#     recv = cg.new_Pvariable("fan_recv", FanRecv())
#     await cg.register_component(recv, {})
# 
#     low = cg.new_Pvariable("fan_send_low", FanSendLow())
#     await cg.register_component(low, {})
#     await switch.register_switch(low, {})
# 
#     medium = cg.new_Pvariable("fan_send_medium", FanSendMedium())
#     await cg.register_component(medium, {})
#     await switch.register_switch(medium, {})
# 
#     high = cg.new_Pvariable("fan_send_high", FanSendHigh())
#     await cg.register_component(high, {})
#     await switch.register_switch(high, {})
# 
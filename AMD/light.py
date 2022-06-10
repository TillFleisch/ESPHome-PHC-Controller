import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch, light
from esphome.const import CONF_OUTPUT_ID, CONF_NAME
from ..PHCController import CONTROLLER_ID, PHCController

DEPENDENCIES = ['PHCController']

ADDRESS = 'dip'
CHANNEL = 'channel'

amd_light_ns = cg.esphome_ns.namespace('AMD_binary')
AMDLight = amd_light_ns.class_(
    'AMD', switch.Switch, light.LightOutput, cg.Component)

CONFIG_SCHEMA = light.BINARY_LIGHT_SCHEMA.extend({
    cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(AMDLight),
    cv.Required(CONF_NAME): cv.string,
    cv.Required(CONTROLLER_ID): cv.use_id(PHCController),
    cv.Required(ADDRESS): cv.int_,
    cv.Required(CHANNEL): cv.int_
}).extend(cv.COMPONENT_SCHEMA)


def to_code(config):
    controller = yield cg.get_variable(config[CONTROLLER_ID])
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    yield cg.register_component(var, config)
    yield light.register_light(var, config)

    cg.add(controller.register_AMD(var))
    cg.add(var.set_address(config[ADDRESS]))
    cg.add(var.set_channel(config[CHANNEL]))
    cg.add(var.set_name(config[CONF_NAME]))

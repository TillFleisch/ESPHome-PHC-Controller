from typing_extensions import Required
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID
from ..PHCController import CONTROLLER_ID, PHCController

DEPENDENCIES = ['PHCController']

DEVICE_TYPE = 'device_type'
ADDRESS = 'dip'
CHANNEL = 'channel'

AMD_ns = cg.esphome_ns.namespace('AMD_switch')
AMD = AMD_ns.class_('AMD', switch.Switch, cg.Component)

CONFIG_SCHEMA = switch.SWITCH_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(AMD),
    cv.Optional(DEVICE_TYPE, default="switch"): cv.string,
    cv.Required(CONTROLLER_ID): cv.use_id(PHCController),
    cv.Required(ADDRESS): cv.int_,
    cv.Required(CHANNEL): cv.int_
}).extend(cv.COMPONENT_SCHEMA)


def to_code(config):
    controller = yield cg.get_variable(config[CONTROLLER_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield switch.register_switch(var, config)

    cg.add(controller.register_AMD(var))
    cg.add(var.set_address(config[ADDRESS]))
    cg.add(var.set_channel(config[CHANNEL]))

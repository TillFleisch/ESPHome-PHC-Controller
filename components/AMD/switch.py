import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import light, switch
from esphome.const import CONF_DEVICE_CLASS, CONF_ID, DEVICE_CLASS_OUTLET

from ..PHCController import CONTROLLER_ID, PHCController

DEPENDENCIES = ["PHCController"]

ADDRESS = "dip"
CHANNEL = "channel"

AMD_ns = cg.esphome_ns.namespace("AMD_binary")
AMD = AMD_ns.class_("AMD_switch", switch.Switch, cg.Component)

CONFIG_SCHEMA = switch.SWITCH_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(AMD),
        cv.Optional(CONF_DEVICE_CLASS, default=DEVICE_CLASS_OUTLET): cv.string,
        cv.Required(CONTROLLER_ID): cv.use_id(PHCController),
        cv.Required(ADDRESS): cv.int_range(min=0, max=31),
        cv.Required(CHANNEL): cv.int_range(min=0, max=7),
    }
).extend(cv.COMPONENT_SCHEMA)


def to_code(config):
    controller = yield cg.get_variable(config[CONTROLLER_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield switch.register_switch(var, config)

    cg.add(var.set_address(config[ADDRESS]))
    cg.add(var.set_channel(config[CHANNEL]))
    cg.add(controller.register_AMD(var))

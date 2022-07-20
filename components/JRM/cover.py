import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import cover
from esphome.const import CONF_ID, DEVICE_CLASS_OUTLET, CONF_DEVICE_CLASS
from ..PHCController import CONTROLLER_ID, PHCController

JRM_cover_ns = cg.esphome_ns.namespace('JRM_cover')
JRMCover = JRM_cover_ns.class_('JRM', cover.Cover, cg.Component)

ADDRESS = 'dip'
CHANNEL = 'channel'

CONFIG_SCHEMA = cover.COVER_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(JRMCover),
    cv.Optional(CONF_DEVICE_CLASS, default=DEVICE_CLASS_OUTLET): cv.string,
    cv.Required(CONTROLLER_ID): cv.use_id(PHCController),
    cv.Required(ADDRESS): cv.int_,
    cv.Required(CHANNEL): cv.int_
}).extend(cv.COMPONENT_SCHEMA)


def to_code(config):
    controller = yield cg.get_variable(config[CONTROLLER_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield cover.register_cover(var, config)

    cg.add(controller.register_JRM(var))
    cg.add(var.set_address(config[ADDRESS]))
    cg.add(var.set_channel(config[CHANNEL]))

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover
from esphome.const import CONF_ID
from ..PHCController import CONTROLLER_ID, PHCController

JRM_cover_ns = cg.esphome_ns.namespace('JRM_cover')
JRMCover = JRM_cover_ns.class_('JRM', cover.Cover, cg.Component)

ADDRESS = 'dip'
CHANNEL = 'channel'
MAX_CLOSE_TIME = 'max_close_time'
MAX_OPEN_TIME = 'max_open_time'

CONFIG_SCHEMA = cover.COVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(JRMCover),
        cv.Required(CONTROLLER_ID): cv.use_id(PHCController),
        cv.Required(ADDRESS): cv.int_range(min=0, max=31),
        cv.Required(CHANNEL): cv.int_range(min=0, max=3),
        cv.Optional(MAX_CLOSE_TIME, default="30s"): cv.All(
            cv.positive_time_period_milliseconds,
            cv.Range(
                min=cv.TimePeriod(milliseconds=0),
                max=cv.TimePeriod(milliseconds=655350),
            ),
        ),
        cv.Optional(MAX_OPEN_TIME, default="30s"): cv.All(
            cv.positive_time_period_milliseconds,
            cv.Range(
                min=cv.TimePeriod(milliseconds=0),
                max=cv.TimePeriod(milliseconds=655350),
            ),
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


def to_code(config):
    controller = yield cg.get_variable(config[CONTROLLER_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield cover.register_cover(var, config)

    cg.add(controller.register_JRM(var))
    cg.add(var.set_address(config[ADDRESS]))
    cg.add(var.set_channel(config[CHANNEL]))
    cg.add(var.set_max_close_time(config[MAX_CLOSE_TIME]))
    cg.add(var.set_max_open_time(config[MAX_OPEN_TIME]))

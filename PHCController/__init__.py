from typing_extensions import Required
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.components.uart import UARTComponent

from esphome.const import CONF_ID

DEPENDENCIES = ['uart']

CONTROLLER_ID = 'phc_controller_id'


phc_controller_ns = cg.esphome_ns.namespace('phc_controller')
PHCController = phc_controller_ns.class_(
    'PHCController', cg.Component, uart.UARTDevice)

CONFIG_SCHEMA = uart.UART_DEVICE_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(PHCController),
    cv.Required("uart_id"): cv.use_id(UARTComponent)
})


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield uart.register_uart_device(var, config)

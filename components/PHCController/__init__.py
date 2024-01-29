import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import uart
from esphome.components.uart import UARTComponent
from esphome.const import CONF_ID, CONF_PIN

AUTO_LOAD = ["cover", "light", "switch", "AMD", "EMD", "JRM"]

DEPENDENCIES = ["uart"]

CONTROLLER_ID = "phc_controller_id"
UART_ID = "uart_id"
FLOW_CONTROL_PIN = "flow_control_pin"


phc_controller_ns = cg.esphome_ns.namespace("phc_controller")
PHCController = phc_controller_ns.class_("PHCController", cg.Component, uart.UARTDevice)

CONFIG_SCHEMA = uart.UART_DEVICE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(PHCController),
        cv.Required(UART_ID): cv.use_id(UARTComponent),
        cv.Optional(FLOW_CONTROL_PIN): pins.gpio_output_pin_schema,
    }
)


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield uart.register_uart_device(var, config)

    if FLOW_CONTROL_PIN in config:
        pin = yield cg.gpio_pin_expression(config[FLOW_CONTROL_PIN])
        cg.add(var.set_flow_control_pin(pin))

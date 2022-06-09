#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/uart/uart.h"
#include "../PHCController/util.h"

namespace esphome
{
  namespace AMD_switch
  {

    class AMD : public switch_::Switch, public Component
    {
    public:
      void setup() override;
      void loop() override;
      void write_state(bool state) override;
      void dump_config() override;
      void set_address(uint8_t address) { this->address = address; }
      void set_channel(uint8_t channel) { this->channel = channel; }
      void set_uart_device(uart::UARTDevice *uart_device) { this->uart_device = uart_device; };

      uint8_t get_address() { return this->address; }
      uint8_t get_channel() { return this->channel; }

    private:
      uint8_t address;
      uint8_t channel;
      uart::UARTDevice *uart_device;
    };

  } // namespace AMD_switch
} // namespace esphome
#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/uart/uart.h"
#include "../PHCController/util.h"

namespace esphome
{
  namespace AMD_binary
  {

    class AMD : public switch_::Switch, public Component, public light::LightOutput
    {
    public:
      void setup() override;
      void loop() override;
      void write_state(bool state) override;
      void write_state(light::LightState *state) override
      {
        bool binary;
        state->current_values_as_binary(&binary);
        write_state(binary);
      }

      void dump_config() override;

      light::LightTraits get_traits() override
      {
        auto traits = light::LightTraits();
        traits.set_supported_color_modes({light::ColorMode::ON_OFF});
        return traits;
      }

      void set_address(uint8_t address) { this->address = address; }
      void set_channel(uint8_t channel) { this->channel = channel; }
      void set_uart_device(uart::UARTDevice *uart_device) { this->uart_device = uart_device; };

      uint8_t get_address() { return this->address; }
      uint8_t get_channel() { return this->channel; }

    private:
      bool target_state = false;
      long int last_request = 0;
      int resend_counter = 0;

      uint8_t address;
      uint8_t channel;
      uart::UARTDevice *uart_device;
    };

  } // namespace AMD_switch
} // namespace esphome
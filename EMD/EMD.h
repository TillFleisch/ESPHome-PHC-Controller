#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"

namespace esphome
{
  namespace EMD_switch
  {

    class EMD : public switch_::Switch, public Component
    {
    public:
      void setup() override;
      void write_state(bool state) override;
      void dump_config() override;
      void set_address(uint8_t address) { this->address = address; }
      void set_channel(uint8_t channel) { this->channel = channel; }
      uint8_t get_address() { return this->address; }
      uint8_t get_channel() { return this->channel; }

    private:
      uint8_t address;
      uint8_t channel;
    };

  } // namespace EMD_switch
} // namespace esphome
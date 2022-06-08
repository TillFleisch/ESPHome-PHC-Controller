#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"

namespace esphome
{
  namespace AMD_switch
  {

    class AMD : public switch_::Switch, public Component
    {
    public:
      void setup() override;
      void write_state(bool state) override;
      void dump_config() override;
      void set_address(uint8_t address) { this->address = address; }
      uint8_t get_address() { return this->address; }

    private:
      uint8_t address;
    };

  } // namespace AMD_switch
} // namespace esphome
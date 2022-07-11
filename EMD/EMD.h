#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "../PHCController/util.h"

namespace esphome
{
  namespace EMD_switch
  {

    class EMD : public util::Module, public switch_::Switch, public Component
    {
    public:
      void setup() override;
      void write_state(bool state) override;
      void dump_config() override;
      uint8_t get_device_class_id(){return EMD_MODULE_ADDRESS;};
    };

  } // namespace EMD_switch
} // namespace esphome
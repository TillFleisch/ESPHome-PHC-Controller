#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/light/light_output.h"
#include "../PHCController/util.h"

namespace esphome
{
  namespace AMD_binary
  {

    class AMD : public util::Module, public switch_::Switch, public Component, public light::LightOutput
    {
    public:
      void setup() override;
      void loop() override;
      uint8_t get_device_class_id(){return AMD_MODULE_ADDRESS;};
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

    private:
      bool target_state = false;
      long int last_request = 0;
      int resend_counter = 0;
    };

  } // namespace AMD_switch
} // namespace esphome
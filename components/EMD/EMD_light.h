#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/light_output.h"
#include "../PHCController/util.h"

namespace esphome
{
  namespace EMD_light
  {

    class EMD_light : public util::Module, public EntityBase, public Component, public light::LightOutput
    {
    public:
      void setup() override;
      void loop() override;
      void sync_state() override { this->write_state(get_state()); };
      uint8_t get_device_class_id() { return EMD_MODULE_ADDRESS; };
      void write_state(bool state);
      void write_state(light::LightState *state) override
      {
        light_state_ = state;
        bool binary;
        state->current_values_as_binary(&binary);
        write_state(binary);
      }

      void publish_state(bool state)
      {
        state_ = state;
        if (light_state_ != NULL)
        {
          light_state_->remote_values.set_state(state);
          light_state_->publish_state();
        }
      }

      bool get_state()
      {
        return state_;
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
      bool state_ = false;
      light::LightState *light_state_ = NULL;
    };

  } // namespace EMD_light
} // namespace esphome
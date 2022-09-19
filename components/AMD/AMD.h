#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/light/light_output.h"
#include "../PHCController/util.h"

namespace esphome
{
  namespace AMD_binary
  {

    class AMD : public util::Module, public Component
    {
    public:
      void setup() override;
      void loop() override;
      void sync_state() override { write_state(get_state()); };
      uint8_t get_device_class_id() { return AMD_MODULE_ADDRESS; };
      void write_state(bool state);
      void dump_config() override;
      virtual void publish_state(bool state){};
      virtual bool get_state() { return false; };

    private:
      bool target_state = false;
      long int last_request = 0;
      int resend_counter = 0;
    };

    class AMD_switch : public AMD, public switch_::Switch
    {
    public:
      bool get_state() override
      {
        return state;
      }

      void publish_state(bool state) override
      {
        switch_::Switch::publish_state(state);
      };

      void write_state(bool state) override
      {
        AMD::write_state(state);
      }
    };

    class AMD_light : public AMD, public light::LightOutput
    {
    public:
      void write_state(light::LightState *state) override
      {
        light_state_ = state;
        bool binary;
        state->current_values_as_binary(&binary);
        AMD::write_state(binary);
      }

      light::LightTraits get_traits() override
      {
        auto traits = light::LightTraits();
        traits.set_supported_color_modes({light::ColorMode::ON_OFF});
        return traits;
      }

      bool get_state() override
      {
        return state_;
      }

      void publish_state(bool state) override
      {
        state_ = state;
        if (light_state_ != NULL)
        {
          light_state_->remote_values.set_state(state);
          light_state_->publish_state();
        }
      }

    private:
      bool state_ = false;
      light::LightState *light_state_ = NULL;
    };
  } // namespace AMD_switch
} // namespace esphome
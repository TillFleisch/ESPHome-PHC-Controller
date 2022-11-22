#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/light_output.h"
#include "../PHCController/util.h"
#include <random>

namespace esphome
{
  namespace EMD_light
  {

    /**
     * @brief Light entity implementation for led lights on EMD modules.
     *
     */
    class EMD_light : public util::Module, public EntityBase, public Component, public light::LightOutput
    {
    public:
      void setup() override;
      void loop() override;
      void sync_state() override { write_state(get_state()); };
      uint8_t get_device_class_id() { return EMD_MODULE_ADDRESS; };
      void dump_config() override;
      light::LightTraits get_traits() override
      {
        auto traits = light::LightTraits();
        traits.set_supported_color_modes({light::ColorMode::ON_OFF});
        return traits;
      }

      /**
       * @brief Write a boolean state to this entity which should be propagated to hardware
       *
       * @param state new State the entity should write to hardware
       */
      void write_state(bool state);

      /**
       * @brief Propagates a light state to the boolean write state method
       *
       * @param state
       */
      void write_state(light::LightState *state) override
      {
        light_state_ = state;
        bool binary;
        state->current_values_as_binary(&binary);
        write_state(binary);
      }

      void publish_state(bool state)
      {
        // Publish the new state using the last known light_state
        state_ = state;
        if (light_state_ != NULL)
        {
          light_state_->remote_values.set_state(state);
          light_state_->publish_state();
        }
      }

      bool get_state()
      {
        // Return the private public state
        return state_;
      }

    private:
      /**
       * @brief The entities target state which should be reached within the allowed retry time/count
       *
       */
      bool target_state = false;
      /**
       * @brief Timestamp of the last request sent by this entity
       *
       */
      long int last_request = 0;

      /**
       * @brief Resend counter for non-acknowledged messages
       *
       */
      int resend_counter = 0;
      /**
       * @brief The public state of this light entity
       *
       */
      bool state_ = false;
      /**
       * @brief Last known light state
       *
       */
      light::LightState *light_state_ = NULL;
    };

  } // namespace EMD_light
} // namespace esphome
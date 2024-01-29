#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/light/light_output.h"
#include "../PHCController/util.h"
#include <random>

namespace esphome
{
    namespace AMD_binary
    {

        /**
         * @brief Basic binary AMD class
         *
         */
        class AMD : public util::Module, public Component
        {
        public:
            void setup() override;
            void loop() override;
            void sync_state() override
            {
                write_state(get_state());
            };
            uint8_t get_device_class_id()
            {
                return AMD_MODULE_ADDRESS;
            };

            /**
             * @brief Write a boolean state to this entity which should be propagated to hardware
             *
             * @param state new State the entity should write to hardware
             */
            void write_state(bool state);
            void dump_config() override;

            /**
             * @brief Publish the entities state.
             *
             * @param state the new state to publish
             */
            virtual void publish_state(bool state){};

            /**
             * @brief Get the current state of this entity
             *
             * @return true if the (public) output is currently active
             * @return false if the (public) output is currently inactive
             */
            virtual bool get_state()
            {
                return false;
            };

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
        };

        /**
         * @brief Switch implementation of a binary AMD entity.
         * This class is used for outlets and other switchable outputs.
         *
         */
        class AMD_switch : public AMD, public switch_::Switch
        {
        public:
            bool get_state() override
            {
                // Return the switch component's state as the current public output state
                return state;
            }

            void publish_state(bool state) override
            {
                // publish the state using the switch component
                switch_::Switch::publish_state(state);
            };

            void write_state(bool state) override
            {
                AMD::write_state(state);
            }
        };

        /**
         * @brief Light implementation of a binary AMD entity.
         * This class is used for binary lights
         */
        class AMD_light : public AMD, public light::LightOutput
        {
        public:
            light::LightTraits get_traits() override
            {
                auto traits = light::LightTraits();
                traits.set_supported_color_modes({light::ColorMode::ON_OFF});
                return traits;
            }

            bool get_state() override
            {
                // Return the private public state
                return state_;
            }

            void publish_state(bool state) override
            {
                // Publish the new state using the last known light_state
                state_ = state;
                if (light_state_ != NULL)
                {
                    light_state_->remote_values.set_state(state);
                    light_state_->publish_state();
                }
            }

            /**
             * @brief Propagates a light state to the binary AMD entity
             *
             * @param state
             */
            void write_state(light::LightState *state) override
            {
                light_state_ = state;
                bool binary;
                state->current_values_as_binary(&binary);
                AMD::write_state(binary);
            }

        private:
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
    } // namespace AMD_binary
} // namespace esphome
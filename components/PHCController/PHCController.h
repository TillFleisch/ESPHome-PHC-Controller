#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "util.h"
#include "../AMD/AMD.h"
#include "../EMD/EMD.h"
#include "../EMD/EMD_light.h"
#include "../JRM/JRM.h"

#define FLOW_PIN_PULL_HIGH_DELAY 0
#define FLOW_PIN_PULL_LOW_DELAY 0

#define TIMING_DELAY 150 // 250us on original PHC
// This value has been adjusted, such that the measured delay is roughly equal to 250us. This has changed with arduino core 2.0+
#define INITIAL_SYNC_DELAY 15

namespace esphome
{
    namespace phc_controller
    {
        class PHCController : public uart::UARTDevice, public Component
        {
        public:
            void
            setup() override;
            void loop() override;
            void dump_config() override;
            /**
             * @brief Writes a array of bytes to the PHC-bus using the parent controller.
             *
             * @param data Data to write to the bus
             * @param len Length of the given data
             * @param allow_weak_operation If true writing to the bus is not guaranteed
             */
            void write_array(const uint8_t *data, size_t len, bool allow_weak_operation);
            float get_setup_priority() const override
            {
                return setup_priority::HARDWARE;
            }

            /**
             * @brief Set the flow control pin used by this controller
             *
             * @param pin GPIOPin used for flow control
             */
            void set_flow_control_pin(GPIOPin *pin)
            {
                flow_control_pin_ = pin;
            };

            /**
             * @brief Get the ToggleMap used by this controller
             *
             * @return util::ToggleMap*
             */
            util::ToggleMap *getToggleMap()
            {
                return toggle_map;
            };

            /**
             * @brief Register a AMD entity on this controller
             *
             * @param obj A AMD entity
             */
            void register_AMD(AMD_binary::AMD *obj)
            {
                amds_[obj->get_key()] = obj;
                obj->set_controller(this);
            }

            /**
             * @brief Register a EMD (binary_sensor) entity on this controller
             *
             * @param obj A EMD entity
             */
            void register_EMD(EMD_binary_sensor::EMD *obj)
            {
                emds_[obj->get_key()] = obj;
                obj->set_controller(this);
            }

            /**
             * @brief Register a EMD-Light entity on this controller
             *
             * @param obj A EMD-Light entity
             */
            void register_EMDLight(EMD_light::EMD_light *obj)
            {
                emd_lights_[obj->get_key()] = obj;
                obj->set_controller(this);
            }

            /**
             * @brief Register a JRM entity on this controller
             *
             * @param obj A JRM entity
             */
            void register_JRM(JRM_cover::JRM *obj)
            {
                // Initialize resource lock with -1 (free)
                jrm_resource_lock_[obj->get_address()] = -1;
                jrms_[obj->get_key()] = obj;
                obj->set_controller(this);
            }

            /**
             * @brief Frees a JRM resource lock without any checks
             *
             * @param address Lock to clear
             */
            void free_jrm_lock(uint8_t address)
            {
                jrm_resource_lock_[address] = -1;
            }

            /**
             * @brief Acquires the a resource lock of a module for a channel
             *
             * @param address Module for which a lock should be acquired
             * @param channel Chanel which should acquire the lock
             * @return true If the lock was acquired
             * @return false If the lock is in use
             */
            bool acquire_jrm_lock(uint8_t address, uint8_t channel)
            {
                // Check if resource for lock is free & acquire for channel
                if (jrm_resource_lock_[address] == -1)
                {
                    jrm_resource_lock_[address] = channel;
                }

                return jrm_resource_lock_[address] == channel;
            }

        protected:
            /**
             * @brief Processes a valid command received from the bus and makes necessary changes to registered entities.
             *
             * @param device_class_id The device class id from the message
             * @param toggle The status of the toggle bit.
             * @param message The message content
             * @param length The length of content
             */
            void process_command(uint8_t *device_class_id, bool toggle, uint8_t *message, uint8_t *length);

            /**
             * @brief Write a acknowledgement for a specific module (address) to the bus
             *
             * @param address The modules address to acknowledge
             * @param toggle The status of the toggle bit.
             */
            void send_acknowledgement(uint8_t address, bool toggle);

            /**
             * @brief Write a configuration message for a AMD/JRM module to the bus.
             *
             * @param address The address of the module
             */
            void send_amd_config(uint8_t address);

            /**
             * @brief Write a configuration message for a EMD module to the bus.
             *
             * @param address The address of the module
             */
            void send_emd_config(uint8_t address);

            /**
             * @brief Set the up known modules by pre-writing configuration messages to the bus.
             *
             */
            void setup_known_modules();
            void sync_states();

            HighFrequencyLoopRequester high_freq_;

            /**
             * @brief The flow control pin used by this controller
             *
             */
            GPIOPin *flow_control_pin_;

            /**
             * @brief The toggle map used by this controller
             *
             */
            util::ToggleMap *toggle_map = new util::ToggleMap();

            /**
             * @brief A Map of available AMD entities
             *
             */
            std::map<uint16_t, AMD_binary::AMD *> amds_;

            /**
             * @brief A Map of available EMD entities
             *
             */
            std::map<uint16_t, EMD_binary_sensor::EMD *> emds_;

            /**
             * @brief A Map of available EMD_light entities
             *
             */
            std::map<uint16_t, EMD_light::EMD_light *> emd_lights_;

            /**
             * @brief A Map of available JRM entities
             *
             */
            std::map<uint16_t, JRM_cover::JRM *> jrms_;

            /**
             * @brief JRM resource lock. The key is the module identitier and the value is channel currently owning it.
             * -1 if it is not owned.
             */
            std::map<uint8_t, int8_t> jrm_resource_lock_;

            /**
             * @brief Time since the last message has been received.
             *
             */
            long last_message_time_ = 0;

            /**
             * @brief Determines if states have been synced on start-up.
             *
             */
            bool states_synced_ = false;
        };

    } // namespace phc_controller
} // namespace esphome

inline void util::Module::set_controller(esphome::phc_controller::PHCController *controller)
{
    this->controller = controller;
    toggle_map = controller->getToggleMap();
}

inline void util::Module::write_array(esphome::phc_controller::PHCController *controller, const uint8_t *data, size_t len, bool allow_weak_operation)
{
    controller->write_array(data, len, allow_weak_operation);
}

inline bool util::Module::acquire_jrm_lock(esphome::phc_controller::PHCController *controller, uint8_t address, uint8_t channel)
{
    return controller->acquire_jrm_lock(address, channel);
}

inline void util::Module::free_jrm_lock(esphome::phc_controller::PHCController *controller, uint8_t address)
{
    controller->free_jrm_lock(address);
}

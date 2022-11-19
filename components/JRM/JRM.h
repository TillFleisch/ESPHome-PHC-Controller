#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cover/cover.h"
#include "../PHCController/util.h"

// Position publish frequency in milliseconds
#define POSITION_PUBLISH_PERIOD 500

namespace esphome
{
    namespace JRM_cover
    {

        class JRM : public util::Module, public cover::Cover, public Component
        {
        public:
            void setup() override;
            void loop() override;
            void dump_config() override;
            void sync_state() override { write_state(current_operation, target_position_); };
            uint8_t get_device_class_id() { return JRM_MODULE_ADDRESS; };
            cover::CoverTraits get_traits() override;

            /**
             * @brief Get the target operation of this JRM entity
             *
             * @return cover::CoverOperation
             */
            cover::CoverOperation get_target_operation() { return target_operation_; };

            /**
             * @brief Set the max close time, which guarantees that the cover will close
             *
             * @param time
             */
            void set_max_close_time(uint32_t time) { max_open_time_ = time / 100; };

            /**
             * @brief Set the max open time, which guarantees that the cover will open
             *
             * @param time
             */
            void set_max_open_time(uint32_t time) { max_close_time_ = time / 100; };

            /**
             * @brief Set the close time
             *
             * @param time Time in milliseconds
             */
            void set_close_time(uint32_t time)
            {
                close_time_ = time / 100;
                assume_position_ = true;
            }

            /**
             * @brief Set the open time
             *
             * @param time Time in milliseconds
             */
            void set_open_time(uint32_t time)
            {
                open_time_ = time / 100;
                assume_position_ = true;
            }

        protected:
            void control(const cover::CoverCall &call) override;
            void write_state(cover::CoverOperation state, float position);
            /**
             * @brief Writes a JRM move operation to the bus.
             *
             * @param address The desired module address
             * @param channel The desired channel
             * @param time Desired movement time
             * @param open True if the cover should be opened, false otherwise
             */
            void write_move_operation(uint8_t &address, uint8_t &channel, uint16_t &time, bool open);

            /**
             * @brief Writes a JRM IDLE command to the bus.
             *
             * @param address The desired module address
             * @param channel The desired channels
             */
            void write_idle_operation(uint8_t &address, uint8_t &channel);

            /**
             * @brief Set the position, while ensuring non-assumed covers only have positions OPEN/CLOSED.
             *
             * @param position The new position
             */
            void set_position(float position)
            {
                this->position = assume_position_ ? position : (position > 0.5f ? cover::COVER_OPEN : cover::COVER_CLOSED);
            }
            /**
             * @brief The max time the cover is allowed to move while opening.
             * This value should guarantee that the cover reaches the opened position.
             *
             */
            uint16_t max_open_time_ = 65535;

            /**
             * @brief The max time the cover is allowed to move while closing.
             * This value should guarantee that the cover reaches the closed position.
             *
             */
            uint16_t max_close_time_ = 65535;
            /**
             * @brief The time it takes for the cover to reach the OPEN state from being fully CLOSED.
             *
             */
            uint16_t open_time_ = 1;
            /**
             * @brief The time it takes for the cover to reach the CLOSED state from being fully OPENED.
             *
             */
            uint16_t close_time_ = 1;

            /**
             * @brief True if the covers position should be assumed.
             *
             */
            bool assume_position_ = false;

            /**
             * @brief Timestamp of the last request.
             *
             */
            long int last_request_ = 0;

            /**
             * @brief Time of the last position publish update in milliseconds.
             *
             */
            long int last_position_update_ = 0;

            /**
             * @brief Time of the last operation start.
             *
             */
            long int operation_start_time_ = 0;
            int resend_counter_ = 0;

            /**
             * @brief The covers target operation state.
             *
             */
            cover::CoverOperation target_operation_ = cover::COVER_OPERATION_IDLE;

            /**
             * @brief The covers target position.
             * Initialize with a value close to 1 to allow opening after reboot with miss-matching states.
             */
            float target_position_ = 0.99f;

            /**
             * @brief The movement time required to reach the target position.
             *
             */
            uint16_t target_movement_time_ = 0;

            /**
             * @brief The position in which the cover was at the start of the current operation.
             *
             */
            float operation_start_position_ = target_position_;
        };

    } // namespace JRM_cover
} // namespace esphome
#include "esphome/core/log.h"
#include "JRM.h"

namespace esphome
{
    namespace JRM_cover
    {

        using namespace cover;

        static const char *TAG = "JRM.cover";
        std::random_device rd;
        std::mt19937 rng(rd());
        std::uniform_int_distribution<int> jitter(0, 10);

        void JRM::setup()
        {
            set_position(target_position_);
        }

        void JRM::loop()
        {

            if (current_operation != target_operation_)
            {
                // Wait before retransmitting
                if (millis() - last_request_ > RESEND_TIMEOUT)
                {
                    // Add a little jitter
                    delay(jitter(rng));
                    if (resend_counter_ < MAX_RESENDS)
                    {
                        // Try resending as long as possible, double flip toggle
                        int resend = resend_counter_;
                        toggle_map->flip_toggle(this);
                        write_state(target_operation_, target_position_);
                        resend_counter_ = resend + 1;
                    }
                    else
                    {
                        // Reset the state and log a warning, device cannot be reached
                        ESP_LOGW(TAG, "Device not responding! Is the device connected to the bus? (DIP: %i)", address);
                        current_operation = COVER_OPERATION_IDLE;
                        set_position(operation_start_position_);
                        publish_state();
                        write_state(current_operation, operation_start_position_);
                    }
                }
            }
            else
            {
                // reset the state to idle after the desired open/close time
                if (current_operation != COVER_OPERATION_IDLE)
                {
                    // Check if the target time has been reached. Assume the states have been reached -> publish target state as current state
                    if (std::max(0L, static_cast<long>(millis() - operation_start_time_)) >= target_movement_time_ * 100.0f)
                    {
                        target_operation_ = COVER_OPERATION_IDLE;
                        current_operation = target_operation_;
                        set_position(target_position_);
                        publish_state();
                    }
                    else if (assume_position_)
                    {
                        // Interpolate the position based on time passed
                        // estimate the movement time based on open/close time and not based on max times
                        uint16_t estimated_movement_time = static_cast<uint16_t>(std::min(1.0f, std::max(0.0f, std::abs(operation_start_position_ - target_position_))) * (current_operation == COVER_OPERATION_OPENING ? open_time_ : close_time_));

                        float progess = std::min(1.0f, std::max(0.0f, (millis() - operation_start_time_) / (estimated_movement_time * 100.0f)));

                        // Update the cover position with the new estimated position
                        set_position(operation_start_position_ - (operation_start_position_ - target_position_) * progess);

                        // Publish state only once per second
                        if (millis() - last_position_update_ > POSITION_PUBLISH_PERIOD)
                        {
                            publish_state();
                            last_position_update_ = millis();
                        }
                    }
                }
            }
        }

        void JRM::dump_config()
        {
            ESP_LOGCONFIG(TAG, "JRM DIP-ID: %u\t Channel: %u", address, channel);
        }

        cover::CoverTraits JRM::get_traits()
        {
            auto traits = cover::CoverTraits();
            // We can assume that the cover will always reach the open/closed state if the max open/close times are correct
            // Thus we do not need assumed state -> this will prevent the covers from being opened, if they are already opened.
            traits.set_is_assumed_state(!assume_position_);
            traits.set_supports_position(assume_position_);
            traits.set_supports_tilt(false);

            return traits;
        }

        void JRM::control(const cover::CoverCall &call)
        {
            if (call.get_stop())
            {
                write_state(COVER_OPERATION_IDLE, cover::COVER_OPEN); // Position does not matter
            }
            else if (call.get_position().has_value())
            {
                if (call.get_position().value() >= position)
                {
                    write_state(COVER_OPERATION_OPENING, call.get_position().value());
                }
                else
                {
                    write_state(COVER_OPERATION_CLOSING, call.get_position().value());
                }
            }
        }

        void JRM::write_state(CoverOperation operation, float position)
        {
            resend_counter_ = 0;
            target_operation_ = operation;
            operation_start_position_ = this->position;
            toggle_map->flip_toggle(this);

            if (operation == COVER_OPERATION_IDLE)
            {
                write_idle_operation(address, channel);
            }
            else
            {
                // Ignore no change in position
                if (position == this->position)
                    return;

                target_position_ = position;
                bool open = (operation_start_position_ < position);
                // Use max open/close time to ensure the covers will reach the desired positions
                if (position == COVER_OPEN)
                {
                    target_movement_time_ = max_open_time_;
                    open = true;
                }
                else if (position == COVER_CLOSED)
                {
                    target_movement_time_ = max_close_time_;
                    open = false;
                }
                else if (assume_position_)
                {
                    // Determine time based on difference between current and target position w.r.t. up/down time
                    target_movement_time_ = static_cast<uint16_t>(std::min(1.0f, std::max(0.0f, std::abs(operation_start_position_ - position))) * (open ? open_time_ : close_time_));
                }
                else
                {
                    // This should never happen, since without assumed position a cover can only take positions OPEN/CLOSED
                    // Fallback parameters
                    if (position > 0.5f)
                    {
                        target_movement_time_ = max_open_time_;
                        target_position_ = cover::COVER_OPEN;
                        open = true;
                    }
                    else
                    {
                        target_movement_time_ = max_close_time_;
                        target_position_ = cover::COVER_CLOSED;
                        open = false;
                    }
                }

                write_move_operation(address, channel, target_movement_time_, open);
                operation_start_time_ = millis();
            }

            last_request_ = millis();
        }

        void JRM::write_idle_operation(uint8_t &address, uint8_t &channel)
        {
            uint8_t message[6] = {0x00};

            // 3 MSBits determine the channel, lower 5 bits are for functionality
            uint8_t function = (channel << 5) | 0x02;

            message[0] = static_cast<uint8_t>(JRM_MODULE_ADDRESS | address);
            message[1] = static_cast<uint8_t>((toggle_map->get_toggle(this) ? 0x80 : 0x00) | 0x02);
            message[2] = function;
            message[3] = 0xFC; // Prio

            short crc = util::PHC_CRC(message, 4);
            message[4] = static_cast<uint8_t>(crc & 0xFF);
            message[5] = static_cast<uint8_t>((crc & 0xFF00) >> 8);

            write_array(message, 6, false);
        }

        void JRM::write_move_operation(uint8_t &address, uint8_t &channel, uint16_t &time, bool open)
        {
            uint8_t message[8] = {0x00};

            // 3 MSBits determine the channel, lower 5 bits are for functionality
            uint8_t function = (channel << 5) | (open ? 0x05 : 0x06);

            message[0] = static_cast<uint8_t>(JRM_MODULE_ADDRESS | address);
            message[1] = static_cast<uint8_t>((toggle_map->get_toggle(this) ? 0x80 : 0x00) | 0x04);
            message[2] = function;
            message[3] = 0x07;      // Prio
            message[4] = time;      // unsigned short time value
            message[5] = time >> 8; // unsigned short time value

            short crc = util::PHC_CRC(message, 6);
            message[6] = static_cast<uint8_t>(crc & 0xFF);
            message[7] = static_cast<uint8_t>((crc & 0xFF00) >> 8);

            write_array(message, 8, true);
        }

    } // namespace empty_cover
} // namespace esphome
#include "esphome/core/log.h"
#include "JRM.h"

namespace esphome
{
    namespace JRM_cover
    {

        using namespace cover;

        static const char *TAG = "JRM.cover";

        void JRM::setup()
        {
        }

        void JRM::loop()
        {

            CoverOperation state = id(this).current_operation;
            if (state != this->target_state)
            {
                // Wait before retransmitting
                if (millis() - this->last_request > RESEND_TIMEOUT)
                {
                    if (resend_counter < MAX_RESENDS)
                    {
                        // Try resending as long as possible
                        int resend = this->resend_counter;
                        this->toggle_map->flip_toggle(this);
                        this->write_state(this->target_state);
                        this->resend_counter = resend + 1;
                    }
                    else
                    {
                        // Reset the state and log a warning, device cannot be reaced
                        ESP_LOGW(TAG, "Device not responding! Is the device connected to the bus? (DIP: %i)", address);
                        this->current_operation = COVER_OPERATION_IDLE;
                        this->publish_state();
                        this->write_state(state);
                    }
                }
            }
            else
            {
                // reset the state to idle after the desired open/close time
                if (state == COVER_OPERATION_OPENING || state == COVER_OPERATION_CLOSING)
                {
                    if (millis() - this->operation_start_time > (state == COVER_OPERATION_OPENING ? this->max_open_time : this->max_close_time) * 100)
                    {
                        this->target_state = COVER_OPERATION_IDLE;
                        this->current_operation = this->target_state;
                        this->position = (state == COVER_OPERATION_OPENING ? COVER_OPEN : COVER_CLOSED);
                        this->publish_state();
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
            traits.set_is_assumed_state(true);
            traits.set_supports_position(false);
            traits.set_supports_tilt(false);

            return traits;
        }

        void JRM::control(const cover::CoverCall &call)
        {
            if (call.get_stop())
                this->write_state(COVER_OPERATION_IDLE);

            if (call.get_position() == cover::COVER_OPEN)
                this->write_state(COVER_OPERATION_OPENING);

            if (call.get_position() == cover::COVER_CLOSED)
                this->write_state(COVER_OPERATION_CLOSING);
        }

        void JRM::write_state(CoverOperation state)
        {
            this->resend_counter = 0;
            this->target_state = state;
            this->toggle_map->flip_toggle(this);

            int message_size = 6;

            uint8_t message[message_size] = {0x00};
            if (state == COVER_OPERATION_IDLE)
            {
                message_size = 4;

                // 3 MSBits determine the channel, lower 5 bits are for functionality
                uint8_t function = (channel << 5) | 0x02;

                message[0] = static_cast<uint8_t>(JRM_MODULE_ADDRESS | address);
                message[1] = static_cast<uint8_t>((this->toggle_map->get_toggle(this) ? 0x80 : 0x00) | 0x02);
                message[2] = function;
                message[3] = 0xFC; // Prio
            }

            if (state == COVER_OPERATION_OPENING || state == COVER_OPERATION_CLOSING)
            {
                message_size = 6;
                this->operation_start_time = millis();

                // 3 MSBits determine the channel, lower 5 bits are for functionality
                uint8_t function = (channel << 5) | (state == COVER_OPERATION_OPENING ? 0x05 : 0x06);

                message[0] = static_cast<uint8_t>(JRM_MODULE_ADDRESS | address);
                message[1] = static_cast<uint8_t>((this->toggle_map->get_toggle(this) ? 0x80 : 0x00) | 0x04);
                message[2] = function;
                message[3] = 0x07;                                                                                   // Prio
                message[4] = (state == COVER_OPERATION_OPENING) ? this->max_open_time : this->max_close_time;        // unigned short time value
                message[5] = ((state == COVER_OPERATION_OPENING) ? this->max_open_time : this->max_close_time) >> 8; // unigned short time value
            }

            short crc = util::PHC_CRC(message, message_size);
            message[message_size] = static_cast<uint8_t>(crc & 0xFF);
            message[message_size + 1] = static_cast<uint8_t>((crc & 0xFF00) >> 8);

            write_array(message, message_size + 2);
            this->last_request = millis();
        }

    } // namespace empty_cover
} // namespace esphome
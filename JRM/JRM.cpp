#include "esphome/core/log.h"
#include "JRM.h"

#define RESEND_TIMEOUT 250
#define MAX_RESENDS 20

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
                        this->write_state(this->target_state);
                        this->resend_counter = resend + 1;
                    }
                    else
                    {
                        // Reset the state and log a warning, device cannot be reaced
                        ESP_LOGW(TAG, "Device not responding! Is the device connected to the bus? (DIP: %i)", address);
                        this->publish_state();
                        this->write_state(state);
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

            int message_size = 6;

            uint8_t message[message_size] = {0x00};
            if (state == COVER_OPERATION_IDLE)
            {
                message_size = 3;

                // 3 MSBits determine the channel, lower 5 bits are for functionality
                uint8_t function = (channel << 5) | 0x02;

                message[0] = static_cast<uint8_t>(JRM_MODULE_ADDRESS | address);
                message[1] = 0x01;
                message[2] = function;
            }

            if (state == COVER_OPERATION_OPENING || state == COVER_OPERATION_CLOSING)
            {
                message_size = 5;

                // 3 MSBits determine the channel, lower 5 bits are for functionality
                uint8_t function = (channel << 5) | (state == COVER_OPERATION_OPENING?0x05:0x06);

                message[0] = static_cast<uint8_t>(JRM_MODULE_ADDRESS | address);
                message[1] = 0x04;
                message[2] = function;
                message[3] = 0x00; // Prio
                message[4] = 0xFF; // unigned short time value
                message[5] = 0xFF; // unigned short time value
            }

            short crc = util::PHC_CRC(message, message_size - 2);
            message[message_size] = static_cast<uint8_t>(crc & 0xFF);
            message[message_size + 1] = static_cast<uint8_t>((crc & 0xFF00) >> 8);

            uart_device->write_array(message, message_size);
            uart_device->flush();
            this->last_request = millis();
        }

    } // namespace empty_cover
} // namespace esphome
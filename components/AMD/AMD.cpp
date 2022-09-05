#include "esphome/core/log.h"
#include "AMD.h"


namespace esphome
{
    namespace AMD_binary
    {

        static const char *TAG = "AMD";

        void AMD::setup()
        {
        }

        void AMD::loop()
        {
            bool state = id(this).state;
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
                        this->publish_state(state);
                        this->write_state(state);
                    }
                }
            }
        }

        void AMD::write_state(bool state)
        {
            this->resend_counter = 0;
            this->target_state = state;
            this->toggle_map->flip_toggle(this);
            // We don't publish the state here, we wait for the answer and use the callback to make sure the output acutally switched

            // 3 MSBits determine the channel, lower 5 bits are for functionality
            uint8_t function = (channel << 5) | (state ? 0x02 : 0x03);

            uint8_t message[5] = {static_cast<uint8_t>(AMD_MODULE_ADDRESS | address), static_cast<uint8_t>((this->toggle_map->get_toggle(this) ? 0x80 : 0x00) | 0x01), function, 0x00, 0x00};
            short crc = util::PHC_CRC(message, 3);

            message[3] = static_cast<uint8_t>(crc & 0xFF);
            message[4] = static_cast<uint8_t>((crc & 0xFF00) >> 8);

            write_array(message, 5);
            this->last_request = millis();
        }

        void AMD::dump_config()
        {
            ESP_LOGCONFIG(TAG, "AMD DIP-ID: %u\t Channel: %u", address, channel);
        }

    } // namespace AMD_binary
} // namespace esphome
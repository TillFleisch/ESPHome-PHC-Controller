#include "esphome/core/log.h"
#include "EMD_light.h"



namespace esphome
{
    namespace EMD_light
    {

        static const char *TAG = "EMD_Light";

        void EMDLight::setup()
        {
            set_disabled_by_default(true);
        }

        void EMDLight::loop()
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

        void EMDLight::write_state(bool state)
        {
            this->resend_counter = 0;
            this->target_state = state;
            this->toggle_map->flip_toggle(this);

            // 4 MSBits determine the channel, lower 4 bits are for functionality
            uint8_t function = (channel << 4) | (state ? 0x02 : 0x03);

            uint8_t message[5] = {static_cast<uint8_t>(EMD_MODULE_ADDRESS | address), static_cast<uint8_t>((this->toggle_map->get_toggle(this) ? 0x80 : 0x00) | 0x01), function, 0x00, 0x00};
            short crc = util::PHC_CRC(message, 3);

            message[3] = static_cast<uint8_t>(crc & 0xFF);
            message[4] = static_cast<uint8_t>((crc & 0xFF00) >> 8);

            write_array(message, 5);
            this->last_request = millis();
        }

        void EMDLight::dump_config()
        {
            ESP_LOGCONFIG(TAG, "EMD_Light DIP-ID: %u\t Channel: %u", address, channel);
        }

    } // namespace EMD_light
} // namespace esphome
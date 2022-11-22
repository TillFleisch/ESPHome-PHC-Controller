#include "esphome/core/log.h"
#include "EMD_light.h"

namespace esphome
{
    namespace EMD_light
    {

        static const char *TAG = "EMD_light";
        std::random_device rd;
        std::mt19937 rng(rd());
        std::uniform_int_distribution<int> jitter(0, 10);

        void EMD_light::setup()
        {
            set_disabled_by_default(true);
        }

        void EMD_light::loop()
        {
            if (state_ != target_state)
            {
                // Wait before retransmitting
                if (millis() - last_request > RESEND_TIMEOUT)
                {
                    // Add a little jitter
                    delay(jitter(rng));
                    if (resend_counter < MAX_RESENDS)
                    {
                        // Try resending as long as possible
                        int resend = resend_counter;
                        toggle_map->flip_toggle(this);
                        write_state(target_state);
                        resend_counter = resend + 1;
                    }
                    else
                    {
                        // Reset the state and log a warning, device cannot be reaced
                        ESP_LOGW(TAG, "Device not responding! Is the device connected to the bus? (DIP: %i)", address);
                        publish_state(state_);
                        write_state(state_);
                    }
                }
            }
        }

        void EMD_light::write_state(bool state)
        {
            resend_counter = 0;
            target_state = state;
            toggle_map->flip_toggle(this);

            // 4 MSBits determine the channel, lower 4 bits are for functionality
            uint8_t function = (channel << 4) | (this->target_state ? 0x02 : 0x03);

            uint8_t message[5] = {static_cast<uint8_t>(EMD_MODULE_ADDRESS | address), static_cast<uint8_t>((toggle_map->get_toggle(this) ? 0x80 : 0x00) | 0x01), function, 0x00, 0x00};
            short crc = util::PHC_CRC(message, 3);

            message[3] = static_cast<uint8_t>(crc & 0xFF);
            message[4] = static_cast<uint8_t>((crc & 0xFF00) >> 8);

            write_array(message, 5, true);
            last_request = millis();
        }

        void EMD_light::dump_config()
        {
            ESP_LOGCONFIG(TAG, "EMD-Light DIP-ID: %u\t Channel: %u", address, channel);
        }

    } // namespace EMD_light
} // namespace esphome
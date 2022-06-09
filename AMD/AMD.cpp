#include "esphome/core/log.h"
#include "AMD.h"

#define RESEND_TIMEOUT 250

namespace esphome
{
    namespace AMD_switch
    {

        static const char *TAG = "AMD.switch";
        bool target_state = false;
        long int last_request = 0;

        void AMD::setup()
        {
        }

        void AMD::loop()
        {
            bool state = id(this).state;
            if (state != target_state)
            {
                if (millis() - last_request > RESEND_TIMEOUT)
                    write_state(target_state);
            }
        }

        void AMD::write_state(bool state)
        {
            target_state = state;
            // We don't publish the state here, we wait for the answer and use the callback to make sure the output acutally switched

            // 3 MSBits determine the channel, lower 5 bits are for functionality
            uint8_t function = (channel << 5) | (state ? 0x02 : 0x03);

            uint8_t content[3] = {static_cast<uint8_t>(AMD_MODULE_ADDRESS | address), 0x01, function};
            short crc = util::PHC_CRC(content, 3);

            uint8_t message[5] = {static_cast<uint8_t>(AMD_MODULE_ADDRESS | address), 0x01, function, static_cast<uint8_t>(crc & 0xFF), static_cast<uint8_t>((crc & 0xFF00) >> 8)};

            uart_device->write_array(message, 5);
            uart_device->flush();
            last_request = millis();
        }

        void AMD::dump_config()
        {
            ESP_LOGCONFIG(TAG, "AMD DIP-ID: %u\t Channel: %u", address, channel);
        }

    } // namespace AMD_switch
} // namespace esphome
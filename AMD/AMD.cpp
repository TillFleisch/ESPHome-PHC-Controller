#include "esphome/core/log.h"
#include "AMD.h"

namespace esphome
{
    namespace AMD_switch
    {

        static const char *TAG = "AMD.switch";

        void AMD::setup()
        {
        }

        void AMD::write_state(bool state)
        {
            this->publish_state(state);
            uart_device->write_array({0x00});
        }

        void AMD::dump_config()
        {
            ESP_LOGCONFIG(TAG, "AMD DIP-ID: %u\t Channel: %u", address, channel);
        }

    } // namespace AMD_switch
} // namespace esphome
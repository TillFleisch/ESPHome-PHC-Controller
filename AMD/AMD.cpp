#include "esphome/core/log.h"
#include "AMD.h"
#include "esphome/core/application.h"

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
        }

        void AMD::dump_config()
        {
            ESP_LOGCONFIG(TAG, "AMD DIP-ID: %u", address);
        }

    } // namespace AMD_switch
} // namespace esphome
#include "esphome/core/log.h"
#include "EMD.h"
#include "esphome/core/application.h"

namespace esphome
{
    namespace EMD_switch
    {

        static const char *TAG = "EMD.switch";

        void EMD::setup()
        {
            set_disabled_by_default(true);
        }

        void EMD::write_state(bool state)
        {
        }

        void EMD::dump_config()
        {
            ESP_LOGCONFIG(TAG, "EMD DIP-ID: %u \t Channel: %u", address, channel);
        }

    } // namespace EMD_switch
} // namespace esphome
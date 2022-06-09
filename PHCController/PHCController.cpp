#include "esphome/core/log.h"
#include "PHCController.h"

namespace esphome
{
    namespace phc_controller
    {

        static const char *TAG = "phc_controller.component";

        void PHCController::setup()
        {
        }

        int i = 0;
        void PHCController::loop()
        {
            if (available())
            {
            }
        }

        void PHCController::dump_config()
        {
            ESP_LOGCONFIG(TAG, "PHC Controller");

            for (auto *amd_switch : this->amd_switches)
            {
                LOG_SWITCH(" ", "AMD_Switch", amd_switch);
            }
        }

    } // namespace phc_controller
} // namespace esphome
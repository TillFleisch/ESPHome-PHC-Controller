#include "esphome/core/log.h"
#include "JRM.h"

namespace esphome
{
    namespace JRM_cover
    {

        static const char *TAG = "JRM.cover";

        void JRM::setup()
        {
        }

        void JRM::loop()
        {
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

            if(call.get_stop())
                this->write_state(0);
            
            if(call.get_position() == cover::COVER_OPEN)
                this->write_state(1);

            if(call.get_position() == cover::COVER_CLOSED)
                this->write_state(2);

        }

        void JRM::write_state(int state)
        {
            this->resend_counter = 0;
            this->target_state = state;
        }

        

    } // namespace empty_cover
} // namespace esphome
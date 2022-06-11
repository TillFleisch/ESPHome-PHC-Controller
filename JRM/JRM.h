#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/uart/uart.h"
#include "../PHCController/util.h"

namespace esphome
{
    namespace JRM_cover
    {

        class JRM : public cover::Cover, public Component
        {
        public:
            void setup() override;
            void loop() override;
            void dump_config() override;
            cover::CoverTraits get_traits() override;
            void set_address(uint8_t address) { this->address = address; }
            void set_channel(uint8_t channel) { this->channel = channel; }
            void set_uart_device(uart::UARTDevice *uart_device) { this->uart_device = uart_device; };

        protected:
            void control(const cover::CoverCall &call) override;
            void write_state(int state);
            long int last_request = 0;
            int resend_counter = 0;
            int target_state;

            uint8_t address;
            uint8_t channel;
            uart::UARTDevice *uart_device;
        };

    } // namespace JRM_cover
} // namespace esphome
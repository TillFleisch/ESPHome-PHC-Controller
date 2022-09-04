#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/uart/uart.h"
#include "../PHCController/util.h"

namespace esphome
{
    namespace JRM_cover
    {

        class JRM : public util::Module, public cover::Cover, public Component
        {
        public:
            void setup() override;
            void loop() override;
            void dump_config() override;
            uint8_t get_device_class_id() { return JRM_MODULE_ADDRESS; };
            cover::CoverTraits get_traits() override;
            void set_uart_device(uart::UARTDevice *uart_device) { this->uart_device = uart_device; };
            cover::CoverOperation get_target_state() { return this->target_state; };
            void set_max_close_time(uint32_t time) { this->max_open_time = time / 100; };
            void set_max_open_time(uint32_t time) { this->max_close_time = time / 100; };

        protected:
            void control(const cover::CoverCall &call) override;
            void write_state(cover::CoverOperation state);
            uint16_t max_open_time = 65535;
            uint16_t max_close_time = 65535;
            long int last_request = 0;
            long int operation_start_time = 0;
            int resend_counter = 0;
            cover::CoverOperation target_state;

            uart::UARTDevice *uart_device;
        };

    } // namespace JRM_cover
} // namespace esphome
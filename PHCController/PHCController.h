#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "util.h"
#include "../AMD/AMD.h"
#include "../EMD/EMD.h"

namespace esphome
{
  namespace phc_controller
  {

    class PHCController : public uart::UARTDevice, public Component
    {
    public:
      void setup() override;
      void loop() override;
      void dump_config() override;
      void register_AMD(AMD_switch::AMD *obj)
      {
        this->amd_switches.push_back(obj);
        obj->set_uart_device(static_cast<uart::UARTDevice *>(this));
      }
      void register_EMD(EMD_switch::EMD *obj) { this->emd_switches.push_back(obj); }

    protected:
      void process_command(uint8_t *device_class_id, uint8_t *message, int *length);
      void send_acknowledgement(uint8_t address);

      std::vector<AMD_switch::AMD *> amd_switches;
      std::vector<EMD_switch::EMD *> emd_switches;
    };

  } // namespace phc_controller
} // namespace esphome
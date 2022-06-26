#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "util.h"
#include "../AMD/AMD.h"
#include "../EMD/EMD.h"
#include "../EMD/EMD_light.h"
#include "../JRM/JRM.h"

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
      void register_AMD(AMD_binary::AMD *obj)
      {
        this->amds.push_back(obj);
        obj->set_uart_device(static_cast<uart::UARTDevice *>(this));
      }
      void register_EMD(EMD_switch::EMD *obj) { this->emd_switches.push_back(obj); }
      void register_EMDLight(EMD_light::EMDLight *obj)
      {
        this->emd_lights.push_back(obj);
        obj->set_uart_device(static_cast<uart::UARTDevice *>(this));
      }
      void register_JRM(JRM_cover::JRM *obj)
      {
        this->jmds.push_back(obj);
        obj->set_uart_device(static_cast<uart::UARTDevice *>(this));
      }

    protected:
      void process_command(uint8_t *device_class_id, bool toggle, uint8_t *message, int *length);
      void send_acknowledgement(uint8_t address, bool toggle);
      void send_amd_config(uint8_t address);
      void send_emd_config(uint8_t address);

      std::vector<AMD_binary::AMD *> amds;
      std::vector<EMD_switch::EMD *> emd_switches;
      std::vector<EMD_light::EMDLight *> emd_lights;
      std::vector<JRM_cover::JRM *> jmds;
    };

  } // namespace phc_controller
} // namespace esphome
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
      // float get_loop_priority() const override { return 75.0f; }
      void register_AMD(AMD_binary::AMD *obj)
      {
        this->amds[obj->get_key()] = obj;
        obj->set_uart_device(static_cast<uart::UARTDevice *>(this));
        obj->set_toggle_map(toggle_map);
      }
      void register_EMD(EMD_switch::EMD *obj)
      {
        this->emd_switches[obj->get_key()] = obj;
        obj->set_toggle_map(toggle_map);
      }
      void register_EMDLight(EMD_light::EMDLight *obj)
      {
        this->emd_lights[obj->get_key()] = obj;
        obj->set_uart_device(static_cast<uart::UARTDevice *>(this));
        obj->set_toggle_map(toggle_map);
      }
      void register_JRM(JRM_cover::JRM *obj)
      {
        this->jrms[obj->get_key()] = obj;
        obj->set_uart_device(static_cast<uart::UARTDevice *>(this));
        obj->set_toggle_map(toggle_map);
      }

    protected:
      void process_command(uint8_t *device_class_id, bool toggle, uint8_t *message, int *length);
      void send_acknowledgement(uint8_t address, bool toggle);
      void send_amd_config(uint8_t address);
      void send_emd_config(uint8_t address);
      void setup_known_modules();

      util::ToggleMap *toggle_map = new util::ToggleMap();

      std::map<uint16_t, AMD_binary::AMD *> amds;
      std::map<uint8_t, EMD_switch::EMD *> emd_switches;
      std::map<uint8_t, EMD_light::EMDLight *> emd_lights;
      std::map<uint8_t, JRM_cover::JRM *> jrms;
    };

  } // namespace phc_controller
} // namespace esphome
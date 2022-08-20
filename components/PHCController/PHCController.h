#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "util.h"
#include "../AMD/AMD.h"
#include "../EMD/EMD.h"
#include "../EMD/EMD_light.h"
#include "../JRM/JRM.h"

#define FLOW_PIN_PULL_HIGH_DELAY 0
#define FLOW_PIN_PULL_LOW_DELAY 0

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
      void write_array(const uint8_t *data, size_t len);
      float get_setup_priority() const override { return setup_priority::HARDWARE; }
      void set_flow_control_pin(GPIOPin *pin) { flow_control_pin = pin; };
      util::ToggleMap *getToggleMap() { return toggle_map; };
      void register_AMD(AMD_binary::AMD *obj)
      {
        this->amds.push_back(obj);
        obj->set_controller(this);
      }
      void register_EMD(EMD_switch::EMD *obj)
      {
        this->emd_switches.push_back(obj);
        obj->set_controller(this);
      }
      void register_EMDLight(EMD_light::EMDLight *obj)
      {
        this->emd_lights.push_back(obj);
        obj->set_controller(this);
      }
      void register_JRM(JRM_cover::JRM *obj)
      {
        this->jrms.push_back(obj);
        obj->set_controller(this);
      }

    protected:
      void process_command(uint8_t *device_class_id, bool toggle, uint8_t *message, int *length);
      void send_acknowledgement(uint8_t address, bool toggle);
      void send_amd_config(uint8_t address);
      void send_emd_config(uint8_t address);
      void setup_known_modules();

      GPIOPin *flow_control_pin;
      util::ToggleMap *toggle_map = new util::ToggleMap();

      std::vector<AMD_binary::AMD *> amds;
      std::vector<EMD_switch::EMD *> emd_switches;
      std::vector<EMD_light::EMDLight *> emd_lights;
      std::vector<JRM_cover::JRM *> jrms;
    };

  } // namespace phc_controller
} // namespace esphome

inline void util::Module::set_controller(esphome::phc_controller::PHCController *controller)
{
  this->controller = controller;
  toggle_map = controller->getToggleMap();
}

inline void util::Module::write_array(esphome::phc_controller::PHCController *controller, const uint8_t *data, size_t len)
{
  controller->write_array(data, len);
};

#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "../PHCController/util.h"

namespace esphome
{
  namespace EMD_binary_sensor
  {

    /**
     * @brief binary sensor implementation of a EMD module.
     * This class does not implement any functionality, as the phc controller changes this entities state.
     *
     */
    class EMD : public util::Module, public binary_sensor::BinarySensor, public Component
    {
    public:
      void setup() override;
      void dump_config() override;
      uint8_t get_device_class_id() { return EMD_MODULE_ADDRESS; };
    };

  } // namespace EMD_binary_sensor
} // namespace esphome
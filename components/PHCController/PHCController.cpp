#include "esphome/core/log.h"
#include "PHCController.h"

#ifdef USE_ESP32_FRAMEWORK_ARDUINO
#include "esphome/components/uart/uart_component_esp32_arduino.h"
#elif USE_ESP8266
#include "esphome/components/uart/uart_component_esp8266.h"
#endif

namespace esphome
{
    namespace phc_controller
    {

        static const char *TAG = "phc_controller";

        void PHCController::setup()
        {
            /*
            Since Arduino Core version 2.0.0+ the timing of acknowledgement messages is too large, the reason for this issue is related to the way in which the uart input buffer is pocessed
            See these related issues
            https://github.com/espressif/arduino-esp32/issues/6689
            https://github.com/espressif/arduino-esp32/issues/6921
            setRxFIFOFull(1) is used to force immediate parsing of incoming data which fixes the delay issue
            */
#ifdef USE_ESP32_FRAMEWORK_ARDUINO
            uart::ESP32ArduinoUARTComponent *uartComponent = static_cast<uart::ESP32ArduinoUARTComponent *>(this->parent_);
            uartComponent->get_hw_serial()->setRxTimeout(1);
            uartComponent->get_hw_serial()->setRxFIFOFull(1);
#else
#pragma message("Response timings on the IDF Framework are likely incorrect. Please use the Arduino Framework and ideally an ESP32.")
            ESP_LOGW(TAG, "Response timings on the IDF Framework are likely incorrect. Please use the Arduino Framework and ideally an ESP32.");
#endif

            if (flow_control_pin_ != NULL)
            {
                flow_control_pin_->setup();
                flow_control_pin_->digital_write(false);
            }
            high_freq_.start();

            // Delay setup from here, since bus might be busy from ESP init.
            delay(500);
            setup_known_modules();
            last_message_time_ = millis();
        }

        void PHCController::loop()
        {

            if (available())
            {
                // Holds the abs. and relative address of the device
                uint8_t address = read();

                uint8_t toggle_and_length = read();
                bool toggle = toggle_and_length & 0x80;            // Mask the MRB (toggle bit)
                uint8_t content_length = toggle_and_length & 0x7F; // Mask everything except for the MSB (message length)

                // Assert message length is plausible
                if (content_length > 3)
                    return;

                // Read the actual message content
                uint8_t msg[content_length + 2];
                msg[0] = address;
                msg[1] = toggle_and_length;
                read_array(msg + 2, content_length + 2); // read content and checksum

                // Read the checksum
                uint16_t msg_checksum = msg[content_length + 3] << 8 | msg[content_length + 2];

                // Validate the checksum
                uint16_t calculated_checksum = util::PHC_CRC(msg, content_length + 2); // crc for content and prefix

                if (calculated_checksum != msg_checksum)
                {
                    ESP_LOGW(TAG, "Recieved bad message (checksum missmatch)");

                    // Skip the loop if the checksum is wrong
                    return;
                }

                last_message_time_ = millis();
                process_command(&address, toggle, msg + 2, &content_length);
                return;
            }

            if (!states_synced_)
            {
                if (millis() - last_message_time_ > INITIAL_SYNC_DELAY * 1000)
                {
                    sync_states();
                    states_synced_ = true;
                }
            }
        }

        void PHCController::dump_config()
        {
            ESP_LOGCONFIG(TAG, "PHC Controller");
            check_uart_settings(19200, 2, uart::UART_CONFIG_PARITY_NONE, 8);
            if (flow_control_pin_ != NULL)
                LOG_PIN("flow_control_pin: ", flow_control_pin_);

            ESP_LOGCONFIG(TAG, "PHC - NR. of  AMD: %i", amds_.size());
            ESP_LOGCONFIG(TAG, "PHC - NR. of  JRM: %i", jrms_.size());
            ESP_LOGCONFIG(TAG, "PHC - NR. of  EMD: %i", emds_.size());
            ESP_LOGCONFIG(TAG, "PHC - NR. of  EMD-Lights: %i", emd_lights_.size());
        }

        void PHCController::process_command(uint8_t *device_class_id, bool toggle, uint8_t *message, uint8_t *length)
        {
            uint8_t device_id = *device_class_id & 0x1F; // DIP settings (5 LSB)
            uint8_t device_class = *device_class_id & 0xE0;
            // EMD
            if (device_class == EMD_MODULE_ADDRESS)
            {
                // Initial configuration request message
                if (message[0] == 0xFF)
                {
                    //  Configure EMD
                    delayMicroseconds(TIMING_DELAY);
                    send_emd_config(device_id);
                    return;
                }

                uint8_t channel = (message[0] & 0xF0) >> 4;
                // Handle acknowledgement (such as switch led state)
                if (message[0] == 0x00)
                {
                    bool handled = false;
                    uint8_t channels = message[1];
                    for (uint8_t i = 0; i < 8; i++)
                    {
                        if (emd_lights_.count(util::key(device_id, i)))
                        {
                            auto *emd_light = emd_lights_[util::key(device_id, i)];

                            // Mask the channel and publish states accordingly
                            bool state = channels & (0x1 << i);
                            emd_light->publish_state(state);

                            handled = true;
                        }
                    }

                    if (!handled)
                        ESP_LOGI(TAG, "No configuration found for Message from (EMD-Light) Module: [DIP: %i, channel: %i]", device_id, channel);
                }
                else
                {
                    uint8_t action = message[0] & 0x0F;

                    // Send extra (speedy) acknowledgement, seems to help
                    send_acknowledgement(*device_class_id, toggle);

                    //  Find the switch and set the state
                    if (emds_.count(util::key(device_id, channel)))
                    {
                        auto *emd = emds_[util::key(device_id, channel)];
                        if (action == 0x02) // ON
                            emd->publish_state(true);
                        if (action == 0x07 || action == 0x03 || action == 0x05) // OFF
                            emd->publish_state(false);
                        return;
                    }

                    ESP_LOGI(TAG, "No configuration found for Message from (EMD) Module: [DIP: %i, channel: %i]", device_id, channel);
                }
                return;
            }

            if (device_class == AMD_MODULE_ADDRESS || device_class == JRM_MODULE_ADDRESS)
            {
                // Initial configuration request message
                if (message[0] == 0xFF)
                {
                    delayMicroseconds(TIMING_DELAY);
                    send_amd_config(device_id);
                    return;
                }

                // Check for Acknowledgement
                if (message[0] == 0x00)
                {
                    bool handled = false;
                    uint8_t channels = message[1];
                    for (uint8_t i = 0; i < 8; i++)
                    {
                        // Handle output switches
                        if (amds_.count(util::key(device_id, i)))
                        {
                            auto *amd = amds_[util::key(device_id, i)];

                            // Mask the channel and publish states accordingly
                            bool state = channels & (0x1 << i);
                            amd->publish_state(state);
                            handled = true;
                        }

                        // Handle output switches
                        if (jrms_.count(util::key(device_id, i)))
                        {
                            auto *jrm = jrms_[util::key(device_id, i)];
                            // For some reason the cover ack-message does not contain which covers are moving, so we are guessing that the channel has been processed
                            // This might lead to one cover not moving if 2 are manipulated at the same time
                            // Only accepting a single change will increase the chance of correct acknowledgement
                            if (jrm->current_operation != jrm->get_target_operation())
                            {
                                jrm->current_operation = jrm->get_target_operation();
                                jrm->publish_state();
                                handled = true;
                                break;
                            }
                            handled = true;
                        }
                    }
                    if (!handled)
                        ESP_LOGI(TAG, "No configuration found for Message from (AMD/JRM) Module: [DIP: %i]", device_id);
                }
                return;
            }

            // Send default acknowledgement
            send_acknowledgement(*device_class_id, toggle);
        }

        void inline PHCController::send_acknowledgement(uint8_t address, bool toggle)
        {
            uint8_t message[5] = {address, static_cast<uint8_t>((toggle ? 0x80 : 0x00) | 0x01), 0x00, 0x00, 0x00};
            uint16_t crc = util::PHC_CRC(message, 3);

            message[3] = static_cast<uint8_t>(crc & 0xFF);
            message[4] = static_cast<uint8_t>((crc & 0xFF00) >> 8);

            delayMicroseconds(TIMING_DELAY);
            write_array(message, 5, true);
        }

        void PHCController::send_amd_config(uint8_t id)
        {
            ESP_LOGI(TAG, "Configuring Module (AMD/JRM): [DIP: %i]", id);

            uint8_t message[7] = {static_cast<uint8_t>(AMD_MODULE_ADDRESS | id), 0x03, 0xFE, 0x00, 0xFF, 0x00, 0x00};

            short crc = util::PHC_CRC(message, 5);
            message[5] = static_cast<uint8_t>(crc & 0xFF);
            message[6] = static_cast<uint8_t>((crc & 0xFF00) >> 8);

            write_array(message, 7, false);
        }

        void PHCController::send_emd_config(uint8_t id)
        {
            ESP_LOGI(TAG, "Configuring Module (EMD): [DIP: %i]", id);
            uint8_t message[56] = {0x00};
            message[0] = EMD_MODULE_ADDRESS | id;
            message[1] = 0x34; // 52 Bytes

            // source: https://github.com/openhab/openhab-addons/blob/da59cdd255a66275dd7ae11dd294fedca4942d30/bundles/org.openhab.binding.phc/src/main/java/org/openhab/binding/phc/internal/handler/PHCBridgeHandler.java
            int pos = 2;

            message[pos++] = 0xFE;
            message[pos++] = 0x00; // POR

            message[pos++] = 0x00;
            message[pos++] = 0x00;

            for (int i = 0; i < 16; i++)
            { // 16 inputs
                message[pos++] = ((i << 4) | 0x02);
                message[pos++] = ((i << 4) | 0x03);
                message[pos++] = ((i << 4) | 0x05);
            }

            short crc = util::PHC_CRC(message, 54);
            message[54] = static_cast<uint8_t>(crc & 0xFF);
            message[55] = static_cast<uint8_t>((crc & 0xFF00) >> 8);

            write_array(message, 56, false);
        }

        void PHCController::setup_known_modules()
        {
            std::vector<uint8_t> addresses;
            // Collect all EMD Adresses

            for (auto const &module : emds_)
            {
                if (std::find(addresses.begin(), addresses.end(), module.second->get_address()) == addresses.end())
                    addresses.push_back(module.first);
            }

            // Send all known EMD Configurations
            for (uint8_t address : addresses)
                send_emd_config(address);

            addresses.clear();

            // Collect all AMD/JRM Adresses (AMD_MODULE_ADDRESS and JRM_MODULE_ADDRESS are the same)
            for (auto const &module : amds_)
            {
                if (std::find(addresses.begin(), addresses.end(), module.second->get_address()) == addresses.end())
                    addresses.push_back(module.second->get_address());
            }
            for (auto const &module : jrms_)
            {
                if (std::find(addresses.begin(), addresses.end(), module.second->get_address()) == addresses.end())
                    addresses.push_back(module.first);
            }

            // Send all known AMD Configurations
            for (uint8_t address : addresses)
                send_amd_config(address);
        }

        void PHCController::sync_states()
        {
            for (auto const &emd_light : emd_lights_)
            {
                emd_light.second->sync_state();
                delay(40);
            }
            for (auto const &amd : amds_)
            {
                amd.second->sync_state();
                delay(40);
            }
            for (auto const &jrm : jrms_)
            {
                jrm.second->sync_state();
                delay(40);
            }
        }

        void PHCController::write_array(const uint8_t *data, size_t len, bool allow_weak_operation)
        {

            // skip writing if the bus is busy and rely on retransmits
            if (allow_weak_operation && available())
                return;

            // Pull the write pin HIGH
            if (flow_control_pin_ != NULL)
            {
                flow_control_pin_->digital_write(true);
                delay(FLOW_PIN_PULL_HIGH_DELAY);
            }

            // Write data to the bus
            UARTDevice::write_array(data, len);

            // Flush everything out before pulling the flow control pin low
            UARTDevice::flush();

            // safety delay to prevent clashing with repsonses
            delay(1);

            // Pull the write pin LOW
            if (flow_control_pin_ != NULL)
            {
                delay(FLOW_PIN_PULL_LOW_DELAY);
                flow_control_pin_->digital_write(false);
            }
        }
    } // namespace phc_controller
} // namespace esphome
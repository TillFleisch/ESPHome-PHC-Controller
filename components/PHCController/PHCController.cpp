#include "esphome/core/log.h"
#include "PHCController.h"

namespace esphome
{
    namespace phc_controller
    {

        static const char *TAG = "phc_controller";

        void PHCController::setup()
        {
            if (flow_control_pin != NULL)
            {
                flow_control_pin->setup();
                flow_control_pin->digital_write(false);
            }
            this->high_freq_.start();

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
                uint8_t content_length = toggle_and_length & 0x7F; // Mask everything except for the MRB (message length)

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

                    while (int waste = available())
                    {
                        uint8_t trash[waste];
                        read_array(trash, waste);
                    }
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
            if (flow_control_pin != NULL)
                LOG_PIN("flow_control_pin: ", flow_control_pin);

            for (auto const &emd : emds)
            {
                LOG_BINARY_SENSOR(" ", "EMD.binary_sensor", emd.second);
            }

            for (auto const &emd_light : this->emd_lights)
            {
                LOG_SWITCH(" ", "EMD.light", emd_light.second);
            }

            for (auto const &amd : this->amds)
            {
                LOG_SWITCH(" ", "AMD", amd.second);
            }
            for (auto const &jrm : this->jrms)
            {
                LOG_COVER(" ", "JRM", jrm.second);
            }
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
                    send_emd_config(*device_class_id);
                    return;
                }

                uint8_t channel = (message[0] & 0xF0) >> 4;
                uint8_t action = message[0] & 0x0F;

                // Handle acknowledgement (such as switch led state)
                if (action == 0x00)
                {
                    bool handled = false;
                    uint8_t channels = message[1];
                    for (uint8_t i = 0; i < 8; i++)
                    {
                        if (this->emd_lights.count(util::key(device_id, i)))
                        {
                            auto *emd_light = emd_lights[util::key(device_id, i)];

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
                    // Send extra (speedy) acknowledgement, seems to help
                    send_acknowledgement(*device_class_id, toggle);

                    //  Find the switch and set the state
                    if (this->emds.count(util::key(device_id, channel)))
                    {
                        auto *emd = emds[util::key(device_id, channel)];
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
                    send_amd_config(*device_class_id);
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
                        if (this->amds.count(util::key(device_id, i)))
                        {
                            auto *amd = amds[util::key(device_id, i)];

                            // Mask the channel and publish states accordingly
                            bool state = channels & (0x1 << i);
                            amd->publish_state(state);
                            handled = true;
                        }

                        // Handle output switches
                        if (this->jrms.count(util::key(device_id, i)))
                        {
                            auto *jrm = jrms[util::key(device_id, i)];
                            // For some reason the cover ack-message does not contain which covers are moving, so we are guessing that the channel has been processed
                            // This might lead to one cover not moving if 2 are manipulated at the same time
                            jrm->current_operation = jrm->get_target_state();
                            jrm->publish_state();

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
            write_array(message, 5);
        }

        void PHCController::send_amd_config(uint8_t address)
        {

            uint8_t message[7] = {address, 0x03, 0xFE, 0x00, 0xFF, 0x00, 0x00};

            short crc = util::PHC_CRC(message, 5);
            message[5] = static_cast<uint8_t>(crc & 0xFF);
            message[6] = static_cast<uint8_t>((crc & 0xFF00) >> 8);

            write_array(message, 7);
        }

        void PHCController::send_emd_config(uint8_t address)
        {
            uint8_t message[56] = {0x00};
            message[0] = address;
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

            write_array(message, 56);
        }

        void PHCController::setup_known_modules()
        {
            std::vector<uint8_t> addresses;
            // Collect all EMD Adresses

            for (auto const &module : this->emds)
            {
                addresses.push_back(EMD_MODULE_ADDRESS | module.first);
            }

            // Send all known EMD Configurations
            for (uint8_t address : addresses)
                send_emd_config(address);

            addresses.clear();

            // Collect all AMD/JRM Adresses (AMD_MODULE_ADDRESS and JRM_MODULE_ADDRESS are the same)
            for (auto const &amd : this->amds)
            {
                if (std::find(addresses.begin(), addresses.end(), AMD_MODULE_ADDRESS | amd.second->get_address()) == addresses.end())
                    addresses.push_back(AMD_MODULE_ADDRESS | amd.second->get_address());
            }
            for (auto const &module : this->jrms)
            {
                addresses.push_back(JRM_MODULE_ADDRESS | module.first);
            }

            // Send all known AMD Configurations
            for (uint8_t address : addresses)
                send_amd_config(address);
        }

        void PHCController::sync_states()
        {
            for (auto const &emd_light : this->emd_lights)
            {
                emd_light.second->sync_state();
            }
            for (auto const &amd : this->amds)
            {
                amd.second->sync_state();
            }
            for (auto const &jrm : this->jrms)
            {
                jrm.second->sync_state();
            }
        }

        void PHCController::write_array(const uint8_t *data, size_t len)
        {
            // Pull the write pin HIGH
            if (flow_control_pin != NULL)
            {
                flow_control_pin->digital_write(true);
                delay(FLOW_PIN_PULL_HIGH_DELAY);
            }

            // Write data to the bus
            UARTDevice::write_array(data, len);

            // Flush everything out before pulling the flow control pin low
            UARTDevice::flush();

            // safety delay to prevent clashing with repsonses
            delay(1);

            // Pull the write pin LOW
            if (flow_control_pin != NULL)
            {
                delay(FLOW_PIN_PULL_LOW_DELAY);
                flow_control_pin->digital_write(false);
            }
        }
    } // namespace phc_controller
} // namespace esphome
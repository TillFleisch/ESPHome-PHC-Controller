#include "esphome/core/log.h"
#include "PHCController.h"

namespace esphome
{
    namespace phc_controller
    {

        static const char *TAG = "phc_controller";

        void PHCController::setup()
        {
            // Delay setup from here, since bus might be busy from ESP init.
            delay(500);
            setup_known_modules();
        }

        void PHCController::loop()
        {

            if (available())
            {
                // Holds the abs. and relative address of the device
                uint8_t address = read();

                uint8_t toggle_and_length = read();
                bool toggle = toggle_and_length & 0x80; // Mask the MRB (toggle bit)
                int length = toggle_and_length & 0x7F;  // Mask everything except for the MRB (message length)

                // Read the actual message content
                uint8_t msg[length];
                read_array(msg, length);

                // Read the checksum
                uint8_t checksum[2];
                read_array(checksum, 2); // Checksum always consists of 2 bytes
                short msg_checksum = (short)((checksum[1] << 8) | checksum[0]);

                // Determine the checksum for the message
                uint8_t concatinated_content[length + 2];
                concatinated_content[0] = address;
                concatinated_content[1] = toggle_and_length;
                for (int i = 0; i < length; i++)
                    concatinated_content[i + 2] = msg[i];

                // Validate the checksum
                short calculated_checksum = util::PHC_CRC(concatinated_content, length + 2);

                if (calculated_checksum != msg_checksum)
                {
                    ESP_LOGW(TAG, "Recieved bad message (checksum missmatch)");

                    // Clear the input buffer
                    while (available() > 0)
                        read();
                    // Skip the loop if the checksum is wrong
                    return;
                }

                process_command(&address, toggle, msg, &length);

                // Clear the input buffer
                while (available() > 0)
                    read();
            }
        }

        void PHCController::dump_config()
        {
            ESP_LOGCONFIG(TAG, "PHC Controller");
            for (auto *emd_switch : this->emd_switches)
            {
                LOG_SWITCH(" ", "EMD.switch", emd_switch);
            }
            for (auto *amd : this->amds)
            {
                LOG_SWITCH(" ", "AMD", amd);
            }
        }

        void PHCController::process_command(uint8_t *device_class_id, bool toggle, uint8_t *message, int *length)
        {
            uint8_t device_id = *device_class_id & 0x1F; // DIP settings (5 LSB)
            uint8_t device_class = *device_class_id & 0xE0;
            // EMD
            if (device_class == EMD_MODULE_ADDRESS)
            {
                // Initial configuration request message
                if (message[0] == 0xFF)
                {
                    // Configure EMD
                    send_emd_config(*device_class_id);
                    return;
                }

                uint8_t channel = (message[0] & 0xF0) >> 4;
                uint8_t action = message[0] & 0x0F;

                // Handle acknowledgement (such as switch led state)
                if (action == 0x00)
                {
                    for (auto *emd_light : this->emd_lights)
                    {
                        if (emd_light->get_address() == device_id && emd_light->get_channel() == channel)
                        {
                            // since we don't know the state, we toggle the current state
                            emd_light->publish_state(!id(emd_light).state);
                            return;
                        }
                    }
                    ESP_LOGI(TAG, "No configuration found for Message from (EMD-Light) Module: [DIP: %i, channel: %i]", device_id, channel);
                }
                else
                {
                    // Send extra (speedy) acknowledgement, seems to help
                    send_acknowledgement(*device_class_id, toggle);

                    // Find the switch and set the state
                    for (auto *emd_switch : this->emd_switches)
                    {
                        if (emd_switch->get_address() == device_id && emd_switch->get_channel() == channel)
                        {
                            if (action == 0x02) // ON
                                emd_switch->publish_state(true);
                            if (action == 0x07 || action == 0x03 || action == 0x05) // OFF
                                emd_switch->publish_state(false);
                            return;
                        }
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
                    send_amd_config(*device_class_id);
                    return;
                }

                // Check for Acknowledgement
                if (message[0] == 0x00)
                {
                    bool handled = false;
                    uint8_t channels = message[1];
                    for (int i = 0; i < 8; i++)
                    {
                        // Handle output switches
                        for (auto *amd : this->amds)
                        {
                            // mask channels
                            if (amd->get_address() == device_id && amd->get_channel() == i)
                            {
                                // Mask the channel and publish states accordingly
                                bool state = channels & (0x1 << i);
                                amd->publish_state(state);
                                handled = true;
                            }
                        }

                        // Handle output switches
                        for (auto *jrm : this->jrms)
                        {
                            // mask channels
                            if (jrm->get_address() == device_id && jrm->get_channel() == i)
                            {
                                // Mask the channel and publish states accordingly
                                bool state = channels & (0x1 << i);
                                if (state)
                                    jrm->publish_state(jrm->get_target_state());
                                else
                                    jrm->publish_state(cover::CoverOperation::COVER_OPERATION_IDLE);

                                handled = true;
                            }
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

        void PHCController::send_acknowledgement(uint8_t address, bool toggle)
        {
            uint8_t message[5] = {address, static_cast<uint8_t>((toggle ? 0x80 : 0x00) | 0x01), 0x00, 0x00, 0x00};
            short crc = util::PHC_CRC(message, 3);

            message[3] = static_cast<uint8_t>(crc & 0xFF);
            message[4] = static_cast<uint8_t>((crc & 0xFF00) >> 8);

            // Send multiple, seems to be more robust
            for (int i = 0; i <= 3; i++)
                write_array(message, 5);
            flush();
        }

        void PHCController::send_amd_config(uint8_t address)
        {

            uint8_t message[7] = {address, 0x03, 0xFE, 0x00, 0xFF, 0x00, 0x00};

            short crc = util::PHC_CRC(message, 5);
            message[5] = static_cast<uint8_t>(crc & 0xFF);
            message[6] = static_cast<uint8_t>((crc & 0xFF00) >> 8);

            write_array(message, 7);
            flush();
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
            flush();
        }

        void PHCController::setup_known_modules()
        {
            std::vector<uint8_t> addresses;
            // Collect all EMD Adresses
            for (auto *emd_switch : this->emd_switches)
            {
                if (std::find(addresses.begin(), addresses.end(), EMD_MODULE_ADDRESS | emd_switch->get_address()) == addresses.end())
                    addresses.push_back(EMD_MODULE_ADDRESS | emd_switch->get_address());
            }

            // Send all known EMD Configurations
            for (uint8_t address : addresses)
                send_emd_config(address);

            addresses.clear();

            // Collect all AMD/JRM Adresses (AMD_MODULE_ADDRESS and JRM_MODULE_ADDRESS are the same)
            for (auto *amd : this->amds)
            {
                if (std::find(addresses.begin(), addresses.end(), AMD_MODULE_ADDRESS | amd->get_address()) == addresses.end())
                    addresses.push_back(AMD_MODULE_ADDRESS | amd->get_address());
            }
            for (auto *jrm : this->jrms)
            {
                if (std::find(addresses.begin(), addresses.end(), JRM_MODULE_ADDRESS | jrm->get_address()) == addresses.end())
                    addresses.push_back(JRM_MODULE_ADDRESS | jrm->get_address());
            }

            // Send all known AMD Configurations
            for (uint8_t address : addresses)
                send_amd_config(address);
        }

    } // namespace phc_controller
} // namespace esphome
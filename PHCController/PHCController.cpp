#include "esphome/core/log.h"
#include "PHCController.h"

namespace esphome
{
    namespace phc_controller
    {

        static const char *TAG = "phc_controller";

        void PHCController::setup()
        {
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
                    ESP_LOGD(TAG, "Recieved bad message (checksum missmatch)");

                    // Skip the loop if the checksum is wrong
                    return;
                }

                process_command(&address, msg, &length);
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

        void PHCController::process_command(uint8_t *device_class_id, uint8_t *message, int *length)
        {
            uint8_t device_id = *device_class_id & 0x1F; // DIP settings (5 LRB)
            uint8_t device_class = *device_class_id & 0xE0;
            // EMD
            if (device_class == EMD_MODULE_ADDRESS)
            {
                uint8_t channel = (message[0] & 0xF0) >> 4;
                uint8_t action = message[0] & 0x0F;

                // Find the switch and set the state
                for (auto *emd_switch : this->emd_switches)
                {
                    if (emd_switch->get_address() == device_id && emd_switch->get_channel() == channel)
                    {
                        if (action == 0x02)
                            emd_switch->publish_state(true);
                        if (action == 0x07)
                            emd_switch->publish_state(false);
                    }
                }
                send_acknowledgement(*device_class_id);
            }

            if (device_class == AMD_MODULE_ADDRESS)
            {
                // Check for Acknowledgement
                if (message[0] == 0x00)
                {
                    uint8_t channels = message[1];
                    for (int i = 0; i < 8; i++)
                    {
                        // Handle switches
                        for (auto *amd : this->amds)
                        {
                            // Channels are offset for users 1..8
                            if (amd->get_address() == device_id && amd->get_channel() - 1 == i)
                            {
                                // Mask the channel and publish states accordingly
                                bool state = channels & (0x1 << i);
                                amd->publish_state(state);
                            }
                        }
                    }
                }
            }
        }

        void PHCController::send_acknowledgement(uint8_t address)
        {
            // TODO: Do we need to flip the toggle bit?
            uint8_t content[3] = {address, 0x01, 0x00};
            short crc = util::PHC_CRC(content, 3);

            uint8_t message[5] = {address, 0x01, 0x00, static_cast<uint8_t>(crc & 0xFF), static_cast<uint8_t>((crc & 0xFF00) >> 8)};

            write_array(message, 5);
            flush();
        }

    } // namespace phc_controller
} // namespace esphome
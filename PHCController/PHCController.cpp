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
                uint8_t device_class = (address & 0xE0) >> 5; // Mask 3 MRBits
                uint8_t device_id = address & 0x1F;           // DIP settings (5 LRB)

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

                process_command(&device_class, &device_id, msg, &length);

                send_acknowledgement(&address);
            }
        }

        void PHCController::dump_config()
        {
            ESP_LOGCONFIG(TAG, "PHC Controller");
            for (auto *emd_switch : this->emd_switches)
            {
                LOG_SWITCH(" ", "EMD_Switch", emd_switch);
            }
            for (auto *amd_switch : this->amd_switches)
            {
                LOG_SWITCH(" ", "AMD_Switch", amd_switch);
            }
        }

        void PHCController::process_command(uint8_t *device_class, uint8_t *device_id, uint8_t *message, int *length)
        {
            
            // EMD
            if (*device_class == 0)
            {
                uint8_t channel = (message[0] & 0xF0) >> 4;
                uint8_t action = message[0] & 0x0F;

                // Find the switch and set the state
                for (auto *emd_switch : this->emd_switches)
                {
                    if (emd_switch->get_address() == *device_id && emd_switch->get_channel() == channel)
                    {
                        if (action == 0x02)
                            emd_switch->publish_state(true);
                        if (action == 0x07)
                            emd_switch->publish_state(false);
                    }
                }
            }
        }

        void PHCController::send_acknowledgement(uint8_t *address)
        {
            // TODO: Do we need to flip the toggle bit?
            uint8_t content[3] = {*address, 0x01, 0x00};
            short crc = util::PHC_CRC(content, 3);

            uint8_t message[5] = {*address, 0x01, 0x00, (crc & 0xFF),(crc & 0xFF00) >> 8};

            write_array(message,5);
            flush();
        }

    } // namespace phc_controller
} // namespace esphome
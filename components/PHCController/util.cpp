#include "util.h"

namespace util
{
    // This is from https://www.phc-forum.de/media/kunena/attachments/253/PHC-Protokoll.pdf
    // CRC16/X25
    uint16_t PHC_CRC(const uint8_t *Data, int NumData)
    {
        unsigned short crc16;
        int i, j;
        crc16 = 0xFFFF;
        for (i = 0; i < NumData; i++)
        {
            crc16 ^= Data[i];
            for (j = 8; j > 0; j--)
            {
                if (crc16 & 0x0001)
                    crc16 = (crc16 >> 1) ^ 0x8408; // Reverse 0x1021 poly
                else
                    crc16 >>= 1;
            }
        }
        return crc16 ^ 0xFFFF;
    };

    uint16_t key(uint8_t address, uint8_t channel)
    {
        return address << 8 | channel;
    }

    bool ToggleMap::get_toggle(Module *module)
    {
        if (this->toggles.count(module->get_device_class_id()) && this->toggles[module->get_device_class_id()].count(module->get_address()))
            return this->toggles[module->get_device_class_id()][module->get_address()];
        return 0;
    }

    void ToggleMap::set_toggle(Module *module, bool new_toggle)
    {
        if (!this->toggles.count(module->get_device_class_id()))
        {
            this->toggles[module->get_device_class_id()] = std::map<uint8_t, bool>();
        }
        this->toggles[module->get_device_class_id()][module->get_address()] = new_toggle;
    }

}
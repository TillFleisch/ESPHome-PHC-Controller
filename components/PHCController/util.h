#pragma once
#include <map>
#include <stdint.h>

#define RESEND_TIMEOUT 30
#define MAX_RESENDS 40

#define EMD_MODULE_ADDRESS 0x00
#define AMD_MODULE_ADDRESS 0x40
#define JRM_MODULE_ADDRESS 0x40

namespace util
{
    unsigned short PHC_CRC(unsigned char *Data, unsigned char NumData);
    uint16_t key(uint8_t address, uint8_t channel);
    class ToggleMap;
    class Module;

    class ToggleMap
    {
    public:
        void set_toggle(Module *module, bool new_toggle);
        bool get_toggle(Module *module);
        void flip_toggle(Module *module) { this->set_toggle(module, !this->get_toggle(module)); };

    private:
        std::map<uint8_t, std::map<uint8_t, bool>> toggles = std::map<uint8_t, std::map<uint8_t, bool>>();
    };

    class Module
    {
    public:
        void set_address(uint8_t address)
        {
            this->address = address;
            this->key = util::key(this->address, this->channel);
        }
        void set_channel(uint8_t channel)
        {
            this->channel = channel;
            this->key = util::key(this->address, this->channel);
        }
        uint8_t get_address() { return this->address; }
        uint8_t get_channel() { return this->channel; }
        uint16_t get_key() { return this->key; }

        virtual uint8_t get_device_class_id() = 0;
        void set_toggle_map(ToggleMap *toggle_map) { this->toggle_map = toggle_map; };

    protected:
        ToggleMap *toggle_map;
        uint8_t address;
        uint8_t channel;
        uint16_t key = 0;
    };

}
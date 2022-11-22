#pragma once
#include <map>
#include <stdint.h>
#include "esphome/core/hal.h"

#define RESEND_TIMEOUT 50
#define MAX_RESENDS 40

#define EMD_MODULE_ADDRESS 0x00
#define AMD_MODULE_ADDRESS 0x40
#define JRM_MODULE_ADDRESS 0x40

namespace esphome
{
    namespace phc_controller
    {
        class PHCController;
    }
}
namespace util
{
    /**
     * @brief Determine the checksum used by the PHC system
     *
     * @param Data The data for which the checksum should be calculated
     * @param NumData The number of bytes provided
     * @return uint16_t checksum for the given data
     */
    uint16_t PHC_CRC(const uint8_t *Data, int NumData);

    /**
     * @brief Determines a unique Key based on address and channel
     *
     * @param address The modules channel
     * @param channel The modules address
     * @return uint16_t A key unique to this channel/address combination
     */
    uint16_t key(uint8_t address, uint8_t channel);

    class ToggleMap;
    class Module;

    /**
     * @brief Map containing retransmit/resend toggles for different modules.
     *
     */
    class ToggleMap
    {
    public:
        /**
         * @brief Set the toggle for a given module
         *
         * @param module The module for which the toggle will be set
         * @param new_toggle
         */
        void set_toggle(Module *module, bool new_toggle);

        /**
         * @brief Get the current toggle state for a specific module
         *
         * @param module A Module
         * @return bool toggle state
         */
        bool get_toggle(Module *module);

        /**
         * @brief Flips the current toggle for a specific module
         *
         * @param module The module for which the toggle should be flipped.
         */
        void flip_toggle(Module *module) { set_toggle(module, !get_toggle(module)); };

    private:
        /**
         * @brief Map containing toggle values
         *
         */
        std::map<uint8_t, std::map<uint8_t, bool>> toggles = std::map<uint8_t, std::map<uint8_t, bool>>();
    };

    /**
     * @brief Basic module with general parameters
     *
     */
    class Module
    {
    public:
        /**
         * @brief Set the address for this module
         *
         * @param address
         */
        void set_address(uint8_t address)
        {
            this->address = address;
            key = util::key(this->address, channel);
        }

        /**
         * @brief Set the channel on this module
         *
         * @param channel
         */
        void set_channel(uint8_t channel)
        {
            this->channel = channel;
            key = util::key(address, this->channel);
        }

        /**
         * @brief Get this modules address
         *
         * @return uint8_t The modules address
         */
        uint8_t get_address() { return address; }

        /**
         * @brief Get this entities channel
         *
         * @return uint8_t The channel associated with this entity
         */
        uint8_t get_channel() { return channel; }

        /**
         * @brief Get the key unique to this address/channel combination
         *
         * @return uint16_t key
         */
        uint16_t get_key() { return key; }

        /**
         * @brief Get the device class id of this module
         *
         * @return uint8_t device class id of this module
         */
        virtual uint8_t get_device_class_id() { return 0x00; };

        /**
         * @brief Set the PHC controller.
         *
         * @param controller Parent controller to which this entity belongs.
         */
        void set_controller(esphome::phc_controller::PHCController *controller);

        /**
         * @brief Writes a array of bytes to the PHC-bus using the parent controller.
         *
         * @param data Data to write to the bus
         * @param len Length of the given data
         * @param allow_weak_operation If true writing to the bus is not guaranteed
         */
        void write_array(const uint8_t *data, size_t len, bool allow_weak_operation) { write_array(controller, data, len, allow_weak_operation); };

        /**
         * @brief Writes a array of bytes to the PHC-bus using the provided parent controller.
         *
         * @param controller The parent controller to which this module belongs
         * @param data Data to write to the bus
         * @param len Length of the given data
         * @param allow_weak_operation If true writing to the bus is not guaranteed
         */
        virtual void write_array(esphome::phc_controller::PHCController *controller, const uint8_t *data, size_t len, bool allow_weak_operation);

        /**
         * @brief Syncs phc and entity state by forcing an update on the phc sytem.
         *  This function is mainly used after initializtion as a workaround. The first message might not be answered correctly.
         *  Sending the assumed known state sould make no phc state change, but fix the first message issue.
         */
        virtual void sync_state(){};

    protected:
        /**
         * @brief Toggle map reference associated with the controller
         *
         */
        ToggleMap *toggle_map;

        /**
         * @brief Parent controller reference.
         *
         */
        esphome::phc_controller::PHCController *controller;

        /**
         * @brief This entities (DIP) address
         *
         */
        uint8_t address;

        /**
         * @brief This entities channel
         *
         */
        uint8_t channel;

        /**
         * @brief The unique key associated with the channel/address combination of this module
         *
         */
        uint16_t key = 0;
    };

}
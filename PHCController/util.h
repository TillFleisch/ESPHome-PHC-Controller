#pragma once

#define EMD_MODULE_ADDRESS 0x00
#define AMD_MODULE_ADDRESS 0x40
#define JRM_MODULE_ADDRESS 0x40

namespace util
{
    unsigned short PHC_CRC(unsigned char *Data, unsigned char NumData);
}
#include "util.h"

namespace util
{
    // This is from https://www.phc-forum.de/media/kunena/attachments/253/PHC-Protokoll.pdf
    // CRC16/X25
    unsigned short PHC_CRC(unsigned char *Data, unsigned char NumData)
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
    }
}
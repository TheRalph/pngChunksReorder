#pragma once

#include <array>

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
class CCRC32
{

  public:

    ////////////////////////////////////////////////////////////////////////////
    // Return the CRC of the bytes buf[0..len-1].
    static uint32_t compute(const uint8_t *inpBuffer, std::size_t inBufSizeInByte);

  private:

    ////////////////////////////////////////////////////////////////////////////
    // Make the table for a fast CRC.
    static std::array<uint32_t, 256> generate_crc_lookup_table();

    ////////////////////////////////////////////////////////////////////////////
    // Update a running CRC with the bytes buf[0..len-1]--the CRC
    // should be initialized to all 1's, and the transmitted value
    // is the 1's complement of the final running CRC (see the
    // crc() routine below)).
    static uint32_t update_crc(const uint32_t inCRC, const unsigned char *inpBuffer, std::size_t inBufSizeInByte);

}; // CCRC32

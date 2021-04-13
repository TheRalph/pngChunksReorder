#include "CCRC32.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
uint32_t CCRC32::compute(const uint8_t *inpBuffer, std::size_t inBufSizeInByte)
{
  return update_crc(0xffffffffL, inpBuffer, inBufSizeInByte) ^ 0xffffffffL;
} // compute

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::array<uint32_t, 256> CCRC32::generate_crc_lookup_table()
{
  auto table = std::array<uint32_t, 256>{};
  uint32_t c;
  int n, k;

  for (n = 0; n < 256; n++)
  {
    c = (uint32_t) n;
    for (k = 0; k < 8; k++)
    {
      if (c & 1)
      {
        c = 0xedb88320L ^ (c >> 1);
      }
      else
      {
        c = c >> 1;
      }
    }
    table[n] = c;
  }
  return table;
} // generate_crc_lookup_table

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
uint32_t CCRC32::update_crc(const uint32_t inCRC, const unsigned char *inpBuffer, std::size_t inBufSizeInByte)
{
  static auto const crcTable = generate_crc_lookup_table();
  uint32_t crc = inCRC;

  for (std::size_t i = 0; i < inBufSizeInByte; i++)
  {
    crc = crcTable[(crc ^ inpBuffer[i]) & 0xff] ^ (crc >> 8);
  }
  return crc;
} // update_crc

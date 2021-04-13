#include "CChunk.h"
#include "CCRC32.h"

#include <cstring>
#include <iostream>
#include <limits.h>

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
template <typename T>
T swap_endian(T u)
{
    static_assert (CHAR_BIT == 8, "CHAR_BIT != 8");

    union
    {
      T u;
      unsigned char u8[sizeof(T)];
    } source, dest;

    source.u = u;

    for (size_t k = 0; k < sizeof(T); k++)
    {
      dest.u8[k] = source.u8[sizeof(T) - k - 1];
    }

    return dest.u;
} // swap_endian

////////////////////////////////////////////////////////////////////////////
CChunk::CChunk(const std::string& inType, const std::size_t inSizeInByte)
: m_dataSize(inSizeInByte)
{
  std::memset(m_type, 0, sizeof(m_type));
  if (inType.size()==sizeof(m_type))
  {
    std::memcpy(m_type, inType.data(), sizeof(m_type));
  }
  else
  {
    std::cerr<<"Wrong given type, size="<<inType.size()<<" but 4 expected."<<std::endl;
  }
  m_data.resize(inSizeInByte, 0);
} // constructor

////////////////////////////////////////////////////////////////////////////
CChunk::CChunk(const std::vector<uint8_t>& inData, const std::size_t inIndex)
{
  if (inIndex + get_header_size() <= inData.size())
  {
    const uint8_t *pChunkData = inData.data() + inIndex;
    m_dataSize = swap_endian<uint32_t>(*(uint32_t*)pChunkData);
    if (inIndex + get_header_size() + m_dataSize <= inData.size() )
    {
      pChunkData += sizeof(m_dataSize);
      std::memcpy(m_type, pChunkData, sizeof(m_type));
      pChunkData += sizeof(m_type);
      m_data.resize(m_dataSize);
      std::memcpy(m_data.data(), pChunkData, m_dataSize);
      pChunkData += m_dataSize;
      m_crc32 = swap_endian<uint32_t>(*(uint32_t*)pChunkData);
    }
    else
    {
      std::cerr<<"Chunk construction request: data corruption"<<std::endl;
    }
  }
  else
  {
    std::cerr<<"Chunk construction request: index out of range ("<<inIndex<<")"<<std::endl;
  }
} // constructor 

////////////////////////////////////////////////////////////////////////////
void CChunk::dump(std::ostream& ioStream, bool inOneLine)
{
  if (inOneLine)
  {
    ioStream<<get_type()<<" # "<<m_crc32<<std::endl;
  }
  else
  {
    ioStream<<"TYPE            = '"<<get_type()<<"'"<<std::endl;
    ioStream<<" DATA SIZE      = "<<m_dataSize<<std::endl;
    ioStream<<" CRC32          = "<<m_crc32<<std::endl;
    ioStream<<" computed CRC32 = "<<compute_CRC32()<<std::endl;
    ioStream<<" validity       = "<<(is_valid()? "true":"false")<<std::endl;
  }
} // dump

////////////////////////////////////////////////////////////////////////////
void CChunk::dump(std::ofstream& ofStream)
{
  uint32_t localData = swap_endian<uint32_t>(m_dataSize);
  ofStream.write((const char*)&localData, sizeof(localData));
  ofStream.write(m_type, sizeof(m_type));
  ofStream.write((const char*)m_data.data(), m_data.size());
  localData = swap_endian<uint32_t>(m_crc32);
  ofStream.write((const char*)&localData, sizeof(localData));
} // dump

////////////////////////////////////////////////////////////////////////////
uint32_t CChunk::compute_CRC32() const
{
  std::vector<uint8_t> localData;
  localData.resize(m_dataSize+sizeof(m_type));
  std::memcpy(localData.data(), m_type, sizeof(m_type));
  std::memcpy(localData.data()+sizeof(m_type), m_data.data(), m_dataSize);
  return CCRC32::compute(localData.data(), localData.size());
} // compute_CRC32

////////////////////////////////////////////////////////////////////////////
std::size_t CChunk::get_size() const
{
  return m_dataSize;
} // get_size

////////////////////////////////////////////////////////////////////////////
std::string CChunk::get_type() const
{
  std::string localTypeStr;
  char localType[5]={0};
  std::memcpy(localType, m_type, sizeof(m_type));
  localTypeStr = localType;
  return localType;
} // get_type

////////////////////////////////////////////////////////////////////////////
bool CChunk::is_valid() const
{
  return (get_type().size() == 4) && (compute_CRC32() == m_crc32);
} // is_valid

////////////////////////////////////////////////////////////////////////////
void CChunk::update_CRC32()
{
  m_crc32 = compute_CRC32();
} // is_valid

////////////////////////////////////////////////////////////////////////////
std::size_t CChunk::get_header_size()
{
  static constexpr std::size_t HEADER_SIZE = sizeof(m_dataSize) + sizeof(m_type) + sizeof(m_crc32);
  return HEADER_SIZE;
} // get_header_size

////////////////////////////////////////////////////////////////////////////
void CChunk::fill_end_chunk(CChunk& outEndChunk)
{
  outEndChunk.m_dataSize = 0;
  std::memcpy(outEndChunk.m_type, "IEND", sizeof(m_type));
  outEndChunk.m_data.clear();
  outEndChunk.m_crc32 = outEndChunk.compute_CRC32();
} // fill_end_chunk

////////////////////////////////////////////////////////////////////////////
void CChunk::dump_as_header(std::ostream& oStream)
{
  struct SHeader
  {
    uint32_t m_width;
    uint32_t m_height;
    uint8_t  m_depth;
    uint8_t  m_colorType;
    uint8_t  m_compressionMethod;
    uint8_t  m_filterMethod;
    uint8_t  m_interlaceMethod;
  }__attribute__((packed));

  union
  {
    SHeader m_header;
    uint8_t m_data[sizeof(SHeader)];
  } header;

  if (get_size() == sizeof(header))
  {
    std::memcpy(header.m_data, m_data.data(), sizeof(header));
    oStream<<"Header:"<<std::endl;
    oStream<<"  Width ="<<swap_endian<uint32_t>(header.m_header.m_width)<<std::endl;
    oStream<<"  Height="<<swap_endian<uint32_t>(header.m_header.m_height)<<std::endl;
    oStream<<"  Depth ="<<(int)header.m_header.m_depth<<std::endl;
    oStream<<"  Color Type="<<(int)header.m_header.m_colorType<<std::endl;
    oStream<<"  Compression Method="<<(int)header.m_header.m_compressionMethod<<std::endl;
    oStream<<"  Filter Method="<<(int)header.m_header.m_filterMethod<<std::endl;
    oStream<<"  Interlace Method="<<(int)header.m_header.m_interlaceMethod<<std::endl;
  }
  else
  {
    std::cerr<<"Corrupted header found: size="<<get_size()<<" but "<<sizeof(header)<<" expected."<<std::endl;
  }
} // dump_as_header

#include "CPNG.h"

#include <fstream>
#include <iostream>
#include <limits>
#include <exception>
#include <algorithm>
#include <functional>
#include <iomanip>

/*
   Critical chunks (must appear in this order, except PLTE
                    is optional):
   
           Name  Multiple  Ordering constraints
                   OK?
   
           IHDR    No      Must be first
           PLTE    No      Before IDAT
           IDAT    Yes     Multiple IDATs must be consecutive
           IEND    No      Must be last
   
   Ancillary chunks (need not appear in this order):
   
           Name  Multiple  Ordering constraints
                   OK?
   
           cHRM    No      Before PLTE and IDAT
           gAMA    No      Before PLTE and IDAT
           sBIT    No      Before PLTE and IDAT
           bKGD    No      After PLTE; before IDAT
           hIST    No      After PLTE; before IDAT
           tRNS    No      After PLTE; before IDAT
           pHYs    No      Before IDAT
           tIME    No      None
           tEXt    Yes     None
           zTXt    Yes     None
*/

constexpr uint8_t PNG_MAGIC_VALUE[]={0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a};
constexpr std::size_t SIZE_OF_PNG_MAGIC_VALUE=sizeof(PNG_MAGIC_VALUE);

const std::vector<std::string> CHUNK_TYPES = {"IHDR", "PLTE", "sRGB", "cHRM", "gAMA", "sBIT", "bKGD", "hIST", "tRNS", "pHYs", "tIME", "tEXt", "zTXt", "IDAT", "IEND"};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
CPNG::CPNG()
{
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool CPNG::load_from_PNG(const std::string& inName)
{
  bool retVal = false;
  std::ifstream pngFile(inName, std::ifstream::binary);
  if (pngFile)
  {
    // get the size of the file
    pngFile.seekg (0, pngFile.end);
    const std::size_t fileSize = pngFile.tellg();
    pngFile.seekg (0, pngFile.beg);

    std::vector<uint8_t> pngData;
    pngData.resize(fileSize);
    pngFile.read ((char*)pngData.data(), pngData.size());
    pngFile.close();

    if (fileSize > SIZE_OF_PNG_MAGIC_VALUE)
    {
      // Magic check
      bool magicCheck = (PNG_MAGIC_VALUE[0] == pngData.at(0));
      for (std::size_t i = 1; magicCheck && i < SIZE_OF_PNG_MAGIC_VALUE; i++)
      {
        magicCheck &= (PNG_MAGIC_VALUE[i] == pngData.at(i));
      }

      if (magicCheck)
      {
        const size_t nbChunks = load_chunks_from_buffer(pngData, SIZE_OF_PNG_MAGIC_VALUE);
        retVal = (nbChunks > 0);
      }
    }
  }

  return retVal;
} // load_from_PNG

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool CPNG::save_to_PNG(const std::string& inName)
{
  bool retVal = false;
  std::ofstream pngFile(inName, std::ofstream::binary);
  if (pngFile)
  {
    try
    {
      pngFile.write((const char*)PNG_MAGIC_VALUE, SIZE_OF_PNG_MAGIC_VALUE);

      for (auto chunk:m_chunks)
      {
        if (chunk.is_valid())
        {
          chunk.dump(pngFile);
        }
      }
      retVal = true;
    }
    catch (std::exception& inEx)
    {
      std::cerr<<"Error on save_to_PNG: "<<inEx.what()<<std::endl;
    }
  }

  return retVal;
} // save_to_PNG

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void CPNG::dump_chunks(std::ostream& ioStream, bool inOneLine)
{
  std::size_t id = 0;
  for (auto chunk:m_chunks)
  {
    if (chunk.is_valid())
    {
      ioStream << std::setw(3) << id++ << " # ";
      chunk.dump(ioStream, inOneLine);
    }
  }
} // dump_chunks

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::size_t CPNG::load_chunks_from_buffer(const std::vector<uint8_t>& inBufData, const std::size_t inIndex)
{
  std::size_t nbChunkRead = 0;

  const std::size_t fileSize = inBufData.size();
  std::size_t index = find_next_chunk(inBufData, inIndex);

  while ( index < fileSize )
  {
    m_chunks.emplace_back(inBufData, index);
    index = find_next_chunk(inBufData, index + m_chunks.back().get_size()+CChunk::get_header_size());
    nbChunkRead++;
  }

  return nbChunkRead;
} // load_chunks_from_buffer

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::size_t CPNG::load_chunks_from_file(const std::string& inName, const std::size_t inIndex)
{
  std::size_t nbChunkRead = 0;
  std::ifstream dataFile(inName, std::ifstream::binary);
  if (dataFile)
  {
    // get the size of the file
    dataFile.seekg (0, dataFile.end);
    const std::size_t fileSize = dataFile.tellg();
    dataFile.seekg (0, dataFile.beg);

    std::vector<uint8_t> bufData;
    bufData.resize(fileSize);
    dataFile.read ((char*)bufData.data(), bufData.size());
    dataFile.close();

    nbChunkRead = load_chunks_from_buffer(bufData, inIndex);
  }

  return nbChunkRead;
} // load_chunks_from_file

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
std::size_t CPNG::find_next_chunk(const std::vector<uint8_t>& inBufData, std::size_t inIndex)
{
  std::size_t chunkIndex = std::numeric_limits<std::size_t>::max();
  auto fistIndex = inBufData.begin() + inIndex;
  for (const auto& chunkType:CHUNK_TYPES)
  {
    auto it = std::search(fistIndex, inBufData.end(), std::boyer_moore_searcher(chunkType.begin(), chunkType.end()));
    if (it != inBufData.end())
    {
      chunkIndex = it - inBufData.begin() - sizeof(uint32_t);
      break;
    }
  }

  return std::min(chunkIndex, inBufData.size());
} // find_next_chunk

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
bool CPNG::get_data_range(chunkIterator& outFirstIt, chunkIterator& outLastIt)
{
  chunkContainer chunkToSearch;
  chunkToSearch.emplace_back("IDAT", 0);
  
  outFirstIt = std::find_first_of(m_chunks.begin(), m_chunks.end(), chunkToSearch.begin(), chunkToSearch.end(),
                         [](const CChunk& inC1, const CChunk& inC2){ return inC1.get_type() == inC2.get_type(); } );
  outLastIt = std::find_end(m_chunks.begin(), m_chunks.end(), chunkToSearch.begin(), chunkToSearch.end(),
                         [](const CChunk& inC1, const CChunk& inC2){ return inC1.get_type() == inC2.get_type(); } );

  return outFirstIt != m_chunks.end() && outLastIt != m_chunks.end();
}  // get_data_range

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void CPNG::reorder_data_chunks(const std::vector<std::size_t>& inNewOrder)
{
  chunkIterator firstIt;
  chunkIterator lastIt;
  if (get_data_range(firstIt, lastIt))
  {
    // std::cout<<"Range: "<<std::distance(m_chunks.begin(), firstIt)<<" "<<std::distance(m_chunks.begin(), lastIt)<<std::endl;
    lastIt++;
    std::vector<CChunk> dataChunks( firstIt, lastIt);

    if (inNewOrder.size() != dataChunks.size())
    {
      std::cerr<<"Wrong number of chunks: "<<inNewOrder.size()<<" provided, but "<<dataChunks.size()<<" expected"<<std::endl;
    }
    else
    {
      // Reorder data
      std::size_t i = 0;
      for (auto it = firstIt; it != lastIt; ++it)
      {
        *it = dataChunks.at(inNewOrder.at(i++));
      }
    }
  }
  else
  {
    std::cerr<<"No data !"<<std::endl;
  }
} // reorder_data_chunks

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void CPNG::fix_end_chunk()
{
  constexpr auto CHUNK="IEND";
  // remove all existing chunks
  std::vector<chunkIterator> ItToErase;
  for (auto it = m_chunks.begin(); it != m_chunks.end(); ++it)
  {
    if (it->get_type() == CHUNK)
    {
      ItToErase.push_back(it);
    }
  }

  // erase all
  for (auto it:ItToErase)
  {
    m_chunks.erase(it);
  }

  // add the end chunk
  m_chunks.emplace_back(CHUNK, 0);
  m_chunks.back().update_CRC32();
} // fix_end_chunk

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void CPNG::fix_palette_chunk(const int32_t inIdChunkToKeep)
{
  constexpr auto CHUNK="PLTE";
  // remove all existing chunks
  std::vector<chunkIterator> ItToErase;
  chunkIterator itChunkToKeep = m_chunks.end();

  std::size_t chunkId = 0;
  for (auto it = m_chunks.begin(); it != m_chunks.end(); ++it)
  {
    if (it->get_type() == CHUNK)
    {
      if (inIdChunkToKeep >= 0 && chunkId == (std::size_t)inIdChunkToKeep)
      {
        itChunkToKeep = it;
      }
      else
      {
        ItToErase.push_back(it);
      }
      chunkId++;
    }
  }

  // manage case of last one
  if (inIdChunkToKeep < 0 && !ItToErase.empty())
  {
    itChunkToKeep = ItToErase.back();
    ItToErase.resize(ItToErase.size()-1);
  }  

  // clean
  for (auto it:ItToErase)
  {
    m_chunks.erase(it);
  }

  // move before data
  if (itChunkToKeep != m_chunks.end())
  {
    chunkIterator firstIt;
    chunkIterator lastIt;
    if (get_data_range(firstIt, lastIt))
    {
      m_chunks.insert(firstIt, *itChunkToKeep);
      m_chunks.erase(itChunkToKeep);
    }
  }
} // fix_palette_chunk

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void CPNG::fix_all()
{
  fix_palette_chunk();
  fix_end_chunk();
} // fix_all

////////////////////////////////////////////////////////////////////////////
void CPNG::clean_chunks(const std::vector<std::string>& inChunksToKeep)
{
  std::vector<chunkIterator> ItToErase;
  for (auto it = m_chunks.begin(); it != m_chunks.end(); ++it)
  {
    auto itFound = std::find(inChunksToKeep.begin(), inChunksToKeep.end(), it->get_type());
    if (itFound == inChunksToKeep.end())
    {
      ItToErase.push_back(it);
    }
  }

  // erase all
  for (auto it:ItToErase)
  {
    m_chunks.erase(it);
  }
} // clean_chunks

////////////////////////////////////////////////////////////////////////////
void CPNG::dump_header(std::ostream& ioStream)
{
  chunkContainer chunkToSearch;
  chunkToSearch.emplace_back("IHDR", 0);
  
  auto it = std::find_first_of(m_chunks.begin(), m_chunks.end(), chunkToSearch.begin(), chunkToSearch.end(),
                         [](const CChunk& inC1, const CChunk& inC2){ return inC1.get_type() == inC2.get_type(); } );
  if (it != m_chunks.end())
  {
    it->dump_as_header(ioStream);
  }
  else
  {
    std::cerr<<"No header found"<<std::endl;
  }
} // dump_header


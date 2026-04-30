//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifdef ZIP_SUPPORT

#include <zlib.h>

#include "Bankswitch.hxx"
#include "ZipHandler.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::open(string_view filename)
{
  // Close already open file (if any) and add to cache
  addToCache();

  // Ensure we start with a nullptr result
  myZip.reset();

  auto ptr = findCached(filename);
  if(ptr)
  {
    // Only a previously used entry will exist in the cache, so we know it's valid
    myZip = std::move(ptr);

    // Was already initialized; we just need to re-open it
    if(!myZip->open())
      throw ZipException(ZipError::FILE_ERROR);
  }
  else
  {
    // Allocate memory for the ZipFile structure
    try {
      ptr = std::make_unique<ZipFile>(filename);
    }
    catch(const std::bad_alloc&) {
      throw ZipException(ZipError::OUT_OF_MEMORY);
    }

    // Open the file and initialize it
    if(!ptr->open())
      throw ZipException(ZipError::FILE_ERROR);

    ptr->initialize(); // ROM counting happens inside here

    myZip = std::move(ptr);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::pair<size_t, bool> ZipHandler::find(string_view name)
{
  if(!myZip)
    return {0, false};

  const ZipHeader* header = myZip->findHeader(name);
  if(!header)
    return {0, false};

  return {static_cast<size_t>(header->uncompressedLength), true};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt64 ZipHandler::decompress(string_view name, ByteBuffer& image)
{
  if(!myZip)
    throw ZipException(ZipError::FILE_ERROR);

  const ZipHeader* header = myZip->findHeader(name);
  if(!header)
    throw ZipException(ZipError::FILE_NOT_FOUND);
  if(header->uncompressedLength == 0)
    throw ZipException(ZipError::FILE_ERROR);

  const uInt64 length = header->uncompressedLength;
  image = std::make_unique<uInt8[]>(length);

  myZip->decompress(*header, ByteMSpan{image.get(), static_cast<size_t>(length)});

  return length;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::optional<std::pair<string_view, size_t>> ZipHandler::firstRom() const
{
  if(!myZip || !myZip->myFirstRomName.has_value())
    return std::nullopt;

  return std::make_pair(*myZip->myFirstRomName, myZip->myFirstRomSize);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ZipHandler::ZipFilePtr ZipHandler::findCached(string_view filename)
{
  auto it = myZipCache.find(filename);
  if(it == myZipCache.end())
    return {};

  // Extract ZipFilePtr
  ZipFilePtr result = std::move(it->second.first);

  // Remove from both map and list
  myCacheOrder.erase(it->second.second);
  myZipCache.erase(it);

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::addToCache()
{
  if(myZip == nullptr)
    return;

  // Close the open file
  myZip->close();

  // Take ownership of the filename before moving myZip into the cache
  const string key = myZip->myFilename;

  // If cache is full, evict the oldest entry
  if(myZipCache.size() >= CACHE_SIZE)
  {
    const string& oldestKey = myCacheOrder.front();
    myZipCache.erase(oldestKey);
    myCacheOrder.pop_front();
  }

  // Insert the new entry at the back (most recently used)
  myCacheOrder.push_back(key);
  auto iter = std::prev(myCacheOrder.end());
  myZipCache.emplace(key, std::make_pair(std::move(myZip), iter));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ZipHandler::ZipFile::ZipFile(string_view filename)
  : myFilename{filename}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ZipHandler::ZipFile::open()
{
  // NOTE: This can't be opened with FSNode::openFStream(), since we're
  //       already in FSNode when accessing a ZIP file, and this leads to
  //       recursion.
  myStream.open(myFilename, std::fstream::in | std::fstream::binary);
  if(!myStream.is_open())
  {
    myLength = 0;
    return false;
  }

  // Disable exceptions completely
  myStream.exceptions(std::ios::goodbit);

  myStream.seekg(0, std::ios::end);
  if(!myStream)
    return false;

  myLength = static_cast<uInt64>(myStream.tellg());

  myStream.seekg(0, std::ios::beg);
  return static_cast<bool>(myStream);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::ZipFile::initialize()
{
  readEcd();

  // Verify that we can work with this zipfile (no disk spanning allowed)
  if(myEcd.diskNumber != myEcd.cdStartDiskNumber ||
     myEcd.cdDiskEntries != myEcd.cdTotalEntries)
    throw ZipException(ZipError::UNSUPPORTED);

  // Allocate memory for the central directory
  myCd.resize(myEcd.cdSize);

  // Read the central directory
  uInt64 read_length = 0;
  const bool success = readStream(myCd.data(), myEcd.cdStartDiskOffset,
                                  myEcd.cdSize, read_length);

  if(!success)
    throw ZipException(ZipError::FILE_ERROR);
  if(read_length != myEcd.cdSize)
    throw ZipException(ZipError::FILE_TRUNCATED);

  // Build stable ZipHeader table
  myHeaders.clear();
  myHeaders.reserve(myEcd.cdTotalEntries);

  size_t pos = 0;
  while(pos < myCd.size())
  {
    // Make sure we have enough data
    // If we're at or past the end, we're done
    const CentralDirEntryReader reader(myCd.data() + pos);
    if(!reader.signatureCorrect())
      throw ZipException(ZipError::FILE_CORRUPT);

    // Extract file header info
    const ZipHeader header {
      reader.versionCreated(),
      reader.versionNeeded(),
      reader.generalFlag(),
      reader.compressionMethod(),
      reader.modifiedTime(),
      reader.modifiedDate(),
      reader.crc32(),
      reader.compressedSize(),
      reader.uncompressedSize(),
      reader.startDisk(),
      reader.headerOffset(),
      reader.filename()
    };

    // ROM detection
    if(header.uncompressedLength > 0 &&
       Bankswitch::isValidRomName(header.filename))
    {
      ++myRomfiles;

      // Cache FIRST ROM only
      if(!myFirstRomName.has_value())
      {
        myFirstRomName = header.filename;
        myFirstRomSize = static_cast<size_t>(header.uncompressedLength);
      }
    }

    myHeaders.emplace_back(header);
    pos += reader.totalLength();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::ZipFile::close()
{
  if(myStream.is_open())
    myStream.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::ZipFile::readEcd()
{
  vector<uInt8> buffer;
  buffer.reserve(std::min(uInt64{65536}, myLength));

  for(uInt64 buflen = std::min(uInt64{1024}, myLength);
      ;
      buflen = std::min(buflen * 2, myLength))
  {
    buffer.resize(buflen);

    // Read in one buffers' worth of data
    uInt64 read_length = 0;
    const bool success = readStream(buffer.data(), myLength - buflen,
                                    buflen, read_length);
    if(!success || read_length != buflen)
      throw ZipException(ZipError::FILE_ERROR);

    // Scan backwards for ECD signature
    for(auto offset = static_cast<Int32>(buflen - EcdReader::minimumLength());
        offset >= 0; --offset)
    {
      // Extract ECD info
      const EcdReader reader(buffer.data() + offset);
      if(reader.signatureCorrect() &&
         ((static_cast<uInt64>(offset) + reader.totalLength()) <= buflen))
      {
        myEcd.diskNumber        = reader.thisDiskNo();
        myEcd.cdStartDiskNumber = reader.dirStartDisk();
        myEcd.cdDiskEntries     = reader.dirDiskEntries();
        myEcd.cdTotalEntries    = reader.dirTotalEntries();
        myEcd.cdSize            = reader.dirSize();
        myEcd.cdStartDiskOffset = reader.dirOffset();
        return;
      }
    }

    // Searched the whole file and still didn't find it
    if(buflen >= myLength)
      throw ZipException(ZipError::BAD_SIGNATURE);

    if(buflen >= 65536)
      throw ZipException(ZipError::OUT_OF_MEMORY);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ZipHandler::ZipFile::readStream(uInt8* out, uInt64 offset,
                                     uInt64 length, uInt64& actual)
{
  myStream.seekg(offset, std::ios::beg);
  if(!myStream) return false;

  myStream.read(reinterpret_cast<char*>(out), length);
  actual = static_cast<uInt64>(myStream.gcount());

  // If we hit EOF early, that's OK (caller checks size)
  return myStream || myStream.eof();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::ZipFile::ensureIndex() const
{
  if(myIndexBuilt || myHeaders.size() < 256)
    return;

  myHeaderIndex.reserve(myHeaders.size());

  for(size_t i = 0; i < myHeaders.size(); ++i)
    myHeaderIndex.emplace(myHeaders[i].filename, i);

  myIndexBuilt = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ZipHandler::ZipHeader* ZipHandler::ZipFile::findHeader(string_view name) const
{
  if(myHeaders.size() >= 256)
  {
    ensureIndex();
    auto it = myHeaderIndex.find(name);
    if(it != myHeaderIndex.end())
      return &myHeaders[it->second];
    return nullptr;
  }

  // Small ZIP → linear scan (faster)
  for(const auto& header: myHeaders)
    if(header.filename == name)
      return &header;

  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::ZipFile::decompress(const ZipHeader& header, ByteMSpan out)
{
  // If we don't have enough buffer, error
  if(out.size() < header.uncompressedLength)
    throw ZipException(ZipError::BUFFER_TOO_SMALL);

  if(header.startDiskNumber != myEcd.diskNumber)
    throw ZipException(ZipError::UNSUPPORTED);

  // Get the compressed data offset
  const uInt64 offset = getCompressedDataOffset(header);

  // Handle compression types
  switch(header.compression)
  {
    case 0: decompressDataType0(header, out, offset); break;
    case 8: decompressDataType8(header, out, offset); break;
    default:
      throw ZipException(ZipError::UNSUPPORTED);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt64 ZipHandler::ZipFile::getCompressedDataOffset(const ZipHeader& header)
{
  const GeneralFlagReader flags(header.bitFlag);

  if(header.startDiskNumber != myEcd.diskNumber ||
     header.versionNeeded > 63 || flags.patchData() ||
     flags.encrypted() || flags.strongEncryption())
    throw ZipException(ZipError::UNSUPPORTED);

  // Use a small local buffer; myBuffer is reserved for streaming decompression
  std::array<uInt8, LocalFileHeaderReader::minimumLength()> localBuf{};

  // Read the fixed-sized part of the local file header
  uInt64 read_length = 0;
  const bool success = readStream(localBuf.data(), header.localHeaderOffset,
                                  localBuf.size(), read_length);

  if(!success)
    throw ZipException(ZipError::FILE_ERROR);
  else if(read_length != localBuf.size())
    throw ZipException(ZipError::FILE_TRUNCATED);

  // Compute the final offset
  const LocalFileHeaderReader reader(localBuf.data());
  if(!reader.signatureCorrect())
    throw ZipException(ZipError::BAD_SIGNATURE);

  return header.localHeaderOffset + reader.totalLength();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::ZipFile::decompressDataType0(const ZipHeader& header,
                                              ByteMSpan out, uInt64 offset)
{
  // The data is uncompressed; just read it
  uInt64 read_length = 0;
  const bool success = readStream(out.data(), offset,
                                  header.compressedLength, read_length);

  if(!success)
    throw ZipException(ZipError::FILE_ERROR);
  else if(read_length != header.compressedLength)
    throw ZipException(ZipError::FILE_TRUNCATED);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::ZipFile::decompressDataType8(const ZipHeader& header,
                                              ByteMSpan out, uInt64 offset)
{
  // Seek ONCE to start of compressed data
  myStream.seekg(offset, std::ios::beg);
  if(!myStream)
    throw ZipException(ZipError::FILE_ERROR);

  size_t input_remaining = header.compressedLength;

  z_stream stream{};
  stream.next_out  = out.data();
  stream.avail_out = static_cast<uInt32>(out.size());

  // RAII cleanup
  struct InflateGuard {
    z_stream& z_s;
    explicit InflateGuard(z_stream& s) : z_s{s} { }
    ~InflateGuard() { inflateEnd(&z_s); }
    InflateGuard(const InflateGuard&) = delete;
    InflateGuard(InflateGuard&&) = delete;
    InflateGuard& operator=(const InflateGuard&) = delete;
    InflateGuard& operator=(InflateGuard&&) = delete;
  };

  // Initialize the decompressor
  if(inflateInit2(&stream, -MAX_WBITS) != Z_OK)
    throw ZipException(ZipError::DECOMPRESS_ERROR);

  const InflateGuard guard{stream};

  // Streaming loop
  for(;;)
  {
    // Refill input buffer when empty
    if(stream.avail_in == 0 && input_remaining > 0)
    {
      const size_t chunkSize = std::min(input_remaining, DECOMPRESS_BUFSIZE);

      myStream.read(reinterpret_cast<char*>(myBuffer.data()), chunkSize);
      const auto read_length =
        static_cast<uInt64>(myStream.gcount());

      // If we read nothing, but still have data left, the file is truncated
      if(read_length == 0)
        throw ZipException(ZipError::FILE_TRUNCATED);

      // Fail only on real errors (not EOF)
      if(!myStream && !myStream.eof())
        throw ZipException(ZipError::FILE_ERROR);

      // Fill out the input data
      stream.next_in  = myBuffer.data();
      stream.avail_in = static_cast<uInt32>(read_length);

      input_remaining -= read_length;
    }

    // Now inflate
    const int zerr = inflate(&stream, Z_NO_FLUSH);

    if(zerr == Z_STREAM_END)
      break;

    if(zerr != Z_OK)
      throw ZipException(ZipError::DECOMPRESS_ERROR);
  }

  // Final validation
  // If anything looks funny, report an error
  if(stream.avail_out > 0 || input_remaining > 0)
    throw ZipException(ZipError::DECOMPRESS_ERROR);
}

#endif  // ZIP_SUPPORT

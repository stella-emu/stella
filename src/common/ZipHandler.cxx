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
void ZipHandler::open(const string& filename)
{
  // Close already open file (if any) and add to cache
  addToCache();

  // Ensure we start with a nullptr result
  myZip.reset();

  ZipFilePtr ptr = findCached(filename);
  if(ptr)
  {
    // Only a previously used entry will exist in the cache, so we know it's valid
    myZip = std::move(ptr);

    // Was already initialized; we just need to re-open it
    if(!myZip->open())
      throw std::runtime_error(errorMessage(ZipError::FILE_ERROR));
  }
  else
  {
    // Allocate memory for the ZipFile structure
    try        { ptr = std::make_unique<ZipFile>(filename); }
    catch(...) { throw std::runtime_error(errorMessage(ZipError::OUT_OF_MEMORY)); }

    // Open the file and initialize it
    if(!ptr->open())
      throw std::runtime_error(errorMessage(ZipError::FILE_ERROR));
    ptr->initialize();

    myZip = std::move(ptr);

    // Count ROM files (we do it here so it will be cached)
    try
    {
      while(hasNext())
      {
        if(myZip->nextFileIsValidRom())
          myZip->myRomfiles++;
      }
    }
    catch(...)
    {
      myZip->myRomfiles = 0;
    }
  }

  reset();  // Reset iterator to beginning for subsequent use
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::reset()
{
  // Reset the position and go from there
  if(myZip)
    myZip->myCdPos = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ZipHandler::hasNext() const
{
  return myZip && (myZip->myCdPos < myZip->myEcd.cdSize);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::tuple<string, size_t> ZipHandler::next()
{
  while(hasNext())
  {
    const ZipHeader* const header = myZip->nextFile();
    if(!header)
      throw std::runtime_error(errorMessage(ZipError::FILE_CORRUPT));
    if(header->uncompressedLength > 0)
      return {string{header->filename}, header->uncompressedLength};
  }
  return {EmptyString(), 0};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt64 ZipHandler::decompress(ByteBuffer& image)
{
  if(!myZip || myZip->myHeader.uncompressedLength == 0)
    throw std::runtime_error("Invalid ZIP archive");

  const uInt64 length = myZip->myHeader.uncompressedLength;
  image = std::make_unique<uInt8[]>(length);

  myZip->decompress(std::span<uInt8>(image.get(), static_cast<size_t>(length)));
  return length;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ZipHandler::ZipFilePtr ZipHandler::findCached(const string& filename)
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

#if 0
  cerr << "\nCACHE contents:\n";
    for(cachenum = 0; cachenum < myZipCache.size(); ++cachenum)
      if(myZipCache[cachenum] != nullptr)
        cerr << "  " << cachenum << " : " << myZipCache[cachenum]->filename << '\n';
  cerr << '\n';
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ZipHandler::ZipFile::ZipFile(const string& filename)
  : myFilename{filename}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ZipHandler::ZipFile::open()
{
  myStream.open(myFilename, std::fstream::in | std::fstream::binary);
  if(!myStream.is_open())
  {
    myLength = 0;
    return false;
  }

  // Disable exceptions completely
  myStream.exceptions(std::ios::goodbit);

  myStream.seekg(0, std::ios::end);
  if(!myStream)  return false;

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
    throw std::runtime_error(errorMessage(ZipError::UNSUPPORTED));

  // Allocate memory for the central directory
  myCd.resize(myEcd.cdSize);

  // Read the central directory
  uInt64 read_length = 0;
  const bool success = readStream(myCd.data(), myEcd.cdStartDiskOffset,
                                  myEcd.cdSize, read_length);
  if(!success)
    throw std::runtime_error(errorMessage(ZipError::FILE_ERROR));
  else if(read_length != myEcd.cdSize)
    throw std::runtime_error(errorMessage(ZipError::FILE_TRUNCATED));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::ZipFile::close()
{
  if(myStream.is_open())
    myStream.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ZipHandler::ZipFile::nextFileIsValidRom()
{
  const ZipHeader* header = nextFile();
  if(!header || header->uncompressedLength == 0)
    return false;

  return Bankswitch::isValidRomName(header->filename);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::ZipFile::readEcd()
{
  vector<uInt8> buffer;

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
      throw std::runtime_error(errorMessage(ZipError::FILE_ERROR));

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
      throw std::runtime_error(errorMessage(ZipError::BAD_SIGNATURE));

    if(buflen >= 65536)
      throw std::runtime_error(errorMessage(ZipError::OUT_OF_MEMORY));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ZipHandler::ZipFile::readStream(uInt8* out, uInt64 offset,
                                     uInt64 length, uInt64& actual)
{
  myStream.seekg(offset, std::ios::beg);
  if(!myStream)  return false;

  myStream.read(reinterpret_cast<char*>(out), length);
  actual = static_cast<uInt64>(myStream.gcount());

  // If we hit EOF early, that's OK (caller checks size)
  return myStream || myStream.eof();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ZipHandler::ZipHeader* ZipHandler::ZipFile::nextFile()
{
  // Make sure we have enough data
  // If we're at or past the end, we're done
  const CentralDirEntryReader reader(myCd.data() + myCdPos);
  if(!reader.signatureCorrect() || ((myCdPos + reader.totalLength()) > myEcd.cdSize))
    return nullptr;

  // Extract file header info
  myHeader.versionCreated     = reader.versionCreated();
  myHeader.versionNeeded      = reader.versionNeeded();
  myHeader.bitFlag            = reader.generalFlag();
  myHeader.compression        = reader.compressionMethod();
  myHeader.crc                = reader.crc32();
  myHeader.compressedLength   = reader.compressedSize();
  myHeader.uncompressedLength = reader.uncompressedSize();
  myHeader.startDiskNumber    = reader.startDisk();
  myHeader.localHeaderOffset  = reader.headerOffset();
  myHeader.filename           = reader.filename();

  // Advance the position
  myCdPos += reader.totalLength();
  return &myHeader;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::ZipFile::decompress(std::span<uInt8> out)
{
  // If we don't have enough buffer, error
  if(out.size() < myHeader.uncompressedLength)
    throw std::runtime_error(errorMessage(ZipError::BUFFER_TOO_SMALL));

  // Make sure the info in the header aligns with what we know
  if(myHeader.startDiskNumber != myEcd.diskNumber)
    throw std::runtime_error(errorMessage(ZipError::UNSUPPORTED));

  // Get the compressed data offset
  const uInt64 offset = getCompressedDataOffset();

  // Handle compression types
  switch(myHeader.compression)
  {
    case 0: decompressDataType0(out, offset); break;
    case 8: decompressDataType8(out, offset); break;
    default:
      throw std::runtime_error(errorMessage(ZipError::UNSUPPORTED));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt64 ZipHandler::ZipFile::getCompressedDataOffset()
{
  // Don't support a number of features
  const GeneralFlagReader flags(myHeader.bitFlag);
  if(myHeader.startDiskNumber != myEcd.diskNumber ||
     myHeader.versionNeeded > 63 || flags.patchData() ||
     flags.encrypted() || flags.strongEncryption())
    throw std::runtime_error(errorMessage(ZipError::UNSUPPORTED));

  // Use a small local buffer; myBuffer is reserved for streaming decompression
  std::array<uInt8, LocalFileHeaderReader::minimumLength()> localBuf{};

  // Read the fixed-sized part of the local file header
  uInt64 read_length = 0;
  const bool success = readStream(localBuf.data(), myHeader.localHeaderOffset,
                                  localBuf.size(), read_length);
  if(!success)
    throw std::runtime_error(errorMessage(ZipError::FILE_ERROR));
  else if(read_length != localBuf.size())
    throw std::runtime_error(errorMessage(ZipError::FILE_TRUNCATED));

  // Compute the final offset
  const LocalFileHeaderReader reader(localBuf.data());
  if(!reader.signatureCorrect())
    throw std::runtime_error(errorMessage(ZipError::BAD_SIGNATURE));

  return myHeader.localHeaderOffset + reader.totalLength();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::ZipFile::decompressDataType0(std::span<uInt8> out,
                                              uInt64 offset)
{
  // The data is uncompressed; just read it
  uInt64 read_length = 0;
  const bool success = readStream(out.data(), offset,
                                  myHeader.compressedLength, read_length);
  if(!success)
    throw std::runtime_error(errorMessage(ZipError::FILE_ERROR));
  else if(read_length != myHeader.compressedLength)
    throw std::runtime_error(errorMessage(ZipError::FILE_TRUNCATED));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::ZipFile::decompressDataType8(std::span<uInt8> out,
                                              uInt64 offset)
{
  // Seek ONCE to start of compressed data
  myStream.seekg(offset, std::ios::beg);
  if(!myStream)
    throw std::runtime_error(errorMessage(ZipError::FILE_ERROR));

  size_t input_remaining = myHeader.compressedLength;

  // Setup zlib stream
  z_stream stream{};
  stream.zalloc    = Z_NULL;
  stream.zfree     = Z_NULL;
  stream.opaque    = Z_NULL;
  stream.avail_in  = 0;
  stream.next_in   = Z_NULL;
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
    throw std::runtime_error(errorMessage(ZipError::DECOMPRESS_ERROR));

  const InflateGuard guard{stream};

  // Streaming loop
  for(;;)
  {
    // Refill input buffer when empty
    if(stream.avail_in == 0 && input_remaining > 0)
    {
      const size_t chunkSize = std::min(input_remaining, DECOMPRESS_BUFSIZE);

      // Read in the next chunk of data
      myStream.read(reinterpret_cast<char*>(myBuffer.data()), chunkSize);
      const auto read_length = static_cast<uInt64>(myStream.gcount());

      // If we read nothing, but still have data left, the file is truncated
      if(read_length == 0)
        throw std::runtime_error(errorMessage(ZipError::FILE_TRUNCATED));

      // Fail only on real errors (not EOF)
      if(!myStream && !myStream.eof())
        throw std::runtime_error(errorMessage(ZipError::FILE_ERROR));

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
      throw std::runtime_error(errorMessage(ZipError::DECOMPRESS_ERROR));
  }

  // Final validation
  // If anything looks funny, report an error
  if(stream.avail_out > 0 || input_remaining > 0)
    throw std::runtime_error(errorMessage(ZipError::DECOMPRESS_ERROR));
}

#endif  /* ZIP_SUPPORT */

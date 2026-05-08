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

#ifndef ZIP_HANDLER_HXX
#define ZIP_HANDLER_HXX

#include <cassert>
#include <fstream>
#include <list>
#include <optional>
#include <unordered_map>

#include "bspf.hxx"

/**
  This class implements a thin wrapper around the zip file management code
  from the MAME project.

  @author  Original code by Aaron Giles, ZipHandler wrapper class and heavy
           modifications/refactoring by Stephen Anthony.
*/
class ZipHandler
{
  public:
    enum class ZipError: uInt8 {
      NONE,
      NO_ROMS,
      OUT_OF_MEMORY,
      FILE_ERROR,
      FILE_NOT_FOUND,
      BAD_SIGNATURE,
      DECOMPRESS_ERROR,
      FILE_TRUNCATED,
      FILE_CORRUPT,
      UNSUPPORTED,
      LZMA_UNSUPPORTED,
      BUFFER_TOO_SMALL
    };

    class ZipException : public std::runtime_error
    {
      public:
        explicit ZipException(ZipError err)
          : std::runtime_error(toString(err)), myError{err} { }

        ZipError code() const noexcept { return myError; }

      private:
        ZipError myError;

        static constexpr const char* toString(ZipError err) noexcept
        {
          switch (err)
          {
            case ZipError::NONE: return "ZIP NONE";
            case ZipError::NO_ROMS: return "ZIP NO_ROMS";
            case ZipError::OUT_OF_MEMORY: return "ZIP OUT_OF_MEMORY";
            case ZipError::FILE_ERROR: return "ZIP FILE_ERROR";
            case ZipError::FILE_NOT_FOUND: return "ZIP FILE_NOT_FOUND";
            case ZipError::BAD_SIGNATURE: return "ZIP BAD_SIGNATURE";
            case ZipError::DECOMPRESS_ERROR: return "ZIP DECOMPRESS_ERROR";
            case ZipError::FILE_TRUNCATED: return "ZIP FILE_TRUNCATED";
            case ZipError::FILE_CORRUPT: return "ZIP FILE_CORRUPT";
            case ZipError::UNSUPPORTED: return "ZIP UNSUPPORTED";
            case ZipError::LZMA_UNSUPPORTED: return "ZIP LZMA_UNSUPPORTED";
            case ZipError::BUFFER_TOO_SMALL: return "ZIP BUFFER_TOO_SMALL";
          }
          return "ZIP UNKNOWN_ERROR";
        }
    };

  public:
    ZipHandler() = default;
    ~ZipHandler() = default;

    // Open ZIP file for processing
    // An exception will be thrown on any errors
    void open(string_view filename);

    // Find a file by name in the currently opened ZIP
    // Returns {size, true} if found, {0, false} otherwise
    std::pair<size_t, bool> find(string_view name);

    // Iterate over each entry in the ZIP and apply the given function
    template<typename F>
    void forEachEntry(F fn);

    // Decompress the currently selected file and return its length
    // An exception will be thrown on any errors
    // TODO: move to vector instead of allocating unique_ptr each time
    //       will require changes to FSNode and friends
    uInt64 decompress(string_view name, ByteBuffer& image);

    // Give the first ROM file (with a valid extension) found
    std::optional<std::pair<string_view, size_t>> firstRom() const;

    // Answer the number of ROM files (with a valid extension) found
    uInt16 romFiles() const { return myZip ? myZip->myRomfiles : 0; }

  private:
    // Contains extracted file header information
    struct ZipHeader
    {
      uInt16 versionCreated{0};      // version made by
      uInt16 versionNeeded{0};       // version needed to extract
      uInt16 bitFlag{0};             // general purpose bit flag
      uInt16 compression{0};         // compression method
      uInt16 fileTime{0};            // last mod file time
      uInt16 fileDate{0};            // last mod file date
      uInt32 crc{0};                 // crc-32
      uInt64 compressedLength{0};    // compressed size
      uInt64 uncompressedLength{0};  // uncompressed size
      uInt32 startDiskNumber{0};     // disk number start
      uInt64 localHeaderOffset{0};   // relative offset of local header
      string_view filename;          // view into myCd — valid for ZipFile lifetime
    };

    // Contains extracted end of central directory information
    struct ZipEcd
    {
      uInt32 diskNumber{0};        // number of this disk
      uInt32 cdStartDiskNumber{0}; // number of the disk with the start of the central directory
      uInt64 cdDiskEntries{0};     // total number of entries in the central directory on this disk
      uInt64 cdTotalEntries{0};    // total number of entries in the central directory
      uInt64 cdSize{0};            // size of the central directory
      uInt64 cdStartDiskOffset{0}; // offset of start of central directory with respect to the starting disk number
    };

    // Describes an open ZIP file
    struct ZipFile
    {
      static constexpr size_t DECOMPRESS_BUFSIZE = 128_KB;

      string  myFilename;     // copy of ZIP filename (for caching)
      std::fstream myStream;  // C++ fstream file handle
      uInt64  myLength{0};    // length of zip file
      uInt16  myRomfiles{0};  // number of ROM files in central directory

      // Used when only one ROM is present
      std::optional<string_view> myFirstRomName;
      size_t myFirstRomSize{0};

      ZipEcd  myEcd;  // end of central directory

      vector<uInt8>     myCd;       // central directory raw data
      vector<ZipHeader> myHeaders;  // stable parsed headers

      std::array<uInt8, DECOMPRESS_BUFSIZE> myBuffer{}; // buffer for decompression

      /** Lookup support */
      mutable std::unordered_map<string_view, size_t> myHeaderIndex;
      mutable bool myIndexBuilt{false};

      /** Constructor */
      explicit ZipFile(string_view filename);

      /** Open the file and set up the internal stream buffer */
      bool open();

      /** Read the ZIP contents from the internal stream buffer */
      void initialize();

      /** Close previously opened internal stream buffer */
      void close();

      /** Read the ECD data */
      void readEcd();

      /** Read data from stream */
      bool readStream(uInt8* out, uInt64 offset, uInt64 length, uInt64& actual);

      /** Ensure index exists (lazy) */
      void ensureIndex() const;

      /** Find header by name */
      const ZipHeader* findHeader(string_view name) const;

      /** Decompress the most recently found file in the ZIP into target buffer */
      void decompress(const ZipHeader& header, ByteMSpan out);

      /** Return the offset of the compressed data */
      uInt64 getCompressedDataOffset(const ZipHeader& header);

      /** Decompress type 0 data (which is uncompressed) */
      void decompressDataType0(const ZipHeader& header, ByteMSpan out, uInt64 offset);

      /** Decompress type 8 data (which is deflated) */
      void decompressDataType8(const ZipHeader& header, ByteMSpan out, uInt64 offset);
    };
    using ZipFilePtr = unique_ptr<ZipFile>;

    /** Classes to parse the ZIP metadata in an abstracted way */
    class ReaderBase
    {
      protected:
        explicit ReaderBase(const uInt8* const b) : myBuf{b} { }

        constexpr uInt8 read_byte(size_t offs) const
        {
          return myBuf[offs];
        }
        constexpr uInt16 read_word(size_t offs) const
        {
          return (static_cast<uInt16>(myBuf[offs + 1]) << 8) |
                 (static_cast<uInt16>(myBuf[offs + 0]) << 0);
        }
        constexpr uInt32 read_dword(size_t offs) const
        {
          return (static_cast<uInt32>(myBuf[offs + 3]) << 24) |
                 (static_cast<uInt32>(myBuf[offs + 2]) << 16) |
                 (static_cast<uInt32>(myBuf[offs + 1]) << 8)  |
                 (static_cast<uInt32>(myBuf[offs + 0]) << 0);
        }
        constexpr uInt64 read_qword(size_t offs) const
        {
          return (static_cast<uInt64>(myBuf[offs + 7]) << 56) |
                 (static_cast<uInt64>(myBuf[offs + 6]) << 48) |
                 (static_cast<uInt64>(myBuf[offs + 5]) << 40) |
                 (static_cast<uInt64>(myBuf[offs + 4]) << 32) |
                 (static_cast<uInt64>(myBuf[offs + 3]) << 24) |
                 (static_cast<uInt64>(myBuf[offs + 2]) << 16) |
                 (static_cast<uInt64>(myBuf[offs + 1]) << 8)  |
                 (static_cast<uInt64>(myBuf[offs + 0]) << 0);
        }
        string_view read_string(size_t offs, size_t len = string_view::npos) const
        {
          return {reinterpret_cast<const char*>(myBuf + offs), len};
        }

      private:
        const uInt8* myBuf{nullptr};
    };

    class LocalFileHeaderReader : public ReaderBase
    {
      public:
        explicit LocalFileHeaderReader(const uInt8* const b) : ReaderBase(b) { }

        uInt32  signature() const         { return read_dword(0x00); }
        uInt8   versionNeeded() const     { return read_byte(0x04);  }
        uInt8   osNeeded() const          { return read_byte(0x05);  }
        uInt16  generalFlag() const       { return read_word(0x06);  }
        uInt16  compressionMethod() const { return read_word(0x08);  }
        uInt16  modifiedTime() const      { return read_word(0x0a);  }
        uInt16  modifiedDate() const      { return read_word(0x0c);  }
        uInt32  crc32() const             { return read_dword(0x0e); }
        uInt32  compressedSize() const    { return read_dword(0x12); }
        uInt32  uncompressedSize() const  { return read_dword(0x16); }
        uInt16  filenameLength() const    { return read_word(0x1a);  }
        uInt16  extraFieldLength() const  { return read_word(0x1c);  }
        string_view filename() const {
          return read_string(0x1e, filenameLength());
        }

        bool signatureCorrect() const  { return signature() == 0x04034b50; }

        size_t totalLength() const { return minimumLength() + filenameLength() + extraFieldLength(); }
        static constexpr size_t minimumLength() { return 0x1e; }
    };

    class CentralDirEntryReader : public ReaderBase
    {
      public:
        explicit CentralDirEntryReader(const uInt8* const b) : ReaderBase(b) { }

        uInt32 signature() const          { return read_dword(0x00); }
        uInt8  versionCreated() const     { return read_byte(0x04);  }
        uInt8  osCreated() const          { return read_byte(0x05);  }
        uInt8  versionNeeded() const      { return read_byte(0x06);  }
        uInt8  osNeeded() const           { return read_byte(0x07);  }
        uInt16 generalFlag() const        { return read_word(0x08);  }
        uInt16 compressionMethod() const  { return read_word(0x0a);  }
        uInt16 modifiedTime() const       { return read_word(0x0c);  }
        uInt16 modifiedDate() const       { return read_word(0x0e);  }
        uInt32 crc32() const              { return read_dword(0x10); }
        uInt32 compressedSize() const     { return read_dword(0x14); }
        uInt32 uncompressedSize() const   { return read_dword(0x18); }
        uInt16 filenameLength() const     { return read_word(0x1c);  }
        uInt16 extraFieldLength() const   { return read_word(0x1e);  }
        uInt16 fileCommentLength() const  { return read_word(0x20);  }
        uInt16 startDisk() const          { return read_word(0x22);  }
        uInt16 intFileAttr() const        { return read_word(0x24);  }
        uInt32 extFileAttr() const        { return read_dword(0x26); }
        uInt32 headerOffset() const       { return read_dword(0x2a); }
        string_view filename() const {
          return read_string(0x2e, filenameLength());
        }
        string_view fileComment() const {
          return read_string(0x2e + filenameLength() + extraFieldLength(),
                             fileCommentLength());
        }

        bool signatureCorrect() const { return signature() == 0x02014b50; }

        size_t totalLength() const { return minimumLength() + filenameLength() + extraFieldLength() + fileCommentLength(); }
        static constexpr size_t minimumLength() { return 0x2e; }
    };

    class EcdReader : public ReaderBase
    {
      public:
        explicit EcdReader(const uInt8* const b) : ReaderBase(b) { }

        uInt32 signature() const       { return read_dword(0x00); }
        uInt16 thisDiskNo() const      { return read_word(0x04);  }
        uInt16 dirStartDisk() const    { return read_word(0x06);  }
        uInt16 dirDiskEntries() const  { return read_word(0x08);  }
        uInt16 dirTotalEntries() const { return read_word(0x0a);  }
        uInt32 dirSize() const         { return read_dword(0x0c); }
        uInt32 dirOffset() const       { return read_dword(0x10); }
        uInt16 commentLength() const   { return read_word(0x14);  }
        string_view comment() const {
          return read_string(0x16, commentLength());
        }

        bool signatureCorrect() const  { return signature() == 0x06054b50; }

        size_t totalLength() const { return minimumLength() + commentLength(); }
        static constexpr size_t minimumLength() { return 0x16; }
    };

    class GeneralFlagReader
    {
      public:
        explicit GeneralFlagReader(uInt16 val) : myValue{val} { }

        bool   encrypted() const           { return static_cast<bool>(myValue & 0x0001); }
        bool   implode8kDict() const       { return static_cast<bool>(myValue & 0x0002); }
        bool   implode3Trees() const       { return static_cast<bool>(myValue & 0x0004); }
        uInt32 deflateOption() const       { return static_cast<uInt32>((myValue >> 1) & 0x0003); }
        bool   lzmaEosMark() const         { return static_cast<bool>(myValue & 0x0002); }
        bool   useDescriptor() const       { return static_cast<bool>(myValue & 0x0008); }
        bool   patchData() const           { return static_cast<bool>(myValue & 0x0020); }
        bool   strongEncryption() const    { return static_cast<bool>(myValue & 0x0040); }
        bool   utf8Encoding() const        { return static_cast<bool>(myValue & 0x0800); }
        bool   directoryEncryption() const { return static_cast<bool>(myValue & 0x2000); }

      private:
        uInt16 myValue{0};
    };

  private:
    /** Search cache for given ZIP file */
    ZipFilePtr findCached(string_view filename);

    /** Close a ZIP file and add it to the cache */
    void addToCache();

  private:
    static constexpr size_t CACHE_SIZE = 64; // number of open files to cache

    ZipFilePtr myZip;

    // LRU cache: map filename -> (ZipFilePtr, iterator in list)
    std::unordered_map<string,
      std::pair<ZipFilePtr, std::list<string>::iterator>,
      BSPF::StringHash,
      std::equal_to<>> myZipCache;
    std::list<string> myCacheOrder; // front = oldest, back = newest

  private:
    // Following constructors and assignment operators not supported
    ZipHandler(const ZipHandler&) = delete;
    ZipHandler(ZipHandler&&) = delete;
    ZipHandler& operator=(const ZipHandler&) = delete;
    ZipHandler& operator=(ZipHandler&&) = delete;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename F>
void ZipHandler::forEachEntry(F fn)
{
  assert(myZip && "forEachEntry called without open()");
  for(const auto& header: myZip->myHeaders)
    fn(header.filename, header.uncompressedLength);
}

#endif  // ZIP_HANDLER_HXX

#endif  // ZIP_SUPPORT

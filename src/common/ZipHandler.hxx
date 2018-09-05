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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef ZIP_HANDLER_HXX
#define ZIP_HANDLER_HXX

#include <array>

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
    ZipHandler();

    // Open ZIP file for processing
    // An exception will be thrown on any errors
    void open(const string& filename);

    // The following form an iterator for processing the filenames in the ZIP file
    void reset();          // Reset iterator to first file
    bool hasNext() const;  // Answer whether there are more files present
    const string& next();  // Get next file

    // Decompress the currently selected file and return its length
    // An exception will be thrown on any errors
    uInt64 decompress(BytePtr& image);

    // Answer the number of ROM files (with a valid extension) found
    uInt16 romFiles() const { return myZip ? myZip->myRomfiles : 0; }

  private:
    // Error types
    enum class ZipError
    {
      NONE = 0,
      OUT_OF_MEMORY,
      FILE_ERROR,
      BAD_SIGNATURE,
      DECOMPRESS_ERROR,
      FILE_TRUNCATED,
      FILE_CORRUPT,
      UNSUPPORTED,
      LZMA_UNSUPPORTED,
      BUFFER_TOO_SMALL
    };

    // Contains extracted file header information
    struct ZipHeader
    {
      uInt16 versionCreated;      // version made by
      uInt16 versionNeeded;       // version needed to extract
      uInt16 bitFlag;             // general purpose bit flag
      uInt16 compression;         // compression method
      uInt16 fileTime;            // last mod file time
      uInt16 fileDate;            // last mod file date
      uInt32 crc;                 // crc-32
      uInt64 compressedLength;    // compressed size
      uInt64 uncompressedLength;  // uncompressed size
      uInt32 startDiskNumber;     // disk number start
      uInt64 localHeaderOffset;   // relative offset of local header
      string filename;            // filename

      /** Constructor */
      ZipHeader();
    };

    // Contains extracted end of central directory information
    struct ZipEcd
    {
      uInt32 diskNumber;        // number of this disk
      uInt32 cdStartDiskNumber; // number of the disk with the start of the central directory
      uInt64 cdDiskEntries;     // total number of entries in the central directory on this disk
      uInt64 cdTotalEntries;    // total number of entries in the central directory
      uInt64 cdSize;            // size of the central directory
      uInt64 cdStartDiskOffset; // offset of start of central directory with respect to the starting disk number

      /** Constructor */
      ZipEcd();
    };

    // Describes an open ZIP file
    struct ZipFile
    {
      string  myFilename; // copy of ZIP filename (for caching)
      fstream myStream;   // C++ fstream file handle
      uInt64  myLength;   // length of zip file
      uInt16  myRomfiles; // number of ROM files in central directory

      ZipEcd    myEcd;    // end of central directory

      BytePtr   myCd;     // central directory raw data
      uInt64    myCdPos;  // position in central directory
      ZipHeader myHeader; // current file header

      BytePtr myBuffer;   // buffer for decompression

      /** Constructor */
      explicit ZipFile(const string& filename);

      /** Open the file and set up the internal stream buffer*/
      bool open();

      /** Read the ZIP contents from the internal stream buffer */
      void initialize();

      /** Close previously opened internal stream buffer */
      void close();

      /** Read the ECD data */
      void readEcd();

      /** Read data from stream */
      bool readStream(BytePtr& out, uInt64 offset, uInt64 length, uInt64& actual);

      /** Return the next entry in the ZIP file */
      const ZipHeader* const nextFile();

      /** Decompress the most recently found file in the ZIP into target buffer */
      void decompress(BytePtr& out, uInt64 length);

      /** Return the offset of the compressed data */
      uInt64 getCompressedDataOffset();

      /** Decompress type 0 data (which is uncompressed) */
      void decompressDataType0(uInt64 offset, BytePtr& out, uInt64 length);

      /** Decompress type 8 data (which is deflated) */
      void decompressDataType8(uInt64 offset, BytePtr& out, uInt64 length);
    };
    using ZipFilePtr = unique_ptr<ZipFile>;

    class EcdReader
    {
      public:
        explicit EcdReader(const uInt8* const b) : myBuf(b) { }

        uInt32 signature() const       { return read_dword(myBuf + 0x00); }
        uInt16 thisDiskNo() const      { return read_word(myBuf + 0x04);  }
        uInt16 dirStartDisk() const    { return read_word(myBuf + 0x06);  }
        uInt16 dirDiskEntries() const  { return read_word(myBuf + 0x08);  }
        uInt16 dirTotalEntries() const { return read_word(myBuf + 0x0a);  }
        uInt32 dirSize() const         { return read_dword(myBuf + 0x0c); }
        uInt32 dirOffset() const       { return read_dword(myBuf + 0x10); }
        uInt16 commentLength() const   { return read_word(myBuf + 0x14);  }
        string comment() const         { return read_string(myBuf + 0x16, commentLength()); }

        bool signatureCorrect() const  { return signature() == 0x06054b50; }

        size_t totalLength() const { return minimumLength() + commentLength(); }
        static size_t minimumLength() { return 0x16; }

      private:
        const uInt8* const myBuf;
    };

    class CentralDirEntryReader
    {
      public:
        explicit CentralDirEntryReader(const uInt8* const b) : myBuf(b) { }

        uInt32 signature() const          { return read_dword(myBuf + 0x00); }
        uInt8  versionCreated() const     { return myBuf[0x04];              }
        uInt8  osCreated() const          { return myBuf[0x05];              }
        uInt8  versionNeeded() const      { return myBuf[0x06];              }
        uInt8  osNeeded() const           { return myBuf[0x07];              }
        uInt16 generalFlag() const        { return read_word(myBuf + 0x08);  }
        uInt16 compressionMethod() const  { return read_word(myBuf + 0x0a);  }
        uInt16 modifiedTime() const       { return read_word(myBuf + 0x0c);  }
        uInt16 modifiedDate() const       { return read_word(myBuf + 0x0e);  }
        uInt32 crc32() const              { return read_dword(myBuf + 0x10); }
        uInt32 compressedSize() const     { return read_dword(myBuf + 0x14); }
        uInt32 uncompressedSize() const   { return read_dword(myBuf + 0x18); }
        uInt16 filenameLength() const     { return read_word(myBuf + 0x1c);  }
        uInt16 extraFieldLength() const   { return read_word(myBuf + 0x1e);  }
        uInt16 fileCommentLength() const  { return read_word(myBuf + 0x20);  }
        uInt16 startDisk() const          { return read_word(myBuf + 0x22);  }
        uInt16 intFileAttr() const        { return read_word(myBuf + 0x24);  }
        uInt32 extFileAttr() const        { return read_dword(myBuf + 0x26); }
        uInt32 headerOffset() const       { return read_dword(myBuf + 0x2a); }
        string filename() const           { return read_string(myBuf + 0x2e, filenameLength()); }
        string fileComment() const        { return read_string(myBuf + 0x2e + filenameLength() + extraFieldLength(), fileCommentLength()); }

        bool signatureCorrect() const { return signature() == 0x02014b50; }

        size_t totalLength() const { return minimumLength() + filenameLength() + extraFieldLength() + fileCommentLength(); }
        static size_t minimumLength() { return 0x2e; }

      private:
        const uInt8* const myBuf;
    };

  private:
    /** Get message for given ZipError enumeration */
    string errorMessage(ZipError err) const;

    /** Search cache for given ZIP file */
    ZipFilePtr findCached(const string& filename);

    /** Close a ZIP file and add it to the cache */
    void addToCache();

    /** Convenience functions to read specific datatypes */
    static inline uInt16 read_word(const uInt8* const buf)
    {
      uInt16 p0 = uInt16(buf[0]), p1 = uInt16(buf[1]);
      return (p1 << 8) | p0;
    }
    static inline uInt32 read_dword(const uInt8* const buf)
    {
      return (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
    }
    static inline string read_string(const uInt8* const buf, size_t len = string::npos)
    {
      return (buf == nullptr || *buf == '\0') ? "" :
          string(reinterpret_cast<const char*>(buf), len);
    }

  private:
    static constexpr uInt32 DECOMPRESS_BUFSIZE = 16384;
    static constexpr uInt32 CACHE_SIZE = 8; // number of open files to cache

    ZipFilePtr myZip;
    std::array<ZipFilePtr, CACHE_SIZE> myZipCache;

  private:
    // Following constructors and assignment operators not supported
    ZipHandler(const ZipHandler&) = delete;
    ZipHandler(ZipHandler&&) = delete;
    ZipHandler& operator=(const ZipHandler&) = delete;
    ZipHandler& operator=(ZipHandler&&) = delete;
};

#endif  /* ZIP_HANDLER_HXX */

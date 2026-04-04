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

#ifndef SERIALIZER_HXX
#define SERIALIZER_HXX

#include <bit>
#include <cstring>
#include <fstream>
#include <optional>
#include <span>

#include "bspf.hxx"

/**
  This class implements a Serializer device, whereby data is serialized and
  read from/written to a binary stream in a system-independent way.  The
  stream can be either an actual file, or an in-memory structure.  Which
  one is used is determined by which constructor is called.

  Bytes are written as characters, shorts as 2 characters (16-bits),
  integers as 4 characters (32-bits), long integers and doubles as 8 bytes
  (64-bits), strings are written as characters prepended by the length of
  the string, boolean values are written using a special character pattern.

  @author  Stephen Anthony
*/
class Serializer
{
  public:
    // This is used when opening a file-based Serializer; a memory-based
    // Serializer is essentially always read and write
    enum class FileMode: uInt8 { ReadOnly, ReadWrite, ReadWriteTrunc };

  public:
    /**
      Creates a new Serializer device for streaming binary data.

      If a filename is provided (c'tor 1), the stream will be to the
      given filename opened in the given FileMode.  If a file is opened
      readonly, we can never write to it.

      Otherwise if (c'tor 2), the stream will be in memory.

      The valid() method must immediately be called to verify the stream
      was correctly initialized.
    */
    explicit Serializer(string_view filename, FileMode fm = FileMode::ReadWrite);
    Serializer();

    ~Serializer() = default;

  public:
    /**
      Answers whether the serializer is currently initialized for reading
      and writing.
    */
    explicit operator bool() const { return myMemory || myFile; }

    /**
      Sets the read/write location to the given offset in the stream.
    */
    void setPosition(size_t pos);

    /**
      Resets the read/write location to the beginning of the stream.
    */
    void rewind();

    /**
      Returns the current total size of the stream.
    */
    size_t size();

    /**
      Reads a byte value (unsigned 8-bit) from the current input stream.

      @result The byte value which has been read from the stream.
    */
    uInt8 getByte();

    /**
      Reads a byte array (unsigned 8-bit) from the current input stream.

      @param array  The storage space to write bytes to
    */
    void getByteArray(std::span<uInt8> array);

    /**
      Reads a short value (unsigned 16-bit) from the current input stream.

      @result The short value which has been read from the stream.
    */
    uInt16 getShort();

    /**
      Reads a short array (unsigned 16-bit) from the current input stream.

      @param array  The storage space to write shorts to
    */
    void getShortArray(std::span<uInt16> array);

    /**
      Reads an int value (unsigned 32-bit) from the current input stream.

      @result The int value which has been read from the stream.
    */
    uInt32 getInt();

    /**
      Reads an integer array (unsigned 32-bit) from the current input stream.

      @param array  The storage space to write ints to
    */
    void getIntArray(std::span<uInt32> array);

    /**
      Reads a long int value (unsigned 64-bit) from the current input stream.

      @result The long int value which has been read from the stream.
    */
    uInt64 getLong();

    /**
      Reads a double value (signed 64-bit) from the current input stream.

      @result The double value which has been read from the stream.
    */
    double getDouble();

    /**
      Reads a string from the current input stream.

      @result The string which has been read from the stream.
    */
    string getString();

    /**
      Reads a boolean value from the current input stream.

      @result The boolean value which has been read from the stream.
    */
    bool getBool();

    /**
      Writes an byte value (unsigned 8-bit) to the current output stream.

      @param value The byte value to write to the output stream.
    */
    void putByte(uInt8 value);

    /**
      Writes a byte array (unsigned 8-bit) to the current output stream.

      @param array  The storage space to read bytes from
    */
    void putByteArray(std::span<const uInt8> array);

    /**
      Writes a short value (unsigned 16-bit) to the current output stream.

      @param value The short value to write to the output stream.
    */
    void putShort(uInt16 value);

    /**
      Writes a short array (unsigned 16-bit) to the current output stream.

      @param array  The storage space to read shorts from
    */
    void putShortArray(std::span<const uInt16> array);

    /**
      Writes an int value (unsigned 32-bit) to the current output stream.

      @param value The int value to write to the output stream.
    */
    void putInt(uInt32 value);

    /**
      Writes an integer array (unsigned 32-bit) to the current output stream.

      @param array  The storage space to read ints from
    */
    void putIntArray(std::span<const uInt32> array);

    /**
      Writes a long int value (unsigned 64-bit) to the current output stream.

      @param value The long int value to write to the output stream.
    */
    void putLong(uInt64 value);

    /**
      Writes a double value (signed 64-bit) to the current output stream.

      @param value The double value to write to the output stream.
    */
    void putDouble(double value);

    /**
      Writes a string(view) to the current output stream.

      @param str The string to write to the output stream.
    */
    void putString(string_view str);

    /**
      Writes a boolean value to the current output stream.

      @param b The boolean value to write to the output stream.
    */
    void putBool(bool b);

  private:
    // Raw Read/Write
    template<typename T> T readRaw();
    template<typename T> void writeRaw(T value);

    // Endian conversion helpers
    template<typename T> static constexpr T byteswap(T v) {  // TODO: until C++23
      static_assert(sizeof(T) == 1 || sizeof(T) == 2 ||
                    sizeof(T) == 4 || sizeof(T) == 8,
                    "Unsupported type size for byteswap");

      if constexpr(sizeof(T) > 1) {
        auto src = std::bit_cast<std::array<std::byte, sizeof(T)>>(v);
        std::reverse(src.begin(), src.end());
        return std::bit_cast<T>(src);
      }
      return v;
    }
    template<typename T> static constexpr T toLittle(T v) {
      if constexpr(std::endian::native != std::endian::little)
        return byteswap(v);
      return v;
    }
    template<typename T> static constexpr T fromLittle(T v) {
      if constexpr (std::endian::native != std::endian::little)
        return byteswap(v);
      return v;
    }

  private:
    // Memory backend
    struct MemoryStream {
      vector<std::byte> buffer;
      size_t pos{0};
      size_t size{0};

      MemoryStream() { }  // NOLINT: can't use = default here
                          // possible clang bug; fails to compile with default

      void ensureCapacity(size_t additional) {
        ensureSize(pos + additional);
      }
      void ensureSize(size_t absolute) {
        if(absolute > buffer.size()) {
          const size_t newSize = std::max(buffer.size() * 2, absolute);
          buffer.resize(newSize);
        }
      }
    };
    std::optional<MemoryStream> myMemory;

    // File backend
    struct FileStream {
      std::fstream stream;

      vector<std::byte> writeBuffer;
      size_t bufferPos{0};
      static constexpr size_t MAX_BUF_SIZE = 4_KB;

      FileStream() { writeBuffer.resize(MAX_BUF_SIZE); }

      FileStream(FileStream&& other) noexcept
        : stream(std::move(other.stream)),
          writeBuffer(std::move(other.writeBuffer)),
          bufferPos(other.bufferPos)
      {
        other.bufferPos = 0;
      }
      ~FileStream() { stream.exceptions(std::ios::goodbit); flushBuffer(); }

      void flushBuffer() {
        if(bufferPos > 0) {
          stream.write(reinterpret_cast<const char*>(writeBuffer.data()), bufferPos);
          bufferPos = 0;
        }
      }

      // Buffer small writes to a vector, and flush to disk occasionally
      void writeBuffered(const uInt8* data, size_t dataSize) {
        size_t offset = 0;
        while(dataSize > 0) {
          const size_t space = MAX_BUF_SIZE - bufferPos;
          const size_t toWrite = std::min(space, dataSize);
          std::memcpy(writeBuffer.data() + bufferPos, data + offset, toWrite);
          bufferPos += toWrite;
          offset += toWrite;
          dataSize -= toWrite;

          if(bufferPos == MAX_BUF_SIZE)
            flushBuffer();
        }
      }

      FileStream(const FileStream&) = delete;
      FileStream& operator=(const FileStream&) = delete;
      FileStream& operator=(FileStream&&) = delete;
    };
    std::optional<FileStream> myFile;

    // Using these values for legacy reasons
    static constexpr uInt8 TruePattern = 0xfe, FalsePattern = 0x01;

  private:
    // Following constructors and assignment operators not supported
    Serializer(const Serializer&) = delete;
    Serializer(Serializer&&) = delete;
    Serializer& operator=(const Serializer&) = delete;
    Serializer& operator=(Serializer&&) = delete;
};

#endif

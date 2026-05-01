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

#ifndef SPANSTREAM_HXX
#define SPANSTREAM_HXX

#include <streambuf>
#include <istream>
#include <span>

/**
  This class implements a read-only streambuf backed by a contiguous
  memory span.  This is used to avoid copying data into a string and
  then injecting into a stringstream, just to get a stream interface.

  This is only until we get proper <spanstream> support in C++23.

  @author  Stephen Anthony
 */
class SpanStreamBuf : public std::streambuf
{
  public:
    explicit SpanStreamBuf(std::span<const char> data)
    {
      // streambuf's get area works with char* — cast away const here,
      // which is safe because we never write through these pointers.
      auto* begin = const_cast<char*>(data.data());
      setg(begin, begin, begin + data.size());
    }

    // Convenience constructor for raw pointer + size (e.g. uInt8* buffers
    // common in emulator code), avoids casting at every call site.
    explicit SpanStreamBuf(const void* data, size_t size)
      : SpanStreamBuf{std::span<const char>{
          static_cast<const char*>(data), size}} { }

  protected:
    pos_type seekoff(off_type off, std::ios_base::seekdir dir,
                     std::ios_base::openmode which) override
    {
      if(!(which & std::ios_base::in))
        return static_cast<pos_type>(static_cast<off_type>(-1));

      char* target = nullptr;
      if(dir == std::ios_base::beg)       target = eback() + off;
      else if(dir == std::ios_base::cur)  target = gptr()  + off;
      else if(dir == std::ios_base::end)  target = egptr() + off;
      else return static_cast<pos_type>(static_cast<off_type>(-1));

      if(target < eback() || target > egptr())
        return static_cast<pos_type>(static_cast<off_type>(-1));

      setg(eback(), target, egptr());
      return {target - eback()};
    }

    pos_type seekpos(pos_type pos, std::ios_base::openmode which) override
    {
      return seekoff(static_cast<off_type>(pos), std::ios_base::beg, which);
    }
};

/**
  SpanStream wraps SpanStreamBuf as a std::istream.

  Member declaration order matters here: myBuf must be listed BEFORE the
  std::istream base in any initialiser list reasoning, but because
  std::istream stores only a pointer to the streambuf (never dereferencing
  it during its own construction), passing &myBuf while myBuf is not yet
  fully constructed is safe — the address is stable from the moment the
  object's storage is allocated.  The declaration order below (istream base
  first, then myBuf) matches C++ base-then-member initialisation order and
  is the canonical pattern for embedded-buffer streams.
 */
class SpanStream : public std::istream
{
  public:
    explicit SpanStream(std::span<const char> data)
      : std::istream{&myBuf}, myBuf{data} { }

    // Convenience constructor matching SpanStreamBuf(const void*, size_t).
    explicit SpanStream(const void* data, size_t size)
      : std::istream{&myBuf}, myBuf{data, size} { }

  private:
    SpanStreamBuf myBuf;
};

#endif  // SPANSTREAM_HXX

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

#ifndef BASE_HXX
#define BASE_HXX

#include <iomanip>  // Not needed here, but for classes that include this one

#include "bspf.hxx"

namespace Common {

/**
  This class implements several functions for converting integer data
  into strings in multiple bases, with different formats (# of characters,
  upper/lower-case, etc).

  @author  Stephen Anthony
*/
class Base
{
  public:
    // The base to use for conversion from integers to strings
    // Note that the actual number of places will be determined by
    // the magnitude of the value itself in the general case
    enum class Fmt: uInt8 {
      _16,      // base 16: 2, 4, 8 bytes (depending on value)
      _16_1,    // base 16: 1 byte wide
      _16_2,    // base 16: 2 bytes wide
      _16_2_2,  // base 16: fractional value shown as xx.xx
      _16_3_2,  // base 16: fractional value shown as xxx.xx
      _16_4,    // base 16: 4 bytes wide
      _16_8,    // base 16: 8 bytes wide
      _10,      // base 10: 3 or 5 bytes (depending on value)
      _10_02,   // base 10: 02 digits
      _10_3,    // base 10: 3 digits
      _10_4,    // base 10: 4 digits
      _10_5,    // base 10: 5 digits
      _10_6,    // base 10: 6 digits
      _10_8,    // base 10: 8 digits
      _2,       // base 2:  8 or 16 bits (depending on value)
      _2_8,     // base 2:  1 byte (8 bits) wide
      _2_16,    // base 2:  2 bytes (16 bits) wide
      DEFAULT
    };

  public:
    /** Get/set the number base when parsing numeric values */
    static void setFormat(Fmt base) { myDefaultBase = base; }
    static Base::Fmt format()       { return myDefaultBase; }

    /** Get/set HEX output to be upper/lower case */
    static void setHexUppercase(bool enable) {
      myHexUppercase = enable;
      myHexFlags = std::ios_base::hex |
        (myHexUppercase ? std::ios_base::uppercase : std::ios_base::fmtflags(0));
    }
    static bool hexUppercase() { return myHexUppercase; }

    /** Convert integer to a string in the given base format */
    static string toString(int value, Fmt outputBase = Fmt::DEFAULT)
    {
      array<char, 32> buf{};
      auto* end = toChars(buf.data(), value, outputBase);
      return {buf.data(), end};
    }

    /** Output HEX digits in 0.5/1/2/4 byte format */
    template<int WIDTH>
    static std::ostream& HEX(std::ostream& os) {
      os.setf(myHexFlags, std::ios_base::basefield | std::ios_base::uppercase);
      os.width(WIDTH);
      os.fill('0');
      return os;
    }
    static std::ostream& HEX1(std::ostream& os) { return HEX<1>(os); }
    static std::ostream& HEX2(std::ostream& os) { return HEX<2>(os); }
    static std::ostream& HEX3(std::ostream& os) { return HEX<3>(os); }
    static std::ostream& HEX4(std::ostream& os) { return HEX<4>(os); }
    static std::ostream& HEX8(std::ostream& os) { return HEX<8>(os); }

  private:
    // Core fast path
    static char* toChars(char* out, int value, Fmt fmt = Fmt::DEFAULT);

    // HEX (fast lookup table)
    static char* writeHex(char* out, uInt32 v, int width);

    // DECIMAL (no division for small widths is possible,
    // but keeping it simple and still fast)
    static char* writeDec(char* out, int value, int width, char padChar);

    // BINARY (branch-free inner loop)
    static char* writeBinary(char* out, int value, Fmt fmt);

  private:
    // Default format to use when none is specified
    static inline Fmt myDefaultBase = Fmt::_16;

    // Upper or lower case for HEX digits (default to lowercase)
    static inline bool myHexUppercase = false;
    static std::ios_base::fmtflags myHexFlags;

  private:
    // Following constructors and assignment operators not supported
    Base() = delete;
    ~Base() = delete;
    Base(const Base&) = delete;
    Base(Base&&) = delete;
    Base& operator=(const Base&) = delete;
    Base& operator=(Base&&) = delete;
};

}  // Namespace Common

#endif

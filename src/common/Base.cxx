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

#include "Base.hxx"

namespace Common {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char* Base::toChars(char* out, int value, Fmt fmt)
{
  if(fmt == Fmt::DEFAULT)
    fmt = myDefaultBase;

  switch(fmt)
  {
    case Fmt::_2:
    case Fmt::_2_8:
    case Fmt::_2_16:
      return writeBinary(out, value, fmt);

    case Fmt::_10:
      return writeDec(out, value, (value > -0x100 && value < 0x100) ? 3 : 5, ' ');

    case Fmt::_10_02: return writeDec(out, value, 2, '0');
    case Fmt::_10_3:  return writeDec(out, value, 3, ' ');
    case Fmt::_10_4:  return writeDec(out, value, 4, ' ');
    case Fmt::_10_5:  return writeDec(out, value, 5, ' ');
    case Fmt::_10_6:  return writeDec(out, value, 6, ' ');
    case Fmt::_10_8:  return writeDec(out, value, 8, ' ');

    case Fmt::_16_1:  return writeHex(out, value, 1);
    case Fmt::_16_2:  return writeHex(out, value, 2);
    case Fmt::_16_4:  return writeHex(out, value, 4);
    case Fmt::_16_8:  return writeHex(out, value, 8);

    case Fmt::_16_2_2:
      out = writeHex(out, (value >> 8) & 0xFF, 2);
      *out++ = '.';
      return writeHex(out, value & 0xFF, 2);

    case Fmt::_16_3_2:
      out = writeHex(out, value >> 8, 3);
      *out++ = '.';
      return writeHex(out, value & 0xFF, 2);

    case Fmt::_16:
    default:
      if(value < 0x100)        return writeHex(out, value, 2);
      else if(value < 0x10000) return writeHex(out, value, 4);
      else                     return writeHex(out, value, 8);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char* Base::writeHex(char* out, uInt32 v, int width)
{
  static constexpr char HEX_LOWER[] = "0123456789abcdef";
  static constexpr char HEX_UPPER[] = "0123456789ABCDEF";

  const char* table = myHexUppercase ? HEX_UPPER : HEX_LOWER;

  for(int i = (width - 1) * 4; i >= 0; i -= 4)
    *out++ = table[(v >> i) & 0xF];

  return out;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char* Base::writeDec(char* out, int value, int width, char padChar)
{
  char tmp[16];
  char* p = tmp + sizeof(tmp);  // NOLINT(misc-const-correctness)

  const bool neg = value < 0;
  // Safe unsigned negation: avoids UB when value == INT_MIN
  unsigned int v = neg ? (0U - static_cast<unsigned int>(value))
                       : static_cast<unsigned int>(value);
  do {
    *--p = static_cast<char>('0' + (v % 10));
    v /= 10;
  } while(v);

  if(neg) *--p = '-';

  const int len = static_cast<int>((tmp + sizeof(tmp)) - p);
  int pad = width - len;
  while(pad-- > 0)
    *out++ = padChar;
  while(p < tmp + sizeof(tmp))
    *out++ = *p++;

  return out;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char* Base::writeBinary(char* out, int value, Fmt fmt)
{
  const auto uval = static_cast<unsigned int>(value);
  const int bits = (fmt == Fmt::_2_16) ? 16 :
                   (fmt == Fmt::_2_8)  ? 8 :
                   (uval < 0x100U ? 8 : 16);

  for(int i = bits - 1; i >= 0; --i)
    *out++ = static_cast<char>('0' + ((uval >> i) & 1U));

  return out;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init)
std::ios_base::fmtflags Base::myHexFlags =
  std::ios_base::fmtflags{std::ios_base::hex};

}  // namespace Common

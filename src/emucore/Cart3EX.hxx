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

#ifndef CARTRIDGE_3EX_HXX
#define CARTRIDGE_3EX_HXX

class System;
class Settings;

#include "bspf.hxx"
#include "Cart3E.hxx"

/**
  3EX ("3E eXtended") is an enhanced version of the 3E scheme that increases
  the maximum RAM from 32K (32 banks x 1K) to 256K (256 banks x 1K).
  The bankswitching protocol is identical to 3E: write to $3E to map a RAM
  bank into the lower 2K segment, write to $3F to map a ROM bank.  As in 3E,
  the RAM bank is read at $1000-$13FF and written at $1400-$17FF, and the
  RAM banks are numbered after the ROM banks in the combined numbering space.

  The byte at $FFFA (the 6th last byte of the image) holds the number of RAM
  banks minus 1.  For autodetection, the image must contain the string "3EX"
  at least twice.

  @author  Thomas Jentzsch
*/

class Cartridge3EX : public Cartridge3E
{
  public:
    /**
      Create a new cartridge using the specified image and size

      @param image     Span of the ROM image
      @param md5       The md5sum of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    Cartridge3EX(ByteSpan image, string_view md5,
                 const Settings& settings);
    ~Cartridge3EX() override = default;

  public:
    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override { return "Cartridge3EX"; }

  private:
    // RAM size
    static constexpr size_t RAM_SIZE = U32(RAM_BANKS) << U32(BANK_SHIFT - 1); // = 256K = 0x40000;

  private:
    // Following constructors and assignment operators not supported
    Cartridge3EX() = delete;
    Cartridge3EX(const Cartridge3EX&) = delete;
    Cartridge3EX(Cartridge3EX&&) = delete;
    Cartridge3EX& operator=(const Cartridge3EX&) = delete;
    Cartridge3EX& operator=(Cartridge3EX&&) = delete;
};

#endif  // CARTRIDGE_3EX_HXX

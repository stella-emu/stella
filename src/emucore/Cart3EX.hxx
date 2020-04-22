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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef CARTRIDGE3EX_HXX
#define CARTRIDGE3EX_HXX

class System;

#include "Cart3E.hxx"
#ifdef DEBUGGER_SUPPORT
//#include "Cart3EXWidget.hxx"
#endif

/**
  This is an enhanced version of 3E which supports up to 256KB RAM.

  @author  Thomas Jentzsch
*/

class Cartridge3EX : public Cartridge3E
{
  //friend class Cartridge3EXWidget;

public:
  /**
    Create a new cartridge using the specified image and size

    @param image     Pointer to the ROM image
    @param size      The size of the ROM image
    @param md5       The md5sum of the ROM image
    @param settings  A reference to the various settings (read-only)
  */
  Cartridge3EX(const ByteBuffer& image, size_t size, const string& md5,
              const Settings& settings);
  virtual ~Cartridge3EX() = default;

public:
  /**
    Get a descriptor for the device name (used in error checking).

    @return The name of the object
  */
  string name() const override { return "Cartridge3EX"; }

#ifdef DEBUGGER_SUPPORT
  ///**
  //  Get debugger widget responsible for accessing the inner workings
  //  of the cart.
  //*/
  //CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
  //                             const GUI::Font& nfont, int x, int y, int w, int h) override
  //{
  //  return new Cartridge3EXWidget(boss, lfont, nfont, x, y, w, h, *this);
  //}
#endif

private:
  // RAM size
  static constexpr size_t RAM_SIZE = RAM_BANKS << (BANK_SHIFT - 1); // = 256K = 0x40000;

private:
  // Following constructors and assignment operators not supported
  Cartridge3EX() = delete;
  Cartridge3EX(const Cartridge3EX&) = delete;
  Cartridge3EX(Cartridge3EX&&) = delete;
  Cartridge3EX& operator=(const Cartridge3EX&) = delete;
  Cartridge3EX& operator=(Cartridge3EX&&) = delete;
};

#endif

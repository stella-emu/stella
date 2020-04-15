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

#ifndef CARTRIDGE_3EPLUS_HXX
#define CARTRIDGE_3EPLUS_HXX

class System;

#include "bspf.hxx"
#include "CartEnhanced.hxx"

#ifdef DEBUGGER_SUPPORT
class Cartridge3EPlusWidget;
  #include "Cart3EPlusWidget.hxx"
#endif

/**
  Cartridge class for new tiling engine "Boulder Dash" format games with RAM.
  Kind of a combination of 3F and 3E, with better switchability.
  B.Watson's Cart3E was used as a template for building this implementation.

  The destination bank (0-3) is held in the top bits of the value written to
  $3E (for RAM switching) or $3F (for ROM switching). The low 6 bits give
  the actual bank number (0-63) corresponding to 512 byte blocks for RAM and
  1024 byte blocks for ROM. The maximum size is therefore 32K RAM and 64K ROM.

  D7D6         indicate the bank number (0-3)
  D5D4D3D2D1D0 indicate the actual # (0-63) from the image/ram

  ROM:

  Note: in descriptions $F000 is equivalent to $1000 -- that is, we only deal
  with the low 13 bits of addressing. Stella code uses $1000, I'm used to $F000
  So, mask with top bits clear :) when reading this document.

  In this scheme, the 4K address space is broken into four 1K ROM/512b RAM segments
  living at 0x1000, 0x1400, 0x1800, 0x1C00 (or, same thing, 0xF000... etc.),

  The last 1K ROM ($FC00-$FFFF) segment in the 6502 address space (ie: $1C00-$1FFF)
  is initialised to point to the FIRST 1K of the ROM image, so the reset vectors
  must be placed at the end of the first 1K in the ROM image. Note, this is
  DIFFERENT to 3E which switches in the UPPER bank and this bank is fixed.  This
  allows variable sized ROM without having to detect size. First bank (0) in ROM is
  the default fixed bank mapped to $FC00.

  The system requires the reset vectors to be valid on a reset, so either the
  hardware first switches in the first bank, or the programmer must ensure
  that the reset vector is present in ALL ROM banks which might be switched
  into the last bank area.  Currently the latter (programmer onus) is required,
  but it would be nice for the cartridge hardware to auto-switch on reset.

  ROM switching (write of block+bank number to $3F) D7D6 upper 2 bits of bank #
  indicates the destination segment (0-3, corresponding to $F000, $F400, $F800,
  $FC00), and lower 6 bits indicate the 1K bank to switch in.  Can handle 64
  x 1K ROM banks (64K total).

  D7 D6 D5D4D3D2D1D0
  0  0   x x x x x x   switch a 1K ROM bank xxxxxx to $F000
  0  1                 switch a 1K ROM bank xxxxxx to $F400
  1  0                 switch a 1K ROM bank xxxxxx to $F800
  1  1                 switch a 1K ROM bank xxxxxx to $FC00

  RAM switching (write of segment+bank number to $3E) with D7D6 upper 2 bits of
  bank # indicates the destination RAM segment (0-3, corresponding to $F000,
  $F400, $F800, $FC00).

  Can handle 64 x 512 byte RAM banks (32K total)

  D7 D6 D5D4D3D2D1D0
  0  0   x x x x x x   switch a 512 byte RAM bank xxxxxx to $F000 with write @ $F200
  0  1                 switch a 512 byte RAM bank xxxxxx to $F400 with write @ $F600
  1  0                 switch a 512 byte RAM bank xxxxxx to $F800 with write @ $FA00
  1  1                 switch a 512 byte RAM bank xxxxxx to $FC00 with write @ $FE00

  @author  Thomas Jentzsch and Stephen Anthony
*/

class Cartridge3EPlus: public CartridgeEnhanced
{
  friend class Cartridge3EPlusWidget;

  public:
    /**
      Create a new cartridge using the specified image and size

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param md5       The md5sum of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    Cartridge3EPlus(const ByteBuffer& image, size_t size, const string& md5,
                    const Settings& settings);
    virtual ~Cartridge3EPlus() = default;

  public:
    /** Reset device to its power-on state */
    void reset() override;

    /**
      Install cartridge in the specified system.  Invoked by the system
      when the cartridge is attached to it.

      @param system The system the device should install itself in
    */
    void install(System& system) override;

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override { return "Cartridge3E+"; }

  #ifdef DEBUGGER_SUPPORT
    /**
      Get debugger widget responsible for accessing the inner workings
      of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont, int x, int y, int w, int h) override
    {
      return new Cartridge3EPlusWidget(boss, lfont, nfont, x, y, w, h, *this);
    }
  #endif

  public:
    /**
      Get the byte at the specified address

      @return The byte at the specified address
    */
    uInt8 peek(uInt16 address) override;

    /**
      Change the byte at the specified address to the given value

      @param address  The address where the value should be stored
      @param value    The value to be stored at the address
      @return         True if the poke changed the device address space, else false
    */
    bool poke(uInt16 address, uInt8 value) override;

  private:
    bool checkSwitchBank(uInt16 address, uInt8 value) override;

  private:
    // log(ROM bank segment size) / log(2)
    static constexpr uInt16 BANK_SHIFT = 10; // = 1K = 0x0400

    // The size of extra RAM in ROM address space
    static constexpr uInt16 RAM_BANKS = 64;

    // RAM size
    static constexpr uInt16 RAM_SIZE = RAM_BANKS << (BANK_SHIFT - 1); // = 32K = 0x4000;

    // Write port for extra RAM is at high address
    static constexpr bool RAM_HIGH_WP = true;

  private:
    // Following constructors and assignment operators not supported
    Cartridge3EPlus() = delete;
    Cartridge3EPlus(const Cartridge3EPlus&) = delete;
    Cartridge3EPlus(Cartridge3EPlus&&) = delete;
    Cartridge3EPlus& operator=(const Cartridge3EPlus&) = delete;
    Cartridge3EPlus& operator=(Cartridge3EPlus&&) = delete;
};

#endif

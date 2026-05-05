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

#ifndef CARTRIDGE_EFF_HXX
#define CARTRIDGE_EFF_HXX

class System;

#include "MT24LC16B.hxx"
#include "bspf.hxx"
#include "CartEF.hxx"
#ifdef DEBUGGER_SUPPORT
  #include "CartEFFWidget.hxx"
#endif

/**
   Based on EF, the EFF cartridge adds strobe addresses for i2c access to
   an EEPROM, like an on-cartridge SaveKey interface.

   @author Stephen Anthony, Thomas Jentzsch, Bruce-Robert Pocock
*/
class CartridgeEFF : public CartridgeEF
{
  friend class CartridgeEFFWidget;

  public:
    /**
       Create a new cartridge using the specified image

       @param image     Pointer to the ROM image
       @param size      The size of the ROM image
       @param md5       The md5sum of the ROM image
       @param settings  A reference to the various settings (read-only)
       @param bsSize    The size specified by the bankswitching scheme
    */
    CartridgeEFF(const ByteBuffer& image, size_t size, string_view md5,
                 const Settings& settings, size_t bsSize = 64_KB);
    ~CartridgeEFF() override = default;

    /**
       Get a descriptor for the device name (used in error checking).

       @return The name of the object
    */
    string name() const override { return "CartridgeEFF"; }

  #ifdef DEBUGGER_SUPPORT
    /**
       Get debugger widget responsible for accessing the inner workings
       of the cart.
    */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
                                 const GUI::Font& nfont, int x, int y, int w, int h) override
    {
      return new CartridgeEFFWidget(boss, lfont, nfont, x, y, w, h, *this);
    }
  #endif

    uInt8 peek(uInt16 address) override;
    bool poke(uInt16 address, uInt8 value) override;
    uInt16 hotspot() const override { return 0x1FE0; }
    void setNVRamFile(string_view path) override;

  private:
    bool checkSwitchBank (uInt16 address, uInt8) override;
    uInt16 getStartBank() const override { return 1; }

    void setI2CClock(bool value);
    void setI2CData(bool value);

    bool myI2CData{false};
    bool myI2CClock{false};

    string myEEPROMFile;
    unique_ptr<MT24LC16B> myEEPROM;

    uInt8 readI2C();

  private:
    // Following constructors and assignment operators not supported
    CartridgeEFF() = delete;
    CartridgeEFF(const CartridgeEFF&) = delete;
    CartridgeEFF(CartridgeEFF&&) = delete;
    CartridgeEFF& operator=(const CartridgeEFF&) = delete;
    CartridgeEFF& operator=(CartridgeEFF&&) = delete;
};

#endif  // CARTRIDGE_EFF_HXX

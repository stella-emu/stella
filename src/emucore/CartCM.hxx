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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef CARTRIDGECM_HXX
#define CARTRIDGECM_HXX

class System;

#include "bspf.hxx"
#include "Cart.hxx"

/**
  Cartridge class used for SpectraVideo CompuMate bankswitched games.
  There are 4 4K banks selectable at $1000 - $1FFFF.

  Bankswitching is done though the controller ports
    INPT0: D7 = CTRL key input (0 on startup / 1 = key pressed)
    INPT1: D7 = always HIGH input (tested at startup)
    INPT2: D7 = always HIGH input (tested at startup)
    INPT3: D7 = SHIFT key input (0 on startup / 1 = key pressed)
    INPT4: D7 = keyboard row 1 input (0 = key pressed)
    INPT5: D7 = keyboard row 3 input (0 = key pressed)
    SWCHA: D7 = tape recorder I/O ?
           D6 = 1 -> increase key collumn (0 to 9)
           D5 = 1 -> reset key collumn to 0 (if D4 = 0)
           D5 = 0 -> enable RAM writing (if D4 = 1)
           D4 = 1 -> map 2K of RAM at $1800 - $1fff
           D3 = keyboard row 4 input (0 = key pressed)
           D2 = keyboard row 2 input (0 = key pressed)
           D1 = bank select high bit
           D0 = bank select low bit

  Keyboard column numbering:
    column 0 = 7 U J M
    column 1 = 6 Y H N
    column 2 = 8 I K ,
    column 3 = 2 W S X
    column 4 = 3 E D C
    column 5 = 0 P ENTER SPACE
    column 6 = 9 O L .
    column 7 = 5 T G B
    column 8 = 1 Q A Z
    column 9 = 4 R F V

  @author  Stephen Anthony & z26 team
  @version $Id$
*/
class CartridgeCM : public Cartridge
{
  public:
    /**
      Create a new cartridge using the specified image

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param settings  A reference to the various settings (read-only)
    */
    CartridgeCM(const uInt8* image, uInt32 size, const Settings& settings);
 
    /**
      Destructor
    */
    virtual ~CartridgeCM();

  public:
    /**
      Reset device to its power-on state
    */
    void reset();

    /**
      Install cartridge in the specified system.  Invoked by the system
      when the cartridge is attached to it.

      @param system The system the device should install itself in
    */
    void install(System& system);

    /**
      Install pages for the specified bank in the system.

      @param bank The bank that should be installed in the system
    */
    bool bank(uInt16 bank);

    /**
      Get the current bank.
    */
    uInt16 bank() const;

    /**
      Query the number of banks supported by the cartridge.
    */
    uInt16 bankCount() const;

    /**
      Patch the cartridge ROM.

      @param address  The ROM address to patch
      @param value    The value to place into the address
      @return    Success or failure of the patch operation
    */
    bool patch(uInt16 address, uInt8 value);

    /**
      Access the internal ROM image for this cartridge.

      @param size  Set to the size of the internal ROM image data
      @return  A pointer to the internal ROM image data
    */
    const uInt8* getImage(int& size) const;

    /**
      Save the current state of this cart to the given Serializer.

      @param out  The Serializer object to use
      @return  False on any errors, else true
    */
    bool save(Serializer& out) const;

    /**
      Load the current state of this cart from the given Serializer.

      @param in  The Serializer object to use
      @return  False on any errors, else true
    */
    bool load(Serializer& in);

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const { return "CartridgeCM"; }

  public:
    /**
      Get the byte at the specified address.

      @return The byte at the specified address
    */
    uInt8 peek(uInt16 address);

    /**
      Change the byte at the specified address to the given value

      @param address The address where the value should be stored
      @param value The value to be stored at the address
      @return  True if the poke changed the device address space, else false
    */
    bool poke(uInt16 address, uInt8 value);

  private:
    // Indicates which bank is currently active
    uInt16 myCurrentBank;

    // The 16K ROM image of the cartridge
    uInt8 myImage[16384];
};

#endif

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

#ifndef CARTRIDGECHETIRY_HXX
#define CARTRIDGECHETIRY_HXX

class System;

#include "bspf.hxx"
#include "Cart.hxx"

/**
  TODO - add description from cd-w.

  @author  Stephen Anthony and Chris D. Walton
  @version $Id$
*/
class CartridgeCTY : public Cartridge
{
  public:
    /**
      Create a new cartridge using the specified image

      @param image     Pointer to the ROM image
      @param size      The size of the ROM image
      @param osystem   A reference to the OSystem currently in use
    */
    CartridgeCTY(const uInt8* image, uInt32 size, const OSystem& osystem);
 
    /**
      Destructor
    */
    virtual ~CartridgeCTY();

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
    string name() const { return "CartridgeCTY"; }

    /**
      Informs the cartridge about the name of the ROM file used when
      creating this cart.

      @param name  The properties file name of the ROM
    */
    void setRomName(const string& name);

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
    /**
      Either load or save internal RAM to Harmony EEPROM (represented by
      a file in emulation).

      @return  The value at $FF4 with bit 6 set or cleared (depending on
               whether the RAM access was busy or successful)
    */
    uInt8 ramReadWrite();

    /**
      Actions initiated by accessing $FF4 hotspot.
    */
    void loadTune(uInt8 index);
    void loadScore(uInt8 index);
    void saveScore(uInt8 index);
    void wipeAllScores();

  private:
    // OSsytem currently in use
    const OSystem& myOSystem;

    // Indicates which bank is currently active
    uInt16 myCurrentBank;

    // The 32K ROM image of the cartridge
    uInt8 myImage[32768];

    // The 64 bytes of RAM accessible at $1000 - $1080
    uInt8 myRAM[64];

    // Operation type (written to $1000, used by hotspot $1FF4)
    uInt8 myOperationType;

    // The 8K Harmony RAM (used for tune data)
    // Data is accessed from Harmony EEPROM
    uInt8 myTuneRAM[8192];

    // The time after which the first request of a load/save operation
    // will actually be completed
    // Due to flash RAM constraints, a read/write isn't instantaneous,
    // so we need to emulate the delay as well
    uInt64 myRamAccessTimeout;

    // Full pathname of the file to use when emulating load/save
    // of internal RAM to Harmony cart flash
    string myEEPROMFile;
};

#endif

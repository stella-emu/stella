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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef CARTRIDGEDCP_HXX
#define CARTRIDGEDCP_HXX

class System;

#include "bspf.hxx"
#include "Cart.hxx"

/**
  Cartridge class used for Pitfall II.  There are two 4K program banks, a 
  2K display bank, and the DPC chip.  For complete details on the DPC chip 
  see David P. Crane's United States Patent Number 4,644,495.

  @author  Bradford W. Mott
  @version $Id$
*/
class CartridgeDPC : public Cartridge
{
  public:
    /**
      Create a new cartridge using the specified image

      @param image Pointer to the ROM image
    */
    CartridgeDPC(const uInt8* image, uInt32 size);
 
    /**
      Destructor
    */
    virtual ~CartridgeDPC();

  public:
    /**
      Reset device to its power-on state
    */
    virtual void reset();

    /**
      Notification method invoked by the system right before the
      system resets its cycle counter to zero.  It may be necessary
      to override this method for devices that remember cycle counts.
    */
    virtual void systemCyclesReset();

    /**
      Install cartridge in the specified system.  Invoked by the system
      when the cartridge is attached to it.

      @param system The system the device should install itself in
    */
    virtual void install(System& system);

    /**
      Install pages for the specified bank in the system.

      @param bank The bank that should be installed in the system
    */
    virtual void bank(uInt16 bank);

    /**
      Get the current bank.

      @return  The current bank, or -1 if bankswitching not supported
    */
    virtual int bank();

    /**
      Query the number of banks supported by the cartridge.
    */
    virtual int bankCount();

    /**
      Patch the cartridge ROM.

      @param address  The ROM address to patch
      @param value    The value to place into the address
      @return    Success or failure of the patch operation
    */
    virtual bool patch(uInt16 address, uInt8 value);

    /**
      Access the internal ROM image for this cartridge.

      @param size  Set to the size of the internal ROM image data
      @return  A pointer to the internal ROM image data
    */
    virtual uInt8* getImage(int& size);

    /**
      Save the current state of this cart to the given Serializer.

      @param out  The Serializer object to use
      @return  False on any errors, else true
    */
    virtual bool save(Serializer& out) const;

    /**
      Load the current state of this cart from the given Deserializer.

      @param in  The Deserializer object to use
      @return  False on any errors, else true
    */
    virtual bool load(Deserializer& in);

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    virtual string name() const { return "CartridgeDPC"; }

  public:
    /**
      Get the byte at the specified address.

      @return The byte at the specified address
    */
    virtual uInt8 peek(uInt16 address);

    /**
      Change the byte at the specified address to the given value

      @param address The address where the value should be stored
      @param value The value to be stored at the address
    */
    virtual void poke(uInt16 address, uInt8 value);

  private:
    /** 
      Clocks the random number generator to move it to its next state
    */
    void clockRandomNumberGenerator();

    /** 
      Updates any data fetchers in music mode based on the number of
      CPU cycles which have passed since the last update.
    */
    void updateMusicModeDataFetchers();

  private:
    // Indicates which bank is currently active
    uInt16 myCurrentBank;

    // The 8K program ROM image of the cartridge
    uInt8 myProgramImage[8192];

    // The 2K display ROM image of the cartridge
    uInt8 myDisplayImage[2048];

    // Copy of the raw image, for use by getImage()
    uInt8 myImageCopy[8192 + 2048 + 255];

    // The top registers for the data fetchers
    uInt8 myTops[8];

    // The bottom registers for the data fetchers
    uInt8 myBottoms[8];

    // The counter registers for the data fetchers
    uInt16 myCounters[8];

    // The flag registers for the data fetchers
    uInt8 myFlags[8];

    // The music mode DF5, DF6, & DF7 enabled flags
    bool myMusicMode[3];

    // The random number generator register
    uInt8 myRandomNumber;

    // System cycle count when the last update to music data fetchers occurred
    Int32 mySystemCycles;

    // Fractional DPC music OSC clocks unused during the last update
    double myFractionalClocks;
};

#endif

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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Cart4A50.hxx,v 1.7 2007-10-09 23:56:57 stephena Exp $
//============================================================================

#ifndef CARTRIDGE4A50_HXX
#define CARTRIDGE4A50_HXX

class System;

#include "bspf.hxx"
#include "Cart.hxx"

/**
  Bankswitching method as defined/created by John Payson (aka Supercat),
  documented at http://www.casperkitty.com/stella/cartfmt.htm.

  In this bankswitching scheme the 2600's 4K cartridge address space 
  is broken into four segments.  The first 2K segment accesses any 2K
  region of RAM, or of the first 32K of ROM.  The second 1.5K segment
  accesses the first 1.5K of any 2K region of RAM, or of the last 32K
  of ROM.  The 3rd 256 byte segment points to any 256 byte page of
  RAM or ROM.  The last 256 byte segment always points to the last 256
  bytes of ROM.

  @author  Stephen Anthony
  @version $Id: Cart4A50.hxx,v 1.7 2007-10-09 23:56:57 stephena Exp $
*/
class Cartridge4A50 : public Cartridge
{
  public:
    /**
      Create a new cartridge using the specified image

      @param image Pointer to the ROM image
    */
    Cartridge4A50(const uInt8* image);
 
    /**
      Destructor
    */
    virtual ~Cartridge4A50();

  public:
    /**
      Reset cartridge to its power-on state
    */
    virtual void reset();

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
    virtual string name() const { return "Cartridge4A50"; }

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
      Install the specified slice for segment zero

      @param slice The slice to map into the segment
    */
    void segmentZero(uInt16 slice);

    /**
      Install the specified slice for segment one

      @param slice The slice to map into the segment
    */
    void segmentOne(uInt16 slice);

    /**
      Install the specified slice for segment two

      @param slice The slice to map into the segment
    */
    void segmentTwo(uInt16 slice);

  private:
    // Indicates the slice mapped into each of the four segments
    uInt16 myCurrentSlice[4];

    // The 64K ROM image of the cartridge
    uInt8 myImage[65536];

    // The 32K of RAM on the cartridge
    uInt8 myRAM[32768];
};

#endif

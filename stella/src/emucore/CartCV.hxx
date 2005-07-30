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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CartCV.hxx,v 1.6 2005-07-30 16:58:22 urchlay Exp $
//============================================================================

#ifndef CARTRIDGECV_HXX
#define CARTRIDGECV_HXX

class CartridgeCV;
class System;
class Serializer;
class Deserializer;

#include "bspf.hxx"
#include "Cart.hxx"

/**
  Cartridge class used for Commavid's extra-RAM games.

  $F000-$F3FF read from RAM
  $F400-$F7FF write to RAM
  $F800-$FFFF ROM

  @author  Eckhard Stolberg
  @version $Id: CartCV.hxx,v 1.6 2005-07-30 16:58:22 urchlay Exp $
*/
class CartridgeCV : public Cartridge
{
  public:
    /**
      Create a new cartridge using the specified image

      @param image Pointer to the ROM image
    */
    CartridgeCV(const uInt8* image, uInt32 size);

    /**
      Destructor
    */
    virtual ~CartridgeCV();

  public:
    /**
      Get a null terminated string which is the device's name (i.e. "M6532")

      @return The name of the device
    */
    virtual const char* name() const;

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
      Saves the current state of this device to the given Serializer.

      @param out The serializer device to save to.
      @return The result of the save.  True on success, false on failure.
    */
    virtual bool save(Serializer& out);

    /**
      Loads the current state of this device from the given Deserializer.

      @param in The deserializer device to load from.
      @return The result of the load.  True on success, false on failure.
    */
    virtual bool load(Deserializer& in);

    virtual uInt8* getImage(int& size);

  public:
    /**
      Get the byte at the specified address

      @return The byte at the specified address
    */
    virtual uInt8 peek(uInt16 address);

    /**
      Change the byte at the specified address to the given value

      @param address The address where the value should be stored
      @param value The value to be stored at the address
    */
    virtual void poke(uInt16 address, uInt8 value);

	 bool patch(uInt16 address, uInt8 value);

  private:
    // The 2k ROM image for the cartridge
    uInt8 myImage[2048];

    // The 1024 bytes of RAM
    uInt8 myRAM[1024];
};
#endif


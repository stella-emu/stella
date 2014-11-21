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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <cstring>

#include "System.hxx"
#include "CartPPA.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgePPA::CartridgePPA(const uInt8* image, uInt32 size,
                           const Settings& settings)
  : Cartridge(settings)
{
  // Copy the ROM image into my buffer
  memcpy(myImage, image, BSPF_min(8192u, size));
  createCodeAccessBase(8192);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgePPA::~CartridgePPA()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgePPA::reset()
{
  // Setup segments to some default slices
  bank(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgePPA::install(System& system)
{
  mySystem = &system;
  bank(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgePPA::peek(uInt16 address)
{
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgePPA::poke(uInt16 address, uInt8)
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgePPA::bank(uInt16 bank)
{
  if(bankLocked() || bank > 15) return false;

  segmentZero(ourBankOrg[bank].zero);
  segmentOne(ourBankOrg[bank].one);
  segmentTwo(ourBankOrg[bank].two);
  segmentThree(ourBankOrg[bank].three, ourBankOrg[bank].map3bytes);

  return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgePPA::segmentZero(uInt8 slice)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgePPA::segmentOne(uInt8 slice)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgePPA::segmentTwo(uInt8 slice)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgePPA::segmentThree(uInt8 slice, bool map3bytes)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgePPA::patch(uInt16 address, uInt8 value)
{
  return false;
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgePPA::getImage(int& size) const
{
  size = 8192;
  return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgePPA::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putByteArray(myCurrentSlice, 4);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgePPA::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgePPA::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    in.getByteArray(myCurrentSlice, 4);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgePPA::load" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgePPA::BankOrg CartridgePPA::ourBankOrg[16] = {
  { 0, 0, 1, 2, false },
  { 0, 1, 3, 2, false },
  { 4, 5, 6, 7, false },
  { 7, 4, 3, 2, false },
  { 0, 0, 6, 7, false },
  { 0, 1, 7, 6, false },
  { 3, 2, 4, 5, false },
  { 6, 0, 5, 1, false },
  { 0, 0, 1, 2, false },
  { 0, 1, 3, 2, false },
  { 4, 5, 6, 8, false },
  { 7, 4, 3, 2, false },
  { 0, 0, 6, 7, true  },
  { 0, 1, 7, 6, true  },
  { 3, 2, 4, 5, true  },
  { 6, 0, 5, 1, true  }
};

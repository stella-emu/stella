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
// Copyright (c) 1995-2004 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SoundNull.cxx,v 1.1 2005-04-28 19:30:26 stephena Exp $
//============================================================================

#include "Serializer.hxx"
#include "Deserializer.hxx"

#include "bspf.hxx"

#include "OSystem.hxx"
#include "Settings.hxx"
#include "SoundNull.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundNull::SoundNull(OSystem* osystem)
    : Sound(osystem)
{
  // Add the sound object to the system
  myOSystem->attach(this);

  // Show some info
  if(myOSystem->settings().getBool("showinfo"))
    cout << "Sound support not available." << endl << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundNull::~SoundNull()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundNull::load(Deserializer& in)
{
  string soundDevice = "TIASound";
  if(in.getString() != soundDevice)
    return false;

  uInt8 reg;
  reg = (uInt8) in.getLong();
  reg = (uInt8) in.getLong();
  reg = (uInt8) in.getLong();
  reg = (uInt8) in.getLong();
  reg = (uInt8) in.getLong();
  reg = (uInt8) in.getLong();

  // myLastRegisterSetCycle
  in.getLong();

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundNull::save(Serializer& out)
{
  out.putString("TIASound");

  uInt8 reg = 0;
  out.putLong(reg);
  out.putLong(reg);
  out.putLong(reg);
  out.putLong(reg);
  out.putLong(reg);
  out.putLong(reg);

  // myLastRegisterSetCycle
  out.putLong(0);

  return true;
}

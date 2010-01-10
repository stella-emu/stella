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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "bspf.hxx"

#include "OSystem.hxx"
#include "Settings.hxx"
#include "SoundNull.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundNull::SoundNull(OSystem* osystem)
  : Sound(osystem)
{
  // Show some info
  if(myOSystem->settings().getBool("showinfo"))
    cout << "Sound disabled." << endl << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundNull::~SoundNull()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundNull::load(Serializer& in)
{
  const string& soundDevice = "TIASound";
  if(in.getString() != soundDevice)
    return false;

  uInt8 reg;
  reg = (uInt8) in.getByte();
  reg = (uInt8) in.getByte();
  reg = (uInt8) in.getByte();
  reg = (uInt8) in.getByte();
  reg = (uInt8) in.getByte();
  reg = (uInt8) in.getByte();

  // myLastRegisterSetCycle
  in.getInt();

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundNull::save(Serializer& out)
{
  out.putString("TIASound");

  uInt8 reg = 0;
  out.putByte((char)reg);
  out.putByte((char)reg);
  out.putByte((char)reg);
  out.putByte((char)reg);
  out.putByte((char)reg);
  out.putByte((char)reg);

  // myLastRegisterSetCycle
  out.putInt(0);

  return true;
}

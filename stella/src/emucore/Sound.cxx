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
// $Id: Sound.cxx,v 1.16 2005-02-22 02:59:54 stephena Exp $
//============================================================================

#include "Serializer.hxx"
#include "Deserializer.hxx"

#include "OSystem.hxx"
#include "Sound.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Sound::Sound(OSystem* osystem)
    : myOSystem(osystem),
      myIsInitializedFlag(false),
      myLastRegisterSetCycle(0)
{
  // Add the sound object to the system
  myOSystem->attach(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Sound::~Sound()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::adjustCycleCounter(Int32 amount)
{
  myLastRegisterSetCycle += amount;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::mute(bool state)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::initialize(double displayframerate)
{
  myLastRegisterSetCycle = 0;
  myDisplayFrameRate = displayframerate;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Sound::isSuccessfullyInitialized() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::reset()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::set(uInt16 addr, uInt8 value, Int32 cycle)
{
  myLastRegisterSetCycle = cycle;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::setVolume(Int32 volume)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Sound::adjustVolume(Int8 direction)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Sound::load(Deserializer& in)
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

  myLastRegisterSetCycle = (Int32)in.getLong();

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Sound::save(Serializer& out)
{
  out.putString("TIASound");

  uInt8 reg = 0;
  out.putLong(reg);
  out.putLong(reg);
  out.putLong(reg);
  out.putLong(reg);
  out.putLong(reg);
  out.putLong(reg);

  out.putLong(myLastRegisterSetCycle);

  return true;
}


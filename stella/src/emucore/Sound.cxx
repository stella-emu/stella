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
// $Id: Sound.cxx,v 1.12 2004-06-13 04:54:25 bwmott Exp $
//============================================================================

#include "Serializer.hxx"
#include "Deserializer.hxx"

#include "Sound.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Sound::Sound(uInt32 fragsize)
    : myLastRegisterSetCycle(0)
{
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
void Sound::init(Console* console, MediaSource* mediasrc, System* system)
{
  myConsole = console;
  myMediaSource = mediasrc;
  mySystem = system;
  myLastRegisterSetCycle = 0;
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


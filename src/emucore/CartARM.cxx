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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "System.hxx"
#include "Settings.hxx"
#include "CartARM.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeARM::CartridgeARM(const string& md5, const Settings& settings)
  : Cartridge(settings, md5)
{
  myIncCycles = settings.getBool("dev.settings")
    && settings.getBool("dev.thumb.inccycles");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARM::updateCycles(int cycles)
{
  if(myIncCycles)
    mySystem->incrementCycles(cycles); // * ~1.11 is the limit for ZEVIOUZ title screen (~142,000 cycles)
  myStats = myThumbEmulator->stats();
  myPrevStats = myThumbEmulator->prevStats();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARM::incCycles(bool enable)
{
  myIncCycles = enable;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeARM::cycleFactor(double factor)
{
  myThumbEmulator->cycleFactor(factor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeARM::save(Serializer& out) const
{
  try
  {
    out.putInt(myPrevStats.cycles);
    out.putInt(myPrevStats.fetches);
    out.putInt(myPrevStats.reads);
    out.putInt(myPrevStats.writes);
    out.putInt(myStats.cycles);
    out.putInt(myStats.fetches);
    out.putInt(myStats.reads);
    out.putInt(myStats.writes);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeARM::save" << endl;
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeARM::load(Serializer& in)
{
  try
  {
    myPrevStats.cycles = in.getInt();
    myPrevStats.fetches = in.getInt();
    myPrevStats.reads = in.getInt();
    myPrevStats.writes = in.getInt();
    myStats.cycles = in.getInt();
    myStats.fetches = in.getInt();
    myStats.reads = in.getInt();
    myStats.writes = in.getInt();
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeARM::load" << endl;
    return false;
  }
  return true;
}

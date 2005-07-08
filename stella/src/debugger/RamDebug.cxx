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
// $Id: RamDebug.cxx,v 1.4 2005-07-08 17:22:40 stephena Exp $
//============================================================================

#include "Array.hxx"
#include "System.hxx"
#include "RamDebug.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RamDebug::RamDebug(Debugger* dbg, Console* console)
  : DebuggerSystem(dbg, console),
    mySystem(&(console->system()))
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerState& RamDebug::getState()
{
  myState.ram.clear();
  for(int i=0; i<0x80; i++)
    myState.ram.push_back(read(i));

  return myState;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamDebug::saveOldState()
{
  myOldState.ram.clear();
  for(int i=0; i<0x80; i++)
    myOldState.ram.push_back(read(i));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int RamDebug::read(int offset)
{
  offset &= 0x7f; // there are only 128 bytes
  return mySystem->peek(offset + 0x80);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamDebug::write(int offset, int value)
{
  offset &= 0x7f; // there are only 128 bytes
  mySystem->poke(offset + 0x80, value);
}

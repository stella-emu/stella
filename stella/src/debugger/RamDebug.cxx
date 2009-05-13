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

#include "Array.hxx"
#include "System.hxx"
#include "RamDebug.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RamDebug::RamDebug(Debugger& dbg, Console& console)
  : DebuggerSystem(dbg, console)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const DebuggerState& RamDebug::getState()
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
  return mySystem.peek(offset + 0x80);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamDebug::write(int offset, int value)
{
  offset &= 0x7f; // there are only 128 bytes
  mySystem.poke(offset + 0x80, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RamDebug::toString()
{
  string result;
  char buf[128];
  int bytesPerLine;
  int start = kRamStart, len = kRamSize;

  switch(myDebugger.parser().base())
  {
    case kBASE_16:
    case kBASE_10:
      bytesPerLine = 0x10;
      break;

    case kBASE_2:
      bytesPerLine = 0x04;
      break;

    case kBASE_DEFAULT:
    default:
      return DebuggerParser::red("invalid base, this is a BUG");
  }

  const RamState& state    = (RamState&) getState();
  const RamState& oldstate = (RamState&) getOldState();
  for (uInt8 i = 0x00; i < len; i += bytesPerLine)
  {
    sprintf(buf, "%.2x: ", start+i);
    result += buf;

    for (uInt8 j = 0; j < bytesPerLine; j++)
    {
      result += myDebugger.invIfChanged(state.ram[i+j], oldstate.ram[i+j]);
      result += " ";

      if(j == 0x07) result += " ";
    }
    result += "\n";
  }

  return result;
}

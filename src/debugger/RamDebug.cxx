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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "bspf.hxx"
#include "Array.hxx"
#include "System.hxx"
#include "RamDebug.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RamDebug::RamDebug(Debugger& dbg, Console& console)
  : DebuggerSystem(dbg, console)
{
  // Zero-page RAM is always present
  addRamArea(0x80, 128, 0, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamDebug::addRamArea(uInt16 start, uInt16 size,
                          uInt16 roffset, uInt16 woffset)
{
  // First make sure this area isn't already present
  for(uInt32 i = 0; i < myState.rport.size(); ++i)
    if(myState.rport[i] == start + roffset ||
       myState.wport[i] == start + woffset)
      return;

  // Otherwise, add a new area
  for(uInt32 i = 0; i < size; ++i)
  {
    myState.rport.push_back(i + start + roffset);
    myState.wport.push_back(i + start + woffset);

    myOldState.rport.push_back(i + start + roffset);
    myOldState.wport.push_back(i + start + woffset);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamDebug::addRamArea(const RamAreaList& areas)
{
  myRamAreas = areas;
  for(RamAreaList::const_iterator i = areas.begin(); i != areas.end(); ++i)
    addRamArea(i->start, i->size, i->roffset, i->woffset);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const DebuggerState& RamDebug::getState()
{
  myState.ram.clear();
  for(uInt32 i = 0; i < myState.rport.size(); ++i)
    myState.ram.push_back(read(myState.rport[i]));

  return myState;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamDebug::saveOldState()
{
  myOldState.ram.clear();
  for(uInt32 i = 0; i < myOldState.rport.size(); ++i)
    myOldState.ram.push_back(read(myOldState.rport[i]));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 RamDebug::read(uInt16 addr)
{
  return mySystem.peek(addr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamDebug::write(uInt16 addr, uInt8 value)
{
  mySystem.poke(addr, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int RamDebug::readFromWritePort()
{
  uInt16 addr = mySystem.m6502().lastReadAddress();
  if(addr & 0x1000)
  {
    addr &= 0x0FFF;
    for(RamAreaList::const_iterator i = myRamAreas.begin(); i != myRamAreas.end(); ++i)
    {
      uInt16 start = (i->start + i->woffset) & 0x0FFF;
      uInt16 end = (i->start + i->woffset + i->size) & 0x0FFF;
      if(addr >= start && addr < end)
        return addr;
    }
  }
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RamDebug::toString()
{
  string result;
  char buf[128];
  uInt32 bytesPerLine;

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

  uInt32 curraddr = 0, bytesSoFar = 0;
  for(uInt32 i = 0; i < state.ram.size(); i += bytesPerLine, bytesSoFar += bytesPerLine)
  {
    // We detect different 'pages' of RAM when the addresses jump by
    // more than the number of bytes on the previous line, or when 256
    // bytes have been previously output
    if(state.rport[i] - curraddr > bytesPerLine || bytesSoFar >= 256)
    {
      sprintf(buf, "%04x: (rport = %04x, wport = %04x)\n",
              state.rport[i], state.rport[i], state.wport[i]);
      buf[2] = buf[3] = 'x';
      result += DebuggerParser::red(buf);
      bytesSoFar = 0;
    }
    curraddr = state.rport[i];
    sprintf(buf, "%.2x: ", curraddr & 0x00ff);
    result += buf;

    for(uInt8 j = 0; j < bytesPerLine; ++j)
    {
      result += myDebugger.invIfChanged(state.ram[i+j], oldstate.ram[i+j]);
      result += " ";

      if(j == 0x07) result += " ";
    }
    result += "\n";
  }

  return result;
}

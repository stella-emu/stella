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
// $Id: CpuDebug.cxx,v 1.1 2005-07-07 18:56:40 stephena Exp $
//============================================================================

#include "Array.hxx"
#include "M6502.hxx"
#include "CpuDebug.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CpuDebug::CpuDebug(Console* console)
  : DebuggerSystem(console),
    mySystem(&(console->system()))
{
  saveOldState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DebuggerState& CpuDebug::getState()
{
  myState.PC = mySystem->m6502().PC;
  myState.SP = mySystem->m6502().SP;
  myState.PS = mySystem->m6502().PS();
  myState.A  = mySystem->m6502().A;
  myState.X  = mySystem->m6502().X;
  myState.Y  = mySystem->m6502().Y;

  myState.PSbits.clear();
  for(int i = 0; i < 8; ++i)
  {
    if(myState.PS & (1<<(7-i)))
      myState.PSbits.push_back(true);
    else
      myState.PSbits.push_back(false);
  }

  return myState;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::saveOldState()
{
  myOldState.PC = mySystem->m6502().PC;
  myOldState.SP = mySystem->m6502().SP;
  myOldState.PS = mySystem->m6502().PS();
  myOldState.A  = mySystem->m6502().A;
  myOldState.X  = mySystem->m6502().X;
  myOldState.Y  = mySystem->m6502().Y;

  myOldState.PSbits.clear();
  for(int i = 0; i < 8; ++i)
  {
    if(myOldState.PS & (1<<(7-i)))
      myOldState.PSbits.push_back(true);
    else
      myOldState.PSbits.push_back(false);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setPC(int pc)
{
  mySystem->m6502().PC = pc;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setSP(int sp)
{
  mySystem->m6502().SP = sp;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setA(int a)
{
  mySystem->m6502().A = a;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setX(int x)
{
  mySystem->m6502().X = x;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setY(int y)
{
  mySystem->m6502().Y = y;
}

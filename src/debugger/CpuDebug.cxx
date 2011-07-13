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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <sstream>

#include "Array.hxx"
#include "M6502.hxx"
#include "Debugger.hxx"
#include "CartDebug.hxx"
#include "TIADebug.hxx"

#include "CpuDebug.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CpuDebug::CpuDebug(Debugger& dbg, Console& console)
  : DebuggerSystem(dbg, console)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const DebuggerState& CpuDebug::getState()
{
  myState.PC = mySystem.m6502().PC;
  myState.SP = mySystem.m6502().SP;
  myState.PS = mySystem.m6502().PS();
  myState.A  = mySystem.m6502().A;
  myState.X  = mySystem.m6502().X;
  myState.Y  = mySystem.m6502().Y;

  myState.srcS = mySystem.m6502().lastSrcAddressS();
  myState.srcA = mySystem.m6502().lastSrcAddressA();
  myState.srcX = mySystem.m6502().lastSrcAddressX();
  myState.srcY = mySystem.m6502().lastSrcAddressY();

  Debugger::set_bits(myState.PS, myState.PSbits);

  return myState;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::saveOldState()
{
  myOldState.PC = mySystem.m6502().PC;
  myOldState.SP = mySystem.m6502().SP;
  myOldState.PS = mySystem.m6502().PS();
  myOldState.A  = mySystem.m6502().A;
  myOldState.X  = mySystem.m6502().X;
  myOldState.Y  = mySystem.m6502().Y;

  myOldState.srcS = mySystem.m6502().lastSrcAddressS();
  myOldState.srcA = mySystem.m6502().lastSrcAddressA();
  myOldState.srcX = mySystem.m6502().lastSrcAddressX();
  myOldState.srcY = mySystem.m6502().lastSrcAddressY();

  Debugger::set_bits(myOldState.PS, myOldState.PSbits);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setPC(int pc)
{
  mySystem.m6502().PC = pc;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setSP(int sp)
{
  mySystem.m6502().SP = sp;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setPS(int ps)
{
  mySystem.m6502().PS(ps);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setA(int a)
{
  mySystem.m6502().A = a;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setX(int x)
{
  mySystem.m6502().X = x;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setY(int y)
{
  mySystem.m6502().Y = y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setN(bool on)
{
  setPS( Debugger::set_bit(mySystem.m6502().PS(), 7, on) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setV(bool on)
{
  setPS( Debugger::set_bit(mySystem.m6502().PS(), 6, on) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setB(bool on)
{
  setPS( Debugger::set_bit(mySystem.m6502().PS(), 4, on) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setD(bool on)
{
  setPS( Debugger::set_bit(mySystem.m6502().PS(), 3, on) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setI(bool on)
{
  setPS( Debugger::set_bit(mySystem.m6502().PS(), 2, on) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setZ(bool on)
{
  setPS( Debugger::set_bit(mySystem.m6502().PS(), 1, on) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setC(bool on)
{
  setPS( Debugger::set_bit(mySystem.m6502().PS(), 0, on) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::toggleN()
{
  setPS( mySystem.m6502().PS() ^ 0x80 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::toggleV()
{
  setPS( mySystem.m6502().PS() ^ 0x40 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::toggleB()
{
  setPS( mySystem.m6502().PS() ^ 0x10 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::toggleD()
{
  setPS( mySystem.m6502().PS() ^ 0x08 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::toggleI()
{
  setPS( mySystem.m6502().PS() ^ 0x04 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::toggleZ()
{
  setPS( mySystem.m6502().PS() ^ 0x02 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::toggleC()
{
  setPS( mySystem.m6502().PS() ^ 0x01 );
}

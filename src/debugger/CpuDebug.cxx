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

#include <sstream>

#include "Array.hxx"
#include "M6502.hxx"
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

  Debugger::set_bits(myOldState.PS, myOldState.PSbits);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CpuDebug::toString()
{
  // TODO - this doesn't seem to be used anywhere ??
  string result;
  char buf[255];

  const CpuState& state = (CpuState&) getState();
  const CpuState& oldstate = (CpuState&) getOldState();

  result += "\nPC=";
  result += myDebugger.invIfChanged(state.PC, oldstate.PC);
  result += " A=";
  result += myDebugger.invIfChanged(state.A, oldstate.A);
  result += " X=";
  result += myDebugger.invIfChanged(state.X, oldstate.X);
  result += " Y=";
  result += myDebugger.invIfChanged(state.Y, oldstate.Y);
  result += " S=";
  result += myDebugger.invIfChanged(state.SP, oldstate.SP);
  result += " P=";
  result += myDebugger.invIfChanged(state.PS, oldstate.PS);
  result += "/";

  // NV-BDIZC
  buf[0] = n() ? 'N' : 'n';
  buf[1] = v() ? 'V' : 'v';
  buf[2] = '-';
  buf[3] = b() ? 'B' : 'b';
  buf[4] = d() ? 'D' : 'd';
  buf[5] = i() ? 'I' : 'i';
  buf[6] = z() ? 'Z' : 'z';
  buf[7] = c() ? 'C' : 'c';
  buf[8] = '\0';

  result += buf;
  result += "\n  FrameCyc:";
  sprintf(buf, "%d", mySystem.cycles());
  result += buf;
  result += " Frame:";
  sprintf(buf, "%d", myDebugger.tiaDebug().frameCount());
  result += buf;
  result += " ScanLine:";
  sprintf(buf, "%d", myDebugger.tiaDebug().scanlines());
  result += buf;
  result += " Clk/Pix/Cyc:";
  int clk = myDebugger.tiaDebug().clocksThisLine();
  sprintf(buf, "%d/%d/%d", clk, clk-68, clk/3);
  result += buf;
  result += " 6502Ins:";
  sprintf(buf, "%d", mySystem.m6502().totalInstructionCount());
  result += buf;
  result += "\n  ";

  result += myDebugger.cartDebug().disassemble(state.PC, 1);
  return result;
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

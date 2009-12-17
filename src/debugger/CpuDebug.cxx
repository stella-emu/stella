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

#include <sstream>

#include "Array.hxx"
#include "EquateList.hxx"
#include "M6502.hxx"
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

  result += myDebugger.disassemble(state.PC, 1);
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CpuDebug::disassemble(int address, string& result, EquateList& list)
{
  ostringstream buf;
  int count = 0;
  int opcode = mySystem.peek(address);

  // Are we looking at a read or write operation?
  // It will determine what type of label to use
  bool isRead = (M6502::AccessModeTable[opcode] == M6502::Read);

  switch(M6502::AddressModeTable[opcode])
  {
    case M6502::Absolute:
      buf << M6502::InstructionMnemonicTable[opcode] << " "
          << list.getLabel(dpeek(mySystem, address + 1), isRead, 4) << " ; "
          << M6502::InstructionCycleTable[opcode];
      count = 3;
      break;

    case M6502::AbsoluteX:
      buf << M6502::InstructionMnemonicTable[opcode] << " "
          << list.getLabel(dpeek(mySystem, address + 1), isRead, 4) << ",x ; "
          << M6502::InstructionCycleTable[opcode];
      count = 3;
      break;

    case M6502::AbsoluteY:
      buf << M6502::InstructionMnemonicTable[opcode] << " "
          << list.getLabel(dpeek(mySystem, address + 1), isRead, 4) << ",y ; "
          << M6502::InstructionCycleTable[opcode];
      count = 3;
      break;

    case M6502::Immediate:
      buf << M6502::InstructionMnemonicTable[opcode] << " #$"
          << hex << setw(2) << setfill('0') << (int) mySystem.peek(address + 1) << " ; "
          << dec << M6502::InstructionCycleTable[opcode];
      count = 2;
      break;

    case M6502::Implied:
      buf << M6502::InstructionMnemonicTable[opcode] << " ; "
          << M6502::InstructionCycleTable[opcode];
      count = 1;
      break;

    case M6502::Indirect:
      buf << M6502::InstructionMnemonicTable[opcode] << " ("
          << list.getLabel(dpeek(mySystem, address + 1), isRead, 4) << ") ; "
          << M6502::InstructionCycleTable[opcode];
      count = 3;
      break;

    case M6502::IndirectX:
      buf << M6502::InstructionMnemonicTable[opcode] << " ("
          << list.getLabel(mySystem.peek(address + 1), isRead, 2) << ",x) ; "
          << M6502::InstructionCycleTable[opcode];
      count = 2;
      break;

    case M6502::IndirectY:
      buf << M6502::InstructionMnemonicTable[opcode] << " ("
          << list.getLabel(mySystem.peek(address + 1), isRead, 2) << "),y ; "
          << M6502::InstructionCycleTable[opcode];
      count = 2;
      break;

    case M6502::Relative:
      buf << M6502::InstructionMnemonicTable[opcode] << " "
          << list.getLabel(address + 2 + ((Int16)(Int8)mySystem.peek(address + 1)), isRead, 4)
          << " ; " << M6502::InstructionCycleTable[opcode];
      count = 2;
      break;

    case M6502::Zero:
      buf << M6502::InstructionMnemonicTable[opcode] << " "
          << list.getLabel(mySystem.peek(address + 1), isRead, 2) << " ; "
          << M6502::InstructionCycleTable[opcode];
      count = 2;
      break;

    case M6502::ZeroX:
      buf << M6502::InstructionMnemonicTable[opcode] << " "
          << list.getLabel(mySystem.peek(address + 1), isRead, 2) << ",x ; "
          << M6502::InstructionCycleTable[opcode];
      count = 2;
      break;

    case M6502::ZeroY:
      buf << M6502::InstructionMnemonicTable[opcode] << " "
          << list.getLabel(mySystem.peek(address + 1), isRead, 2) << ",y ; "
          << M6502::InstructionCycleTable[opcode];
      count = 2;
      break;

    default:
      buf << "dc  $" << hex << setw(2) << setfill('0') << (int) opcode << " ; "
          << dec << M6502::InstructionCycleTable[opcode];
      count = 1;
      break;
  }

  result = buf.str();
  return count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// This doesn't really belong here (banks are Cart properties), but it
// makes like much easier for the expression parser.
int CpuDebug::getBank()
{
  return Debugger::debugger().getBank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CpuDebug::dPeek(int address)
{
  return dpeek(mySystem, address);
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

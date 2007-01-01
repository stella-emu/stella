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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CpuDebug.cxx,v 1.8 2007-01-01 18:04:41 stephena Exp $
//============================================================================

#include "Array.hxx"
#include "EquateList.hxx"
#include "M6502.hxx"

#include "CpuDebug.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CpuDebug::CpuDebug(Debugger* dbg, Console* console)
  : DebuggerSystem(dbg, console),
    mySystem(&(console->system()))
{
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
int CpuDebug::disassemble(int address, char* buffer, EquateList* equateList)
{
  // equateList->dumpAll();
  int opcode = mySystem->peek(address);

  switch(M6502::ourAddressingModeTable[opcode])
  {
    case M6502::Absolute:
      sprintf(buffer, "%s %s ; %d", M6502::ourInstructionMnemonicTable[opcode],
          equateList->getFormatted(dpeek(mySystem, address + 1), 4),
			 M6502::ourInstructionProcessorCycleTable[opcode]);
      return 3;

    case M6502::AbsoluteX:
      sprintf(buffer, "%s %s,x ; %d", M6502::ourInstructionMnemonicTable[opcode],
          equateList->getFormatted(dpeek(mySystem, address + 1), 4),
			 M6502::ourInstructionProcessorCycleTable[opcode]);
      return 3;

    case M6502::AbsoluteY:
      sprintf(buffer, "%s %s,y ; %d", M6502::ourInstructionMnemonicTable[opcode],
          equateList->getFormatted(dpeek(mySystem, address + 1), 4),
			 M6502::ourInstructionProcessorCycleTable[opcode]);
      return 3;

    case M6502::Immediate:
      sprintf(buffer, "%s #$%02X ; %d", M6502::ourInstructionMnemonicTable[opcode],
          mySystem->peek(address + 1),
			 M6502::ourInstructionProcessorCycleTable[opcode]);
      return 2;

    case M6502::Implied:
      sprintf(buffer, "%s ; %d", M6502::ourInstructionMnemonicTable[opcode],
				M6502::ourInstructionProcessorCycleTable[opcode]);
      return 1;

    case M6502::Indirect:
      sprintf(buffer, "%s (%s) ; %d", M6502::ourInstructionMnemonicTable[opcode],
          equateList->getFormatted(dpeek(mySystem, address + 1), 4),
			 M6502::ourInstructionProcessorCycleTable[opcode]);
      return 3;

    case M6502::IndirectX:
      sprintf(buffer, "%s (%s,x) ; %d", 
          M6502::ourInstructionMnemonicTable[opcode], 
          equateList->getFormatted(mySystem->peek(address + 1), 2),
			 M6502::ourInstructionProcessorCycleTable[opcode]);
      return 2;

    case M6502::IndirectY:
      sprintf(buffer, "%s (%s),y ; %d", 
          M6502::ourInstructionMnemonicTable[opcode], 
          equateList->getFormatted(mySystem->peek(address + 1), 2),
			 M6502::ourInstructionProcessorCycleTable[opcode]);
      return 2;

    case M6502::Relative:
      sprintf(buffer, "%s %s ; %d", M6502::ourInstructionMnemonicTable[opcode],
          equateList->getFormatted(address + 2 + ((Int16)(Int8)mySystem->peek(address + 1)), 4),
			 M6502::ourInstructionProcessorCycleTable[opcode]); 
      return 2;

    case M6502::Zero:
      sprintf(buffer, "%s %s ; %d", M6502::ourInstructionMnemonicTable[opcode],
          equateList->getFormatted(mySystem->peek(address + 1), 2),
			 M6502::ourInstructionProcessorCycleTable[opcode]);
      return 2;

    case M6502::ZeroX:
      sprintf(buffer, "%s %s,x ; %d", M6502::ourInstructionMnemonicTable[opcode],
          equateList->getFormatted(mySystem->peek(address + 1), 2),
			 M6502::ourInstructionProcessorCycleTable[opcode]);
      return 2;

    case M6502::ZeroY:
      sprintf(buffer, "%s %s,y ; %d", M6502::ourInstructionMnemonicTable[opcode],
          equateList->getFormatted(mySystem->peek(address + 1), 2),
			 M6502::ourInstructionProcessorCycleTable[opcode]);
      return 2;

    default:
      sprintf(buffer, "dc  $%02X ; %d", opcode,
				M6502::ourInstructionProcessorCycleTable[opcode]);
      return 1;
  } 
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
  mySystem->m6502().PC = pc;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setSP(int sp)
{
  mySystem->m6502().SP = sp;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setPS(int ps)
{
  mySystem->m6502().PS(ps);
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setN(bool on)
{
  setPS( set_bit(mySystem->m6502().PS(), 7, on) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setV(bool on)
{
  setPS( set_bit(mySystem->m6502().PS(), 6, on) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setB(bool on)
{
  setPS( set_bit(mySystem->m6502().PS(), 4, on) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setD(bool on)
{
  setPS( set_bit(mySystem->m6502().PS(), 3, on) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setI(bool on)
{
  setPS( set_bit(mySystem->m6502().PS(), 2, on) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setZ(bool on)
{
  setPS( set_bit(mySystem->m6502().PS(), 1, on) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::setC(bool on)
{
  setPS( set_bit(mySystem->m6502().PS(), 0, on) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::toggleN()
{
  setPS( mySystem->m6502().PS() ^ 0x80 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::toggleV()
{
  setPS( mySystem->m6502().PS() ^ 0x40 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::toggleB()
{
  setPS( mySystem->m6502().PS() ^ 0x10 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::toggleD()
{
  setPS( mySystem->m6502().PS() ^ 0x08 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::toggleI()
{
  setPS( mySystem->m6502().PS() ^ 0x04 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::toggleZ()
{
  setPS( mySystem->m6502().PS() ^ 0x02 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CpuDebug::toggleC()
{
  setPS( mySystem->m6502().PS() ^ 0x01 );
}

//============================================================================
//
// MM     MM  6666  555555  0000   2222
// MMMM MMMM 66  66 55     00  00 22  22
// MM MMM MM 66     55     00  00     22
// MM  M  MM 66666  55555  00  00  22222  --  "A 6502 Microprocessor Emulator"
// MM     MM 66  66     55 00  00 22
// MM     MM 66  66 55  55 00  00 22
// MM     MM  6666   5555   0000  222222
//
// Copyright (c) 1995-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: D6502.cxx,v 1.2 2005-06-13 02:47:44 urchlay Exp $
//============================================================================

#include <stdio.h>
#include "D6502.hxx"
#include "M6502.hxx"
#include "System.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
D6502::D6502(System* system)
    : mySystem(system)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
D6502::~D6502()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static uInt16 dpeek(System* system, uInt16 address)
{
  return (uInt16)system->peek(address) |
      (((uInt16)system->peek(address + 1)) << 8);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 D6502::dPeek(uInt16 address)
{
  return dpeek(mySystem, address);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 D6502::disassemble(uInt16 address, char* buffer)
{
  uInt8 opcode = mySystem->peek(address);

  switch(M6502::ourAddressingModeTable[opcode])
  {
    case M6502::Absolute:
      sprintf(buffer, "%s $%04X", M6502::ourInstructionMnemonicTable[opcode],
          dpeek(mySystem, address + 1));
      return 3;

    case M6502::AbsoluteX:
      sprintf(buffer, "%s $%04X,x", M6502::ourInstructionMnemonicTable[opcode],
          dpeek(mySystem, address + 1));
      return 3;

    case M6502::AbsoluteY:
      sprintf(buffer, "%s $%04X,y", M6502::ourInstructionMnemonicTable[opcode],
          dpeek(mySystem, address + 1));
      return 3;

    case M6502::Immediate:
      sprintf(buffer, "%s #$%02X", M6502::ourInstructionMnemonicTable[opcode],
          mySystem->peek(address + 1));
      return 2;

    case M6502::Implied:
      sprintf(buffer, "%s", M6502::ourInstructionMnemonicTable[opcode]);
      return 1;

    case M6502::Indirect:
      sprintf(buffer, "%s ($%04X)", M6502::ourInstructionMnemonicTable[opcode],
          dpeek(mySystem, address + 1));
      return 3;

    case M6502::IndirectX:
      sprintf(buffer, "%s ($%02X,x)", 
          M6502::ourInstructionMnemonicTable[opcode], 
          mySystem->peek(address + 1));
      return 2;

    case M6502::IndirectY:
      sprintf(buffer, "%s ($%02X),y", 
          M6502::ourInstructionMnemonicTable[opcode], 
          mySystem->peek(address + 1));
      return 2;

    case M6502::Relative:
      sprintf(buffer, "%s $%04X", M6502::ourInstructionMnemonicTable[opcode],
          address + 2 + ((Int16)(Int8)mySystem->peek(address + 1))); 
      return 2;

    case M6502::Zero:
      sprintf(buffer, "%s $%02X", M6502::ourInstructionMnemonicTable[opcode],
          mySystem->peek(address + 1));
      return 2;

    case M6502::ZeroX:
      sprintf(buffer, "%s $%02X,x", M6502::ourInstructionMnemonicTable[opcode],
          mySystem->peek(address + 1));
      return 2;

    case M6502::ZeroY:
      sprintf(buffer, "%s $%02X,y", M6502::ourInstructionMnemonicTable[opcode],
          mySystem->peek(address + 1));
      return 2;

    default:
      sprintf(buffer, "dc  $%02X", opcode);
      return 1;
  } 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 D6502::a()
{
  return mySystem->m6502().A;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void D6502::a(uInt8 value)
{
  mySystem->m6502().A = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 D6502::pc()
{
  return mySystem->m6502().PC;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void D6502::pc(uInt16 value)
{
  mySystem->m6502().PC = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 D6502::ps()
{
  return mySystem->m6502().PS();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void D6502::ps(uInt8 value)
{
  mySystem->m6502().PS(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 D6502::sp()
{
  return mySystem->m6502().SP;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void D6502::sp(uInt8 value)
{
  mySystem->m6502().SP = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 D6502::x()
{
  return mySystem->m6502().X;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void D6502::x(uInt8 value)
{
  mySystem->m6502().X = value;
}
   
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 D6502::y()
{
  return mySystem->m6502().Y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void D6502::y(uInt8 value)
{
  mySystem->m6502().Y = value;
}


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
// $Id: M6502.cxx,v 1.1.1.1 2001-12-27 19:54:30 bwmott Exp $
//============================================================================

#include "M6502.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6502::M6502(uInt32 systemCyclesPerProcessorCycle)
    : myExecutionStatus(0),
      mySystem(0),
      mySystemCyclesPerProcessorCycle(systemCyclesPerProcessorCycle)
{
  uInt16 t;

  // Compute the BCD lookup table
  for(t = 0; t < 256; ++t)
  {
    ourBCDTable[0][t] = ((t >> 4) * 10) + (t & 0x0f);
    ourBCDTable[1][t] = (((t % 100) / 10) << 4) | (t % 10);
  }

  // Compute the System Cycle table
  for(t = 0; t < 256; ++t)
  {
    myInstructionSystemCycleTable[t] = ourInstructionProcessorCycleTable[t] *
        mySystemCyclesPerProcessorCycle;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6502::~M6502()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::install(System& system)
{
  // Remember which system I'm installed in
  mySystem = &system;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::reset()
{
  // Clear the execution status flags
  myExecutionStatus = 0;

  // Set registers to default values
  A = X = Y = 0;
  SP = 0xff;
  PS(0x20);

  // Load PC from the reset vector
  PC = (uInt16)mySystem->peek(0xfffc) | ((uInt16)mySystem->peek(0xfffd) << 8);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::irq()
{
  myExecutionStatus |= MaskableInterruptBit;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::nmi()
{
  myExecutionStatus |= NonmaskableInterruptBit;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::stop()
{
  myExecutionStatus |= StopExecutionBit;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6502::AddressingMode M6502::addressingMode(uInt8 opcode) const
{
  return ourAddressingModeTable[opcode];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 M6502::PS() const
{
  uInt8 ps = 0x20;

  if(N) 
    ps |= 0x80;
  if(V) 
    ps |= 0x40;
  if(B) 
    ps |= 0x10;
  if(D) 
    ps |= 0x08;
  if(I) 
    ps |= 0x04;
  if(!notZ) 
    ps |= 0x02;
  if(C) 
    ps |= 0x01;

  return ps;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6502::PS(uInt8 ps)
{
  N = ps & 0x80;
  V = ps & 0x40;
  B = ps & 0x10;
  D = ps & 0x08;
  I = ps & 0x04;
  notZ = !(ps & 0x02);
  C = ps & 0x01;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream& operator<<(ostream& out, const M6502::AddressingMode& mode)
{
  switch(mode)
  {
    case M6502::Absolute:
      out << "$nnnn  ";
      break;
    case M6502::AbsoluteX:
      out << "$nnnn,X";
      break;
    case M6502::AbsoluteY:
      out << "$nnnn,Y";
      break;
    case M6502::Implied:
      out << "implied";
      break;
    case M6502::Immediate:
      out << "#$nn   ";
      break;
    case M6502::Indirect:
      out << "($nnnn)";
      break;
    case M6502::IndirectX:
      out << "($nn,X)";
      break;
    case M6502::IndirectY:
      out << "($nn),Y";
      break;
    case M6502::Invalid:
      out << "invalid";
      break;
    case M6502::Relative:
      out << "$nn    ";
      break;
    case M6502::Zero:
      out << "$nn    ";
      break;
    case M6502::ZeroX:
      out << "$nn,X  ";
      break;
    case M6502::ZeroY:
      out << "$nn,Y  ";
      break;
  }
  return out;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 M6502::ourBCDTable[2][256];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6502::AddressingMode M6502::ourAddressingModeTable[256] = {
    Implied,    IndirectX, Invalid,   IndirectX,  // 0x0?
    Zero,       Zero,      Zero,      Zero,
    Implied,    Immediate, Implied,   Invalid,
    Absolute,   Absolute,  Absolute,  Absolute,

    Relative,   IndirectY, Invalid,   IndirectY,  // 0x1?
    ZeroX,      ZeroX,     ZeroX,     ZeroX,
    Implied,    AbsoluteY, Implied,   AbsoluteY,
    AbsoluteX,  AbsoluteX, AbsoluteX, AbsoluteX,

    Absolute,   IndirectX, Invalid,   IndirectX,  // 0x2?
    Zero,       Zero,      Zero,      Zero,
    Implied,    Immediate, Implied,   Invalid,
    Absolute,   Absolute,  Absolute,  Absolute,

    Relative,   IndirectY, Invalid,   IndirectY,  // 0x3?
    ZeroX,      ZeroX,     ZeroX,     ZeroX,
    Implied,    AbsoluteY, Implied,   AbsoluteY,
    AbsoluteX,  AbsoluteX, AbsoluteX, AbsoluteX,

    Implied,    IndirectX, Invalid,   Invalid,    // 0x4?
    Zero,       Zero,      Zero,      Invalid,
    Implied,    Immediate, Implied,   Invalid,
    Absolute,   Absolute,  Absolute,  Invalid,

    Relative,   IndirectY, Invalid,   Invalid,    // 0x5?
    ZeroX,      ZeroX,     ZeroX,     Invalid,
    Implied,    AbsoluteY, Implied,   Invalid,
    AbsoluteX,  AbsoluteX, AbsoluteX, Invalid,

    Implied,    IndirectX, Invalid,   Invalid,    // 0x6?
    Zero,       Zero,      Zero,      Invalid,
    Implied,    Immediate, Implied,   Invalid,
    Indirect,   Absolute,  Absolute,  Invalid,

    Relative,   IndirectY, Invalid,   Invalid,    // 0x7?
    ZeroX,      ZeroX,     ZeroX,     Invalid,
    Implied,    AbsoluteY, Implied,   Invalid,
    AbsoluteX,  AbsoluteX, AbsoluteX, Invalid,

    Immediate,  IndirectX, Immediate, IndirectX,  // 0x8?
    Zero,       Zero,      Zero,      Zero,
    Implied,    Invalid,   Implied,   Invalid,
    Absolute,   Absolute,  Absolute,  Absolute,

    Relative,   IndirectY, Invalid,   Invalid,    // 0x9?
    ZeroX,      ZeroX,     ZeroY,     ZeroY,
    Implied,    AbsoluteY, Implied,   Invalid,
    Invalid,    AbsoluteX, Invalid,   Invalid,

    Immediate,  IndirectX, Immediate, Invalid,    // 0xA?
    Zero,       Zero,      Zero,      Invalid,
    Implied,    Immediate, Implied,   Invalid,
    Absolute,   Absolute,  Absolute,  Invalid,

    Relative,   IndirectY, Invalid,   Invalid,    // 0xB?
    ZeroX,      ZeroX,     ZeroY,     Invalid,
    Implied,    AbsoluteY, Implied,   Invalid,
    AbsoluteX,  AbsoluteX, AbsoluteY, Invalid,

    Immediate,  IndirectX, Immediate, Invalid,    // 0xC?
    Zero,       Zero,      Zero,      Invalid,
    Implied,    Immediate, Implied,   Invalid,
    Absolute,   Absolute,  Absolute,  Invalid,

    Relative,   IndirectY, Invalid,   Invalid,    // 0xD?
    ZeroX,      ZeroX,     ZeroX,     Invalid,
    Implied,    AbsoluteY, Implied,   Invalid,
    AbsoluteX,  AbsoluteX, AbsoluteX, Invalid,

    Immediate,  IndirectX, Immediate, Invalid,    // 0xE?
    Zero,       Zero,      Zero,      Invalid,
    Implied,    Immediate, Implied,   Invalid,
    Absolute,   Absolute,  Absolute,  Invalid,

    Relative,   IndirectY, Invalid,   Invalid,    // 0xF?
    ZeroX,      ZeroX,     ZeroX,     Invalid,
    Implied,    AbsoluteY, Implied,   Invalid,
    AbsoluteX,  AbsoluteX, AbsoluteX, Invalid
  };

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 M6502::ourInstructionProcessorCycleTable[256] = {
//  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
    7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,  // 0
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,  // 1
    6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,  // 2
    2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,  // 3
    6, 6, 2, 2, 3, 3, 5, 2, 3, 2, 2, 2, 3, 4, 6, 2,  // 4
    2, 5, 2, 2, 4, 4, 6, 2, 2, 4, 2, 2, 4, 4, 7, 2,  // 5
    6, 6, 2, 2, 3, 3, 5, 2, 4, 2, 2, 2, 5, 4, 6, 2,  // 6
    2, 5, 2, 2, 4, 4, 6, 2, 2, 4, 2, 2, 4, 4, 7, 2,  // 7
    2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,  // 8
    2, 6, 2, 2, 4, 4, 4, 4, 2, 5, 2, 2, 2, 5, 2, 2,  // 9
    2, 6, 2, 2, 3, 3, 3, 2, 2, 2, 2, 2, 4, 4, 4, 2,  // a
    2, 5, 2, 2, 4, 4, 4, 2, 2, 4, 2, 2, 4, 4, 4, 2,  // b
    2, 6, 2, 2, 3, 3, 5, 2, 2, 2, 2, 2, 4, 4, 6, 2,  // c
    2, 5, 2, 2, 4, 4, 6, 2, 2, 4, 2, 2, 4, 4, 7, 2,  // d
    2, 6, 2, 2, 3, 3, 5, 2, 2, 2, 2, 2, 4, 4, 6, 2,  // e
    2, 5, 2, 2, 4, 4, 6, 2, 2, 4, 2, 2, 4, 4, 7, 2   // f
  };

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* M6502::ourInstructionMnemonicTable[256] = {
  "BRK",  "ORA",  "n/a",  "aso",  "nop",  "ORA",  "ASL",  "aso",    // 0x0?
  "PHP",  "ORA",  "ASLA", "n/a",  "nop",  "ORA",  "ASL",  "aso",

  "BPL",  "ORA",  "n/a",  "aso",  "nop",  "ORA",  "ASL",  "aso",    // 0x1?
  "CLC",  "ORA",  "nop",  "aso",  "nop",  "ORA",  "ASL",  "aso",

  "JSR",  "AND",  "n/a",  "rla",  "BIT",  "AND",  "ROL",  "rla",    // 0x2?
  "PLP",  "AND",  "ROLA", "n/a",  "BIT",  "AND",  "ROL",  "rla",

  "BMI",  "AND",  "rla",  "n/a",  "nop",  "AND",  "ROL",  "rla",    // 0x3?
  "SEC",  "AND",  "nop",  "rla",  "nop",  "AND",  "ROL",  "rla",
  
  "RTI",  "EOR",  "n/a",  "n/a",  "nop",  "EOR",  "LSR",  "n/a",    // 0x4?
  "PHA",  "EOR",  "LSRA", "n/a",  "JMP",  "EOR",  "LSR",  "n/a",

  "BVC",  "EOR",  "n/a",  "n/a",  "nop",  "EOR",  "LSR",  "n/a",    // 0x5?
  "CLI",  "EOR",  "nop",  "n/a",  "nop",  "EOR",  "LSR",  "n/a",

  "RTS",  "ADC",  "n/a",  "n/a",  "nop",  "ADC",  "ROR",  "n/a",    // 0x6?
  "PLA",  "ADC",  "RORA", "n/a",  "JMP",  "ADC",  "ROR",  "n/a",

  "BVS",  "ADC",  "n/a",  "n/a",  "nop",  "ADC",  "ROR",  "n/a",    // 0x7?
  "SEI",  "ADC",  "nop",  "n/a",  "nop",  "ADC",  "ROR",  "n/a",

  "nop",  "STA",  "nop",  "axs",  "STY",  "STA",  "STX",  "axs",    // 0x8?
  "DEY",  "n/a",  "TXA",  "n/a",  "STY",  "STA",  "STX",  "axs",

  "BCC",  "STA",  "n/a",  "n/a",  "STY",  "STA",  "STX",  "axs",    // 0x9?
  "TYA",  "STA",  "TXS",  "n/a",  "n/a",  "STA",  "n/a",  "n/a",

  "LDY",  "LDA",  "LDX",  "n/a",  "LDY",  "LDA",  "LDX",  "n/a",    // 0xA?
  "TAY",  "LDA",  "TAX",  "n/a",  "LDY",  "LDA",  "LDX",  "n/a",

  "BCS",  "LDA",  "n/a",  "n/a",  "LDY",  "LDA",  "LDX",  "n/a",    // 0xB?
  "CLV",  "LDA",  "TSX",  "n/a",  "LDY",  "LDA",  "LDX",  "n/a",

  "CPY",  "CMP",  "nop",  "n/a",  "CPY",  "CMP",  "DEC",  "n/a",    // 0xC?
  "INY",  "CMP",  "DEX",  "n/a",  "CPY",  "CMP",  "DEC",  "n/a",

  "BNE",  "CMP",  "n/a",  "n/a",  "nop",  "CMP",  "DEC",  "n/a",    // 0xD?
  "CLD",  "CMP",  "nop",  "n/a",  "nop",  "CMP",  "DEC",  "n/a",

  "CPX",  "SBC",  "nop",  "n/a",  "CPX",  "SBC",  "INC",  "n/a",    // 0xE?
  "INX",  "SBC",  "NOP",  "n/a",  "CPX",  "SBC",  "INC",  "n/a",

  "BEQ",  "SBC",  "n/a",  "n/a",  "nop",  "SBC",  "INC",  "n/a",    // 0xF?
  "SED",  "SBC",  "nop",  "n/a",  "nop",  "SBC",  "INC",  "n/a"
};


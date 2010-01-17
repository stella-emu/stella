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

#ifndef DISTELLA_HXX
#define DISTELLA_HXX

#include <queue>

#include "Array.hxx"
#include "bspf.hxx"

//#include "CartDebug.hxx"

//// The following will go in CartDebug
    struct DisassemblyTag {
      int address;
      string disasm;
      string bytes;
    };
    typedef Common::Array<DisassemblyTag> DisassemblyList;
//////////////////////////////////////////////////////////////

class DiStella
{
  public:
    DiStella();
    ~DiStella();

  public:
    int disassemble(DisassemblyList& list, const char* datafile);

  private:
    struct resource {
      uInt16 start;
      uInt16 load;
      uInt32 length;
      uInt16 end;
      int disp_data;
    } app_data;

    /* Memory */
    uInt8* mem;
    uInt8* labels;

    uInt8 reserved[64];
    uInt8 ioresrvd[24];
    uInt8 pokresvd[16];
    char linebuff[256],nextline[256];
    FILE* cfg;

    uInt32 pc, pcbeg, pcend, offset, start_adr;
    int cflag, dflag, lineno, charcnt;

    queue<uInt16> myAddressQueue;

  private:
    // Marked bits
    //   This is a reference sheet of bits that can be set for a given address, which
    //   are stored in the labels[] array.
    enum MarkType {
      REFERENCED  = 1 << 0,  /* code somewhere in the program references it, i.e. LDA $F372 referenced $F372 */
      VALID_ENTRY = 1 << 1,  /* addresses that can have a label placed in front of it. A good counterexample
                                would be "FF00: LDA $FE00"; $FF01 would be in the middle of a multi-byte
                                instruction, and therefore cannot be labelled. */
      DATA        = 1 << 2,
      GFX         = 1 << 3,
      REACHABLE   = 1 << 4   /* disassemble-able code segments */
    };

    /**
      Enumeration of the 6502 addressing modes
    */
    enum AddressingMode
    {
      IMPLIED, ACCUMULATOR, IMMEDIATE,
      ZERO_PAGE, ZERO_PAGE_X, ZERO_PAGE_Y,
      ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y,
      ABS_INDIRECT, INDIRECT_X, INDIRECT_Y,
      RELATIVE, ASS_CODE
    };

    /**
      Enumeration of the 6502 access modes
    */
    enum AccessMode
    {
      M_NONE, M_AC, M_XR, M_YR, M_SP, M_SR, M_PC, M_IMM, M_ZERO, M_ZERX, M_ZERY,
      M_ABS, M_ABSX, M_ABSY, M_AIND, M_INDX, M_INDY, M_REL, M_FC, M_FD, M_FI,
      M_FV, M_ADDR, M_,

      M_ACIM, /* Source: AC & IMMED (bus collision) */
      M_ANXR, /* Source: AC & XR (bus collision) */
      M_AXIM, /* Source: (AC | #EE) & XR & IMMED (bus collision) */
      M_ACNC, /* Dest: M_AC and Carry = Negative */
      M_ACXR, /* Dest: M_AC, M_XR */

      M_SABY, /* Source: (ABS_Y & SP) (bus collision) */
      M_ACXS, /* Dest: M_AC, M_XR, M_SP */
      M_STH0, /* Dest: Store (src & Addr_Hi+1) to (Addr +0x100) */
      M_STH1,
      M_STH2,
      M_STH3
    };

uInt32 filesize(FILE *stream);
uInt32 read_adr();
int file_load(const char* file);
int load_config(const char* file);
void check_range(uInt32 beg, uInt32 end);
void disasm(uInt32 distart, int pass, DisassemblyList& list);
int mark(uInt32 address, MarkType bit);
int check_bit(uInt8 bitflags, int i);
void showgfx(uInt8 c);

    struct Instruction_tag {
      const char*    mnemonic;
      AddressingMode addr_mode;
      AccessMode     source;
      AccessMode     destination;
      uInt8          cycles;
    };
    static const Instruction_tag ourLookup[256];

    /// Table of instruction mnemonics
    static const char* ourTIAMnemonic[62];
    static const char* ourIOMnemonic[24];

    static const int ourCLength[14];
};

#endif

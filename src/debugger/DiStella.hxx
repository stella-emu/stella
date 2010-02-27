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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef DISTELLA_HXX
#define DISTELLA_HXX

#include <queue>
#include <sstream>

#include "Array.hxx"
#include "bspf.hxx"

#include "CartDebug.hxx"

/**
  This class is a wrapper around the Distella code.  Much of the code remains
  exactly the same, except that generated data is now redirected to a
  DisassemblyList structure rather than being printed.

  All 7800-related stuff has been removed, as well as all commandline options.
  For now, the only configurable item is whether to automatically determine
  code vs. data sections (which is on by default).  Over time, some of the
  configurability of Distella may be added again.

  @author  Stephen Anthony
*/
class DiStella
{
  public:
    /**
      Disassemble the current state of the System from the given start address.

      @param list     The results of the disassembly are placed here
      @param start    The address at which to start disassembly
      @param autocode If enabled, try to determine code vs. data sections
    */
    DiStella(CartDebug::DisassemblyList& list, uInt16 start,
             bool autocode = true);

    ~DiStella();

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

    // Indicate that a new line of disassembly has been completed
    // In the original Distella code, this indicated a new line to be printed
    // Here, we add a new entry to the DisassemblyList
    void addEntry(CartDebug::DisassemblyList& list);

    // These functions are part of the original Distella code
    void disasm(CartDebug::DisassemblyList& list, uInt32 distart, int pass);
    int mark(uInt32 address, MarkType bit);
    int check_bit(uInt8 bitflags, int i);
    void showgfx(uInt8 c);

  private:
    stringstream myBuf;
    queue<uInt16> myAddressQueue;
    uInt32 myOffset, myPC, myPCBeg, myPCEnd;

    struct resource {
      uInt16 start;
      uInt32 length;
      uInt16 end;
    } myAppData;

    /* Memory */
    uInt8* labels;

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

    /**
      Enumeration of the 6502 read/write mode
      (if the opcode is reading or writing its operand)
    */
    enum ReadWriteMode
    {
      READ, WRITE, NONE
    };

    struct Instruction_tag {
      const char*    mnemonic;
      AddressingMode addr_mode;
      AccessMode     source;
      ReadWriteMode  rw_mode;
      uInt8          cycles;
    };
    static const Instruction_tag ourLookup[256];

    /// Table of instruction mnemonics
    static const char* ourTIAMnemonicR[16];  // read mode
    static const char* ourTIAMnemonicW[64];  // write mode
    static const char* ourIOMnemonic[24];

    static const int ourCLength[14];
};

#endif

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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
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
#include "DebuggerParser.hxx"

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

      @param dbg         The CartDebug instance containing all label information
      @param list        The results of the disassembly are placed here
      @param info        Various info about the current bank
      @param labels      Array storing label info determined by Distella
      @param directives  Array storing directive info determined by Distella
      @param resolvedata If enabled, try to determine code vs. data sections
    */
    DiStella(const CartDebug& dbg, CartDebug::DisassemblyList& list,
             CartDebug::BankInfo& info, uInt8* labels, uInt8* directives,
             bool resolvedata);

    ~DiStella();

  public:
    // A list of options that can be applied to the disassembly
    // This will eventually grow to include all options supported by
    // standalone Distella
    typedef struct {
      BaseFormat gfx_format;
      bool show_addresses;
      bool fflag;  // Forces correct address length (-f in Distella)
      bool rflag;  // Relocate calls out of address range (-r in Distella)
    } Settings;
    static Settings settings;

  private:
    // Indicate that a new line of disassembly has been completed
    // In the original Distella code, this indicated a new line to be printed
    // Here, we add a new entry to the DisassemblyList
    void addEntry(CartDebug::DisasmType type);

    // Process directives given in the list
    // Directives are basically the contents of a distella configuration file
    void processDirectives(const CartDebug::DirectiveList& directives);

    // These functions are part of the original Distella code
    void disasm(uInt32 distart, int pass);
    bool check_range(uInt16 start, uInt16 end) const;
    int mark(uInt32 address, uInt8 mask, bool directive = false);
    bool check_bit(uInt16 address, uInt8 mask) const;

  private:
    const CartDebug& myDbg;
    CartDebug::DisassemblyList& myList;
    stringstream myDisasmBuf;
    queue<uInt16> myAddressQueue;
    uInt16 myOffset, myPC, myPCBeg, myPCEnd;

    struct resource {
      uInt16 start;
      uInt16 end;
      uInt16 length;
    } myAppData;

    /* Stores info on how each address is marked, both in the general
       case as well as when manual directives are enabled (in which case
       the directives take priority
       The address mark type is defined in CartDebug.hxx */
    uInt8 *myLabels, *myDirectives;

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
};

#endif

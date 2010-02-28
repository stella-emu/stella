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

#ifndef CART_DEBUG_HXX
#define CART_DEBUG_HXX

class System;

#include <map>

#include "bspf.hxx"
#include "Array.hxx"
#include "Cart.hxx"
#include "DebuggerSystem.hxx"

// pointer types for CartDebug instance methods
typedef int (CartDebug::*CARTDEBUG_INT_METHOD)();

// call the pointed-to method on the (global) CPU debugger object.
#define CALL_CARTDEBUG_METHOD(method) ( ( Debugger::debugger().cartDebug().*method)() )

class CartState : public DebuggerState
{
  public:
    IntArray ram;    // The actual data values
    IntArray rport;  // Address for reading from RAM
    IntArray wport;  // Address for writing to RAM
};

class CartDebug : public DebuggerSystem
{
  // The disassembler needs special access to this class
  friend class DiStella;

  public:
    struct DisassemblyTag {
      uInt16 address;
      string label;
      string disasm;
      string bytes;
    };
    typedef Common::Array<DisassemblyTag> DisassemblyList;

  public:
    CartDebug(Debugger& dbg, Console& console, const RamAreaList& areas);

    const DebuggerState& getState();
    const DebuggerState& getOldState() { return myOldState; }

    void saveOldState();
    string toString();

    // The following assume that the given addresses are using the
    // correct read/write port ranges; no checking will be done to
    // confirm this.
    uInt8 read(uInt16 addr);
    void write(uInt16 addr, uInt8 value);

    // Return the address at which an invalid read was performed in a
    // write port area.
    int readFromWritePort();

    // Indicate that a read from write port has occurred.
    void triggerReadFromWritePort(uInt16 addr) { myRWPortAddress = addr; }

    /**
      Let the Cart debugger subsystem treat this area as addressable memory.

      @param start    The beginning of the RAM area (0x0000 - 0x2000)
      @param size     Total number of bytes of area
      @param roffset  Offset to use when reading from RAM (read port)
      @param woffset  Offset to use when writing to RAM (write port)
    */
    void addRamArea(uInt16 start, uInt16 size, uInt16 roffset, uInt16 woffset);

    // The following two methods are meant to be used together
    // First, a call is made to disassemble(), which updates the disassembly
    // list; it will figure out when an actual complete disassembly is
    // required, and when the previous results can be used
    //
    // Later, successive calls to disassemblyList() simply return the
    // previous results; no disassembly is done in this case
    /**
      Disassemble from the given address using the Distella disassembler
      Address-to-label mappings (and vice-versa) are also determined here

      @return  True if disassembly changed from previous call, else false
    */
    bool disassemble(bool autocode);

    /**
      Get the results from the most recent call to disassemble()
    */
    const DisassemblyList& disassemblyList() const { return myDisassembly; }

    /**
      Determine the line in the disassembly that corresponds to the given
      address.  A value of zero indicates that no such address exists.
    */
    int addressToLine(uInt16 address) const;

    /**
      Disassemble from the starting address the specified number of lines.
      Note that automatic code determination is turned off for this method;
      it will treat all address contents as instructions.
    */
    string disassemble(uInt16 start, uInt16 lines) const;

    int getBank();
    int bankCount();
    string getCartType();

////////////////////////////////////////
    /**
      Add a label and associated address.
      Labels that reference either TIA or RIOT spaces will not be processed.
    */
    void addLabel(const string& label, uInt16 address);

    /**
      Remove the given label and its associated address.
      Labels that reference either TIA or RIOT spaces will not be processed.
    */
    bool removeLabel(const string& label);

    /**
      Accessor methods for labels and addresses

      The mapping from address to label can be one-to-many (ie, an
      address can have different labels depending on its context, and
      whether its being read or written; if isRead is true, the context
      is a read, else it's a write
      If places is not -1 and a label hasn't been defined, return a
      formatted hexidecimal address
    */
    const string& getLabel(uInt16 addr, bool isRead, int places = -1) const;
    int getAddress(const string& label) const;

    /**
      Load user equates from the given symbol file (as generated by DASM)
    */
    string loadSymbolFile(const string& file);

    /**
      Methods used by the command parser for tab-completion
    */
    int countCompletions(const char *in);
    const string& getCompletions() const      { return myCompletions; }
    const string& getCompletionPrefix() const { return myCompPrefix;  }
////////////////////////////////////////

  private:
    typedef map<uInt16, string> AddrToLabel;
    typedef map<string, uInt16> LabelToAddr;

    // Determine 'type' of address (ie, what part of the system accessed)
    enum AddrType {
      ADDR_TIA,
      ADDR_RAM,
      ADDR_RIOT,
      ADDR_ROM
    };
    AddrType addressType(uInt16 addr) const;

    // Extract labels and values from the given character stream
    string extractLabel(char *c) const;
    int extractValue(char *c) const;

    // Count completions for the given mapping
    int countCompletions(const char *in, LabelToAddr& addresses);

  private:
    CartState myState;
    CartState myOldState;
    RamAreaList myRamAreas;

    // Used for the disassembly display, and mapping from addresses
    // to corresponding lines of text in that display
    DisassemblyList myDisassembly;
    map<uInt16, int> myAddrToLineList;

    // Mappings from label to address (and vice versa) for items
    // defined by the user (either through a symbol file or manually
    // from the commandline in the debugger)
    AddrToLabel myUserLabels;
    LabelToAddr myUserAddresses;

    string myCompletions;
    string myCompPrefix;

    uInt16 myRWPortAddress;

    /// Table of instruction mnemonics
    static const char* ourTIAMnemonicR[16];  // read mode
    static const char* ourTIAMnemonicW[64];  // write mode
    static const char* ourIOMnemonic[24];
};

#endif

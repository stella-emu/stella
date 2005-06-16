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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Debugger.hxx,v 1.6 2005-06-16 00:20:11 stephena Exp $
//============================================================================

#ifndef DEBUGGER_HXX
#define DEBUGGER_HXX

class OSystem;
class DebuggerParser;

class Console;
class System;
class D6502;

#include "DialogContainer.hxx"
#include "M6502.hxx"
#include "EquateList.hxx"
#include "bspf.hxx"

enum {
  kDebuggerWidth = 511,
  kDebuggerHeight = 383
};

// Constants for RAM area
enum {
  kRamStart = 0x80,
  kRamSize = 128
};

/**
  The base dialog for all debugging widgets in Stella.  Also acts as the parent
  for all debugging operations in Stella (parser, 6502 debugger, etc).

  @author  Stephen Anthony
  @version $Id: Debugger.hxx,v 1.6 2005-06-16 00:20:11 stephena Exp $
*/
class Debugger : public DialogContainer
{
  friend class DebuggerParser;

  public:
    /**
      Create a new menu stack
    */
    Debugger(OSystem* osystem);

    /**
      Destructor
    */
    virtual ~Debugger();

  public:
    /**
      Updates the basedialog to be of the type defined for this derived class.
    */
    void initialize();

    /**
      Initialize the video subsystem wrt this class.
    */
    void initializeVideo();

    /**
      Inform this object of a console change.
    */
    void setConsole(Console* console);

    /** Convenience methods to convert to hexidecimal values */
    static char *to_hex_8(int i)
    {
      static char out[3];
      sprintf(out, "%02x", i);
      return out;
    }
    static char *to_hex_16(int i)
    {
      static char out[5];
      sprintf(out, "%04x", i);
      return out;
    }

  public:
    /**
      Run the debugger command and return the result.
    */
    const string run(const string& command);

    /**
      Give the contents of the CPU registers and disassembly of
      next instruction.
    */
    const string state();

    /**
      Return a formatted string containing the contents of the specified
      device.
    */
    const string dumpRAM(uInt16 start);
    const string dumpTIA();

    // Read and write 128-byte RAM area
    uInt8 readRAM(uInt16 addr);
    void writeRAM(uInt16 addr, uInt8 value);

    // set a bunch of RAM locations at once
    const string setRAM(int argCount, int *args);

    void quit();
    void trace();
    void step();
    void setA(int a);
    void setX(int x);
    void setY(int y);
    void setS(int sp);
    void setPC(int pc);
    void toggleC();
    void toggleZ();
    void toggleN();
    void toggleV();
    void toggleD();
    void reset();

    void formatFlags(int f, char *out);
    EquateList *equates();

  protected:
    Console* myConsole;
    System* mySystem;

    DebuggerParser* myParser;
    D6502* myDebugger;
    EquateList *equateList;
};

#endif

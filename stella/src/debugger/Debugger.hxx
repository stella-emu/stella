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
// $Id: Debugger.hxx,v 1.23 2005-06-22 13:00:58 urchlay Exp $
//============================================================================

#ifndef DEBUGGER_HXX
#define DEBUGGER_HXX

class OSystem;

class Console;
class System;
class D6502;

#include "DialogContainer.hxx"
#include "M6502.hxx"
#include "DebuggerParser.hxx"
#include "EquateList.hxx"
#include "PackedBitArray.hxx"
#include "PromptWidget.hxx"
#include "TIADebug.hxx"
#include "bspf.hxx"

enum {
  kDebuggerWidth = 511,
  kDebuggerLineHeight = 12,   // based on the height of the console font
  kDebuggerLines = 15,
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
  @version $Id: Debugger.hxx,v 1.23 2005-06-22 13:00:58 urchlay Exp $
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
    static char *to_hex_4(int i)
    {
      static char out[2];
      sprintf(out, "%1x", i);
      return out;
    }
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
    static char *to_bin(int dec, int places, char *buf) {
      int bit = 1;
      buf[places] = '\0';
      while(--places >= 0) {
        if(dec & bit)
          buf[places] = '1';
        else
          buf[places] = '0';

        bit <<= 1;
      }
      return buf;
    }
    static char *to_bin_8(int dec) {
      static char buf[9];
      return to_bin(dec, 8, buf);
    }
    static char *to_bin_16(int dec) {
      static char buf[17];
      return to_bin(dec, 16, buf);
    }

    int stringToValue(const string& stringval)
        { return myParser->decipher_arg(stringval); }
    const string valueToString(int value, BaseFormat outputBase);
    const string valueToString(int value);

    void toggleBreakPoint(int bp);

    bool breakPoint(int bp);
    void toggleReadTrap(int t);
    void toggleWriteTrap(int t);
    void toggleTrap(int t);
    bool readTrap(int t);
    bool writeTrap(int t);
    void clearAllTraps();


    string disassemble(int start, int lines);
    bool setHeight(int height);

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
    const string setRAM(IntArray args);

    bool start();
    void quit();
    int trace();
    int step();
    void setA(int a);
    void setX(int x);
    void setY(int y);
    void setS(int sp);
    void setPC(int pc);
    int getPC();
    int getA();
    int getX();
    int getY();
    int getP();
    int getS();
    int peek(int addr);
    int dpeek(int addr);
    void toggleC();
    void toggleZ();
    void toggleN();
    void toggleV();
    void toggleD();
    void reset();
    void autoLoadSymbols(string file);
    void nextFrame();
    void clearAllBreakPoints();

    void formatFlags(int f, char *out);
    EquateList *equates();
    PromptWidget *prompt();
    string showWatches();

	 PackedBitArray *breakpoints() { return breakPoints; }
	 PackedBitArray *readtraps() { return readTraps; }
	 PackedBitArray *writetraps() { return writeTraps; }

  protected:
    Console* myConsole;
    System* mySystem;

    DebuggerParser* myParser;
    D6502* myDebugger;
    EquateList *equateList;
    PackedBitArray *breakPoints;
    PackedBitArray *readTraps;
    PackedBitArray *writeTraps;
    PromptWidget *myPrompt;
    TIADebug *myTIAdebug;
};

#endif

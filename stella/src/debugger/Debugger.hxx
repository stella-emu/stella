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
// $Id: Debugger.hxx,v 1.51 2005-07-14 15:13:14 stephena Exp $
//============================================================================

#ifndef DEBUGGER_HXX
#define DEBUGGER_HXX

class OSystem;
class Console;
class System;
class CpuDebug;
class RamDebug;
class TIADebug;

#include "Array.hxx"
#include "DialogContainer.hxx"
#include "M6502.hxx"
#include "DebuggerParser.hxx"
#include "EquateList.hxx"
#include "PackedBitArray.hxx"
#include "PromptWidget.hxx"
#include "bspf.hxx"

enum {
  kDebuggerWidth = 639,
  kDebuggerLineHeight = 12,   // based on the height of the console font
  kDebuggerLines = 20,
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
  @version $Id: Debugger.hxx,v 1.51 2005-07-14 15:13:14 stephena Exp $
*/
class Debugger : public DialogContainer
{
  friend class DebuggerParser;
  friend class EventHandler;

  public:
    /**
      Create a new debugger parent object
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

    /**
      Wrapper method for EventHandler::enterDebugMode() for those classes
      that don't have access to EventHandler.
    */
    bool start();

    /**
      Wrapper method for EventHandler::leaveDebugMode() for those classes
      that don't have access to EventHandler.
    */
    void quit();

    /**
      The debugger subsystem responsible for all CPU state
    */
    CpuDebug& cpuDebug() const { return *myCpuDebug; }

    /**
      The debugger subsystem responsible for all RAM state
    */
    RamDebug& ramDebug() const { return *myRamDebug; }

    /**
      The debugger subsystem responsible for all TIA state
    */
    TIADebug& tiaDebug() const { return *myTiaDebug; }

    /**
      List of English-like aliases for 6502 opcodes and operands.
    */
    EquateList *equates();

    DebuggerParser *parser() { return myParser; }

    PackedBitArray *breakpoints() { return breakPoints; }
    PackedBitArray *readtraps() { return readTraps; }
    PackedBitArray *writetraps() { return writeTraps; }

    /**
      Run the debugger command and return the result.
    */
    const string run(const string& command);

    /**
      Give the contents of the CPU registers and disassembly of
      next instruction.
    */
    const string cpuState();

    /**
      Get contents of RIOT switch & timer registers
    */
    const string riotState();

    /**
      The current cycle count of the System.
    */
    int cycles();

    /**
      Disassemble from the starting address the specified number of lines
      and place result in a string.
    */
    const string& disassemble(int start, int lines);

    int step();
    int trace();
    void nextFrame(int frames);

    string showWatches();

    /**
      Convert between string->integer and integer->string, taking into
      account the current base format.
    */
    int stringToValue(const string& stringval)
        { return myParser->decipher_arg(stringval); }
    const string valueToString(int value, BaseFormat outputBase = kBASE_DEFAULT);

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

    /**
      This is used when we want the debugger from a class that can't
      receive the debugger object in any other way.

      It's basically a hack to prevent the need to pass debugger objects
      everywhere, but I feel it's better to place it here then in
      YaccParser (which technically isn't related to it at all).
    */
    static Debugger& debugger() { return *myStaticDebugger; }

  private:
    /**
      Save state of each debugger subsystem
    */
    void saveOldState();

    /**
      Set initial state before entering the debugger.
    */
    void setStartState();

    /**
      Set final state before leaving the debugger.
    */
    void setQuitState();

    /**
      Get the dimensions of the debugger dialog (takes mediasource into account)
    */
    void getDialogBounds(int* x, int* y, int* w, int* h);

    /**
      Resize the debugger dialog based on the current dimensions from
      getDialogBounds.
    */
    void resizeDialog();

    void toggleBreakPoint(int bp);

    bool breakPoint(int bp);
    void toggleReadTrap(int t);
    void toggleWriteTrap(int t);
    void toggleTrap(int t);
    bool readTrap(int t);
    bool writeTrap(int t);
    void clearAllTraps();

    int setHeight(int height);

    void reloadROM();

    /**
      Return a formatted string containing the contents of the specified
      device.
    */
    const string dumpRAM();
    const string dumpTIA();

    // set a bunch of RAM locations at once
    const string setRAM(IntArray& args);

    int peek(int addr);
    int dpeek(int addr);

    void reset();
    void autoLoadSymbols(string file);
    void clearAllBreakPoints();

    void formatFlags(BoolArray& b, char *out);
    PromptWidget *prompt() { return myPrompt; }
    void addLabel(string label, int address);

    bool setBank(int bank);
    int bankCount();
    int getBank();
    const char *getCartType();
    bool patchROM(int addr, int value);
    void saveState(int state);
    void loadState(int state);

    const string invIfChanged(int reg, int oldReg);

  protected:
    Console* myConsole;
    System* mySystem;

    DebuggerParser* myParser;
    CpuDebug* myCpuDebug;
    RamDebug* myRamDebug;
    TIADebug* myTiaDebug;

    EquateList *equateList;
    PackedBitArray *breakPoints;
    PackedBitArray *readTraps;
    PackedBitArray *writeTraps;
    PromptWidget *myPrompt;

    static Debugger* myStaticDebugger;
};

#endif

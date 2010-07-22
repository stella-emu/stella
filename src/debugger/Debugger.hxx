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
// Copyright (c) 1995-2010 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef DEBUGGER_HXX
#define DEBUGGER_HXX

class OSystem;
class Console;
class System;
class CartDebug;
class CpuDebug;
class RiotDebug;
class TIADebug;
class TiaInfoWidget;
class TiaOutputWidget;
class TiaZoomWidget;
class EditTextWidget;
class RomWidget;
class Expression;
class Serializer;

#include <map>

#include "Array.hxx"
#include "DialogContainer.hxx"
#include "M6502.hxx"
#include "DebuggerParser.hxx"
#include "PackedBitArray.hxx"
#include "PromptWidget.hxx"
#include "Rect.hxx"
#include "Stack.hxx"
#include "bspf.hxx"

typedef map<string,Expression*> FunctionMap;
typedef map<string,string> FunctionDefMap;

/*
// These will probably turn out to be unneeded, left for reference for now
// pointer types for Debugger instance methods
typedef uInt8 (Debugger::*DEBUGGER_BYTE_METHOD)();
typedef uInt16 (Debugger::*DEBUGGER_WORD_METHOD)();

// call the pointed-to method on the (static) debugger object.
#define CALL_DEBUGGER_METHOD(method) ( ( Debugger::debugger().*method)() )
*/


/**
  The base dialog for all debugging widgets in Stella.  Also acts as the parent
  for all debugging operations in Stella (parser, 6502 debugger, etc).

  @author  Stephen Anthony
  @version $Id$
*/
class Debugger : public DialogContainer
{
  // Make these friend classes, to ease communications with the debugger
  // Although it isn't enforced, these classes should use accessor methods
  // directly, and not touch the instance variables
  friend class DebuggerDialog;
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
	 OSystem *getOSystem() { return myOSystem; }

    /**
      Updates the basedialog to be of the type defined for this derived class.
    */
    void initialize();

    /**
      Initialize the video subsystem wrt this class.
    */
    FBInitStatus initializeVideo();

    /**
      Inform this object of a console change.
    */
    void setConsole(Console* console);

    /**
      Wrapper method for EventHandler::enterDebugMode() for those classes
      that don't have access to EventHandler.

      @param message  Message to display when entering debugger
      @param data     An address associated with the message
    */
    bool start(const string& message = "", int address = -1);

    /**
      Wrapper method for EventHandler::leaveDebugMode() for those classes
      that don't have access to EventHandler.
    */
    void quit();

    bool addFunction(const string& name, const string& def,
                     Expression* exp, bool builtin = false);
    bool delFunction(const string& name);
    const Expression* getFunction(const string& name) const;

    const string& getFunctionDef(const string& name) const;
    const FunctionDefMap getFunctionDefMap() const;
    string builtinHelp() const;

    /**
      Methods used by the command parser for tab-completion
      In this case, return completions from the function list
    */
    void getCompletions(const char* in, StringList& list) const;

    /**
      The debugger subsystem responsible for all CPU state
    */
    CpuDebug& cpuDebug() const { return *myCpuDebug; }

    /**
      The debugger subsystem responsible for all Cart RAM/ROM state
    */
    CartDebug& cartDebug() const { return *myCartDebug; }

    /**
      The debugger subsystem responsible for all RIOT state
    */
    RiotDebug& riotDebug() const { return *myRiotDebug; }

    /**
      The debugger subsystem responsible for all TIA state
    */
    TIADebug& tiaDebug() const { return *myTiaDebug; }

    DebuggerParser& parser() const      { return *myParser;      }
    PackedBitArray& breakpoints() const { return *myBreakPoints; }
    PackedBitArray& readtraps() const   { return *myReadTraps;   }
    PackedBitArray& writetraps() const  { return *myWriteTraps;  }

    /**
      Run the debugger command and return the result.
    */
    const string run(const string& command);

    /**
      The current cycle count of the System.
    */
    int cycles();

    string autoExec();

    string showWatches();

    /**
      Convert between string->integer and integer->string, taking into
      account the current base format.
    */
    int stringToValue(const string& stringval)
        { return myParser->decipher_arg(stringval); }
    string valueToString(int value, BaseFormat outputBase = kBASE_DEFAULT);

    /** Convenience methods to convert to/from base values */
    static char* to_hex_4(int i)
    {
      static char out[2];
      sprintf(out, "%1x", i);
      return out;
    }
    static char* to_hex_8(int i)
    {
      static char out[3];
      sprintf(out, "%02x", i);
      return out;
    }
    static char* to_hex_16(int i)
    {
      static char out[5];
      sprintf(out, "%04x", i);
      return out;
    }
    static char* to_bin(int dec, int places, char *buf) {
      int bit = 1;
      buf[places] = '\0';
      while(--places >= 0) {
        if(dec & bit) buf[places] = '1';
        else          buf[places] = '0';
        bit <<= 1;
      }
      return buf;
    }
    static char* to_bin_8(int dec) {
      static char buf[9];
      return to_bin(dec, 8, buf);
    }
    static char* to_bin_16(int dec) {
      static char buf[17];
      return to_bin(dec, 16, buf);
    }
    static int conv_hex_digit(char d) {
      if(d >= '0' && d <= '9')      return d - '0';
      else if(d >= 'a' && d <= 'f') return d - 'a' + 10;
      else if(d >= 'A' && d <= 'F') return d - 'A' + 10;
      else                          return -1;
    }

    /* Convenience methods to get/set bit(s) in an 8-bit register */
    static uInt8 set_bit(uInt8 input, uInt8 bit, bool on)
    {
      if(on)
        return input | (1 << bit);
      else
        return input & ~(1 << bit);
    }
    static void set_bits(uInt8 reg, BoolArray& bits)
    {
      bits.clear();
      for(int i = 0; i < 8; ++i)
      {
        if(reg & (1<<(7-i)))
          bits.push_back(true);
        else
          bits.push_back(false);
      }
    }
    static uInt8 get_bits(BoolArray& bits)
    {
      uInt8 result = 0x0;
      for(int i = 0; i < 8; ++i)
        if(bits[i])
          result |= (1<<(7-i));
      return result;
    }

    /* Invert given input if it differs from its previous value */
    const string invIfChanged(int reg, int oldReg);

    /**
      This is used when we want the debugger from a class that can't
      receive the debugger object in any other way.

      It's basically a hack to prevent the need to pass debugger objects
      everywhere, but I feel it's better to place it here then in
      YaccParser (which technically isn't related to it at all).
    */
    inline static Debugger& debugger() { return *myStaticDebugger; }

    /**
      Get the dimensions of the various debugger dialog areas
      (takes mediasource into account)
    */
    GUI::Rect getDialogBounds() const;
    GUI::Rect getTiaBounds() const;
    GUI::Rect getRomBounds() const;
    GUI::Rect getStatusBounds() const;
    GUI::Rect getTabBounds() const;

    /* These are now exposed so Expressions can use them. */
    int peek(int addr) { return mySystem->peek(addr); }
    int dpeek(int addr) { return mySystem->peek(addr) | (mySystem->peek(addr+1) << 8); }

    void setBreakPoint(int bp, bool set);

    bool saveROM(const string& filename) const;

    bool setBank(int bank);
    bool patchROM(int addr, int value);

    /**
      Normally, accessing RAM or ROM during emulation can possibly trigger
      bankswitching.  However, when we're in the debugger, we'd like to
      inspect values without actually triggering bankswitches.  The
      read/write state must therefore be locked before accessing values,
      and unlocked for normal emulation to occur.
      (takes mediasource into account)
    */
    void lockBankswitchState();
    void unlockBankswitchState();

  private:
    /**
      Save state of each debugger subsystem.
    */
    void saveOldState(bool addrewind = true);

    /**
      Set initial state before entering the debugger.
    */
    void setStartState();

    /**
      Set final state before leaving the debugger.
    */
    void setQuitState();

    int step();
    int trace();
    void nextScanline(int lines);
    void nextFrame(int frames);
    bool rewindState();

    void toggleBreakPoint(int bp);

    bool breakPoint(int bp);
    void toggleReadTrap(int t);
    void toggleWriteTrap(int t);
    void toggleTrap(int t);
    bool readTrap(int t);
    bool writeTrap(int t);
    void clearAllTraps();

    void reloadROM();

    // Set a bunch of RAM locations at once
    const string setRAM(IntArray& args);

    void reset();
    void clearAllBreakPoints();

    PromptWidget *prompt() { return myPrompt; }

    void saveState(int state);
    void loadState(int state);

  private:
    Console* myConsole;
    System*  mySystem;

    DebuggerParser* myParser;
    CartDebug*      myCartDebug;
    CpuDebug*       myCpuDebug;
    RiotDebug*      myRiotDebug;
    TIADebug*       myTiaDebug;

    TiaInfoWidget*   myTiaInfo;
    TiaOutputWidget* myTiaOutput;
    TiaZoomWidget*   myTiaZoom;
    RomWidget*       myRom;
    EditTextWidget*  myMessage;

    PackedBitArray* myBreakPoints;
    PackedBitArray* myReadTraps;
    PackedBitArray* myWriteTraps;
    PromptWidget*   myPrompt;

    static Debugger* myStaticDebugger;

    FunctionMap functions;
    FunctionDefMap functionDefs;

    // Dimensions of the entire debugger window
    uInt32 myWidth;
    uInt32 myHeight;

    // Class holding all rewind state functionality in the debugger
    // Essentially, it's a modified circular array-based stack
    // that cleverly deals with allocation/deallocation of memory
    class RewindManager
    {
      public:
        RewindManager(OSystem& system, ButtonWidget& button);
        virtual ~RewindManager();

      public:
        bool addState();
        bool rewindState();
        bool isEmpty();
        void clear();

      private:
        enum { MAX_SIZE = 100 };
        OSystem& myOSystem;
        ButtonWidget& myRewindButton;
        Serializer* myStateList[MAX_SIZE];
        uInt32 mySize, myTop;
    };
    RewindManager* myRewindManager;
};

#endif

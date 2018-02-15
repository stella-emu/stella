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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef DEBUGGER_HXX
#define DEBUGGER_HXX

class OSystem;
class Console;
class EventHandler;
class TiaInfoWidget;
class TiaOutputWidget;
class TiaZoomWidget;
class EditTextWidget;
class RomWidget;
class Expression;
class PackedBitArray;
class TrapArray;
class PromptWidget;
class ButtonWidget;

class M6502;
class System;
class CartDebug;
class CpuDebug;
class RiotDebug;
class TIADebug;
class DebuggerParser;
class RewindManager;

#include <map>

#include "Base.hxx"
#include "DialogContainer.hxx"
#include "DebuggerDialog.hxx"
#include "FrameBufferConstants.hxx"
#include "bspf.hxx"

using FunctionMap = std::map<string, unique_ptr<Expression>>;
using FunctionDefMap = std::map<string, string>;


/**
  The base dialog for all debugging widgets in Stella.  Also acts as the parent
  for all debugging operations in Stella (parser, 6502 debugger, etc).

  @author  Stephen Anthony
*/
class Debugger : public DialogContainer
{
  // Make these friend classes, to ease communications with the debugger
  // Although it isn't enforced, these classes should use accessor methods
  // directly, and not touch the instance variables
  friend class DebuggerParser;
  friend class EventHandler;
  friend class M6502;

  public:
    /**
      Create a new debugger parent object
    */
    Debugger(OSystem& osystem, Console& console);
    virtual ~Debugger();

  public:
    /**
      Initialize the debugger dialog container.
    */
    void initialize();

    /**
      Initialize the video subsystem wrt this class.
    */
    FBInitStatus initializeVideo();

    /**
      Wrapper method for EventHandler::enterDebugMode() for those classes
      that don't have access to EventHandler.

      @param message  Message to display when entering debugger
      @param address  An address associated with the message
    */
    bool start(const string& message = "", int address = -1, bool read = true);
    bool startWithFatalError(const string& message = "");

    /**
      Wrapper method for EventHandler::leaveDebugMode() for those classes
      that don't have access to EventHandler.
    */
    void quit(bool exitrom);

    bool addFunction(const string& name, const string& def,
                     Expression* exp, bool builtin = false);
    bool isBuiltinFunction(const string& name);
    bool delFunction(const string& name);
    const Expression& getFunction(const string& name) const;

    const string& getFunctionDef(const string& name) const;
    const FunctionDefMap getFunctionDefMap() const;
    string builtinHelp() const;

    /**
      Methods used by the command parser for tab-completion
      In this case, return completions from the function list
    */
    void getCompletions(const char* in, StringList& list) const;

    /**
      The dialog/GUI associated with the debugger
    */
    Dialog& dialog() const { return *myDialog; }

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

    const GUI::Font& lfont() const      { return myDialog->lfont();     }
    const GUI::Font& nlfont() const     { return myDialog->nfont();     }
    DebuggerParser& parser() const      { return *myParser;             }
    PromptWidget& prompt() const        { return myDialog->prompt();    }
    RomWidget& rom() const              { return myDialog->rom();       }
    TiaOutputWidget& tiaOutput() const  { return myDialog->tiaOutput(); }

    PackedBitArray& breakPoints() const;
    TrapArray& readTraps() const;
    TrapArray& writeTraps() const;

    /**
      Run the debugger command and return the result.
    */
    const string run(const string& command);

    string autoExec(StringList* history);

    string showWatches();

    /**
      Convert between string->integer and integer->string, taking into
      account the current base format.
    */
    int stringToValue(const string& stringval);

    /* Convenience methods to get/set bit(s) in an 8-bit register */
    static uInt8 set_bit(uInt8 input, uInt8 bit, bool on)
    {
      if(on)
        return uInt8(input | (1 << bit));
      else
        return uInt8(input & ~(1 << bit));
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
    static uInt8 get_bits(const BoolArray& bits)
    {
      uInt8 result = 0x0;
      for(int i = 0; i < 8; ++i)
        if(bits[i])
          result |= (1<<(7-i));
      return result;
    }

    /** Invert given input if it differs from its previous value */
    const string invIfChanged(int reg, int oldReg);

    /**
      This is used when we want the debugger from a class that can't
      receive the debugger object in any other way.

      It's basically a hack to prevent the need to pass debugger objects
      everywhere, but I feel it's better to place it here then in
      YaccParser (which technically isn't related to it at all).
    */
    static Debugger& debugger() { return *myStaticDebugger; }

    /** Convenience methods to access peek/poke from System */
    uInt8 peek(uInt16 addr, uInt8 flags = 0);
    uInt16 dpeek(uInt16 addr, uInt8 flags = 0);
    void poke(uInt16 addr, uInt8 value, uInt8 flags = 0);

    /** Convenience method to access the 6502 from System */
    M6502& m6502() const;

    /** These are now exposed so Expressions can use them. */
    int peekAsInt(int addr, uInt8 flags = 0);
    int dpeekAsInt(int addr, uInt8 flags = 0);
    int getAccessFlags(uInt16 addr) const;
    void setAccessFlags(uInt16 addr, uInt8 flags);

    void setBreakPoint(uInt16 bp, bool set);
    uInt32 getBaseAddress(uInt32 addr, bool read);

    bool patchROM(uInt16 addr, uInt8 value);

    /**
      Normally, accessing RAM or ROM during emulation can possibly trigger
      bankswitching or other inadvertent changes.  However, when we're in
      the debugger, we'd like to inspect values without restriction.  The
      read/write state must therefore be locked before accessing values,
      and unlocked for normal emulation to occur.
    */
    void lockSystem();
    void unlockSystem();

    /**
      Answers whether the debugger can be exited.  Currently this only
      happens when no other dialogs are active.
    */
    bool canExit() const;

  private:
    /**
      Save state of each debugger subsystem and, by default, mark all
      pages as clean (ie, turn off the dirty flag).
    */
    void saveOldState(bool clearDirtyPages = true);

    /**
      Saves a rewind state with the given message.
    */
    void addState(string rewindMsg);

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
    uInt16 rewindStates(const uInt16 numStates, string& message);
    uInt16 unwindStates(const uInt16 numStates, string& message);

    void toggleBreakPoint(uInt16 bp);

    bool breakPoint(uInt16 bp);
    void addReadTrap(uInt16 t);
    void addWriteTrap(uInt16 t);
    void addTrap(uInt16 t);
    void removeReadTrap(uInt16 t);
    void removeWriteTrap(uInt16 t);
    void removeTrap(uInt16 t);
    bool readTrap(uInt16 t);
    bool writeTrap(uInt16 t);
    void clearAllTraps();

    // Set a bunch of RAM locations at once
    string setRAM(IntArray& args);

    void reset();
    void clearAllBreakPoints();

    void saveState(int state);
    void loadState(int state);

  private:
    Console& myConsole;
    System&  mySystem;

    DebuggerDialog* myDialog;
    unique_ptr<DebuggerParser> myParser;
    unique_ptr<CartDebug>      myCartDebug;
    unique_ptr<CpuDebug>       myCpuDebug;
    unique_ptr<RiotDebug>      myRiotDebug;
    unique_ptr<TIADebug>       myTiaDebug;

    static Debugger* myStaticDebugger;

    FunctionMap myFunctions;
    FunctionDefMap myFunctionDefs;

    // Dimensions of the entire debugger window
    uInt32 myWidth;
    uInt32 myHeight;

    // Various builtin functions and operations
    struct BuiltinFunction {
      string name, defn, help;
    };
    struct PseudoRegister {
      string name, help;
    };
    static const uInt32 NUM_BUILTIN_FUNCS = 18;
    static const uInt32 NUM_PSEUDO_REGS = 12;
    static BuiltinFunction ourBuiltinFunctions[NUM_BUILTIN_FUNCS];
    static PseudoRegister ourPseudoRegisters[NUM_PSEUDO_REGS];

  private:
    // rewind/unwind n states
    uInt16 windStates(uInt16 numStates, bool unwind, string& message);
    // update the rewind/unwind button state
    void updateRewindbuttons(const RewindManager& r);

    // Following constructors and assignment operators not supported
    Debugger() = delete;
    Debugger(const Debugger&) = delete;
    Debugger(Debugger&&) = delete;
    Debugger& operator=(const Debugger&) = delete;
    Debugger& operator=(Debugger&&) = delete;
};

#endif

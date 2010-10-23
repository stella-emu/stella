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

#ifndef DEBUGGER_PARSER_HXX
#define DEBUGGER_PARSER_HXX

#include <sstream>

class Debugger;
class FilesystemNode;
struct Command;

#include "bspf.hxx"
#include "Array.hxx"
#include "FrameBuffer.hxx"

typedef enum {
	kBASE_16,
	kBASE_16_4,
	kBASE_10,
	kBASE_2,
	kBASE_DEFAULT
} BaseFormat;

class DebuggerParser
{
  public:
    DebuggerParser(Debugger* debugger);
    ~DebuggerParser();

    /** Run the given command, and return the result */
    string run(const string& command);

    /** Execute parser commands given in 'file' */
    string exec(const FilesystemNode& file);

    /** Given a substring, determine matching substrings from the list
        of available commands.  Used in the debugger prompt for tab-completion */
    void getCompletions(const char* in, StringList& list) const;

    /** Evaluate the given expression using operators, current base, etc */
    int decipher_arg(const string &str);

    /** String representation of all watches currently defined */
    string showWatches();

    /** Get/set the number base when parsing numeric values */
    void setBase(BaseFormat base) { defaultBase = base; }
    BaseFormat base()             { return defaultBase; }

    static inline string red(const string& msg = "")
    {
      return char(kDbgChangedColor) + msg;
    }
    static inline string inverse(const string& msg = "")
    {
      // ASCII DEL char, decimal 127
      return "\177" + msg;
    }

  private:
    bool getArgs(const string& command, string& verb);
    bool validateArgs(int cmd);
    string eval();
    string trapStatus(int addr);
    bool saveScriptFile(string file);

  private:
    enum {
      kNumCommands   = 69,
      kMAX_ARG_TYPES = 10
    };

    // Constants for argument processing
    enum {
      kIN_COMMAND,
      kIN_SPACE,
      kIN_BRACE,
      kIN_ARG
    };

    typedef enum {
      kARG_WORD,        // single 16-bit value
      kARG_MULTI_WORD,  // multiple 16-bit values (must occur last)
      kARG_BYTE,        // single 8-bit value
      kARG_MULTI_BYTE,  // multiple 8-bit values (must occur last)
      kARG_BOOL,        // 0 or 1 only
      kARG_LABEL,       // label (need not be defined, treated as string)
      kARG_FILE,        // filename
      kARG_BASE_SPCL,   // base specifier: 2, 10, or 16 (or "bin" "dec" "hex")
      kARG_END_ARGS     // sentinel, occurs at end of list
    } parameters;

    // Pointer to DebuggerParser instance method, no args, returns void.
    typedef void (DebuggerParser::*METHOD)();

    struct Command {
      string cmdString;
      string description;
      bool parmsRequired;
      bool refreshRequired;
      parameters parms[kMAX_ARG_TYPES];
      METHOD executor;
    };

    // Pointer to our debugger object
    Debugger* debugger;

    // The results of the currently running command
    ostringstream commandResult;

    // Arguments in 'int' and 'string' format for the currently running command
    IntArray args;
    StringList argStrings;
    int argCount;

    BaseFormat defaultBase;
    StringList watches;

    // List of available command methods
    void executeA();
    void executeBank();
    void executeBase();
    void executeBreak();
    void executeBreakif();
    void executeC();
    void executeCheat();
    void executeClearbreaks();
    void executeClearconfig();
    void executeCleartraps();
    void executeClearwatches();
    void executeCls();
    void executeCode();
    void executeColortest();
    void executeD();
    void executeData();
    void executeDefine();
    void executeDelbreakif();
    void executeDelfunction();
    void executeDelwatch();
    void executeDisasm();
    void executeDump();
    void executeExec();
    void executeFrame();
    void executeFunction();
    void executeGfx();
    void executeHelp();
    void executeJump();
    void executeListbreaks();
    void executeListconfig();
    void executeListfunctions();
    void executeListtraps();
    void executeLoadconfig();
    void executeLoadstate();
    void executeLoadsym();
    void executeN();
    void executePc();
    void executePGfx();
    void executePrint();
    void executeRam();
    void executeReset();
    void executeRewind();
    void executeRiot();
    void executeRom();
    void executeRow();
    void executeRun();
    void executeRunTo();
    void executeRunToPc();
    void executeS();
    void executeSave();
    void executeSaveconfig();
    void executeSaverom();
    void executeSaveses();
    void executeSavestate();
    void executeScanline();
    void executeSkip();
    void executeStep();
    void executeTia();
    void executeTrace();
    void executeTrap();
    void executeTrapread();
    void executeTrapwrite();
    void executeType();
    void executeUndef();
    void executeV();
    void executeWatch();
    void executeX();
    void executeY();
    void executeZ();

    // List of commands available
    static Command commands[kNumCommands];
};

#endif

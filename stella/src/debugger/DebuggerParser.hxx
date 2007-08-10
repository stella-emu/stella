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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: DebuggerParser.hxx,v 1.49 2007-08-10 18:27:11 stephena Exp $
//============================================================================

#ifndef DEBUGGER_PARSER_HXX
#define DEBUGGER_PARSER_HXX

class Debugger;
struct Command;

#include "bspf.hxx"
#include "Array.hxx"

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
    string exec(const string& file, bool verbose = true);

    /** Given a substring, determine matching substrings from the list
        of available commands.  Used in the debugger prompt for tab-completion */  
    int countCompletions(const char *in);
    const char *getCompletions();
    const char *getCompletionPrefix();

    /** Evaluate the given expression using operators, current base, etc */
    int decipher_arg(const string &str);

    /** String representation of all watches currently defined */
    string showWatches();

    /** Get/set the number base when parsing numeric values */
    void setBase(BaseFormat base) { defaultBase = base; }
    BaseFormat base()             { return defaultBase; }

    static inline string red(const string& msg = "")
    {
      // This is TIA color 0x34. The octal value is 0x80+(0x34>>1).
      return "\232" + msg;
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
      kNumCommands   = 59,
      kMAX_ARG_TYPES = 10 // TODO: put in separate header file Command.hxx
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
    string commandResult;

    // Arguments in 'int' and 'string' format for the currently running command
    IntArray args;
    StringList argStrings;
    int argCount;

    BaseFormat defaultBase;
    StringList watches;

    // Used in 'tab-completion', holds list of commands related to a substring
    string completions;
    string compPrefix;

    // List of available command methods
    void executeA();
    void executeBank();
    void executeBase();
    void executeBreak();
    void executeBreakif();
    void executeC();
    void executeCheat();
    void executeClearbreaks();
    void executeCleartraps();
    void executeClearwatches();
    void executeColortest();
    void executeD();
    void executeDefine();
    void executeDelbreakif();
    void executeDelwatch();
    void executeDisasm();
    void executeDump();
    void executeExec();
    void executeFrame();
    void executeFunction();
    void executeHelp();
    void executeList();
    void executeListbreaks();
    void executeListtraps();
    void executeListwatches();
    void executeLoadlist();
    void executeLoadstate();
    void executeLoadsym();
    void executeN();
    void executePc();
    void executePrint();
    void executeRam();  // also implements 'poke' command
    void executeReload();
    void executeReset();
    void executeRiot();
    void executeRom();
    void executeRun();
    void executeRunTo();
    void executeS();
    void executeSave();
    void executeSaverom();
    void executeSaveses();
    void executeSavestate();
    void executeSavesym();
    void executeScanline();
    void executeStep();
    void executeTia();
    void executeTrace();
    void executeTrap();
    void executeTrapread();
    void executeTrapwrite();
    void executeUndef();
    void executeV();
    void executeWatch();
    void executeX();
    void executeY();
    void executeZ();
    void executeResolution();

    // List of commands available
    static Command commands[kNumCommands];
};

#endif

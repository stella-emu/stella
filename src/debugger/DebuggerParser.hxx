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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef DEBUGGER_PARSER_HXX
#define DEBUGGER_PARSER_HXX

#include <set>

class Debugger;
class Settings;
class FSNode;
struct Command;

#include "bspf.hxx"
#include "Device.hxx"
#include "FrameBufferConstants.hxx"

/**
  Interprets and dispatches commands typed at the debugger prompt.

  Each call to run() tokenizes the input, evaluates numeric arguments
  through YaccParser, validates them against the command's parameter table,
  and invokes the corresponding execute* method.  exec() extends this to
  read a sequence of commands from a script file.

  Shared mutable state (args, argStrings, argCount, commandResult, myCommand)
  is saved and restored on each run() invocation so that recursive calls
  originating from executeExec() are safe.
*/
class DebuggerParser
{
  public:
    DebuggerParser(Debugger& debugger, Settings& settings);
    ~DebuggerParser() = default;

    // Sentinel values returned by run() and checked by PromptWidget
    static constexpr string_view kExitDebugger{"_EXIT_DEBUGGER"};
    static constexpr string_view kNoPrompt{"_NO_PROMPT"};

    /** Run the given command, and return the result */
    string run(string_view command);

    /** Execute parser commands given in 'file' */
    string exec(const FSNode& file, StringList* history = nullptr);

    /** Given a substring, determine matching substrings from the list
        of available commands.  Used in the debugger prompt for tab-completion */
    static void getCompletions(string_view in, StringList& completions);

    /** Evaluate the given expression using operators, current base, etc */
    int decipherArg(string_view str);

    /** String representation of all watches currently defined */
    string showWatches();

    /** Prefix msg with the PromptWidget color-red control byte */
    static string red(string_view msg = {}) {
      return static_cast<char>(kDbgColorRed & 0xff) + string{msg};
    }
    /** Prefix msg with the PromptWidget inverse-video control byte (ASCII DEL, 0x7f) */
    static string inverse(string_view msg = {}) {
      return "\177" + string{msg};
    }

  private:
    /** Tokenize command into verb (returned via ref) and argStrings/argCount members.
        Tokens separated by spaces; {braces} allow spaces within a single token. */
    void getArgs(string_view command, string& verb);

    /** Validate argCount and arg values against commands[cmd].parms.
        Writes an error to commandResult and returns false on failure. */
    bool validateArgs(int cmd);

    /** Format all current args in hex, binary, and decimal with label lookups.
        Operates on the args/argStrings/argCount member state set by getArgs(). */
    string eval();

    /** Join argStrings[from .. end) with spaces.
        Used by skipEval executors that need to re-parse the full expression. */
    string buildExprStr(uInt32 from = 0, uInt32 end = ~0U) const;

    /** Serialize the current session (functions, watches, breakpoints, traps,
        timers) to a .script file, creating or overwriting it. */
    string saveScriptFile(string file);

    /** Write out to node; append " to <path>" or a red error to result. */
    static void saveDump(const FSNode& node, const std::ostringstream& out,
                         std::ostringstream& result);

    string_view cartName() const;

  private:
    // Tokenizer state for getArgs(): between tokens, inside {braces}, inside a token
    enum class ParseState: uInt8 { IN_SPACE, IN_BRACE, IN_ARG };

    enum class Parameters: uInt8 {
      ARG_WORD,       // single 16-bit value
      ARG_DWORD,      // single 32-bit value
      ARG_BYTE,       // single 8-bit value
      ARG_MULTI_BYTE, // multiple 8-bit values (must occur last)
      ARG_BOOL,       // 0 or 1 only
      ARG_LABEL,      // label (need not be defined, treated as string)
      ARG_FILE,       // filename
      ARG_BASE_SPCL,  // base specifier: 2, 10, or 16 (or "bin" "dec" "hex")
      ARG_END_ARGS    // sentinel, occurs at end of list
    };

    // List of commands available
    struct Command {
      // Name typed at the prompt (e.g. "breakIf")
      string_view cmdString;
      // One-line description shown in the full help listing
      string_view description;
      // Extended help shown for "help <command>"; may be empty
      string_view extendedDesc;
      // Usage example shown in help and appended to argument errors; may be empty
      string_view example;
      // True when at least one argument is required
      bool parmsRequired{false};
      // True when the debugger UI must reload its config before and after execution
      bool refreshRequired{false};
      // True when the executor re-parses all args itself via buildExprStr();
      // suppresses the normal YaccParser evaluation pass in evalArgs()
      bool skipEval{false};
      // Expected argument types, in order, terminated by ARG_END_ARGS;
      // MULTI_* entries repeat for all remaining arguments
      std::array<Parameters, 10> parms;
      // Member function that carries out the command
      void (DebuggerParser::*executor)();
    };
    using CommandArray = std::array<Command, 112>;
    static const CommandArray commands;

    /** Evaluate each argString through YaccParser and store results in
        args[]. No-op when cmd.skipEval is true (executor will re-parse via
        buildExprStr). */
    void evalArgs(const Command& cmd);

    Debugger& debugger;
    Settings& settings;

    // Output buffer written by execute* methods; returned as a string by run()
    std::ostringstream commandResult;

    // Index into commands[] for the currently executing command
    int myCommand{0};

    // Evaluated (int) and raw (string) forms of the current command's arguments
    IntArray args;
    StringList argStrings;
    uInt32 argCount{0};

    // Nesting depth of exec() calls; nonzero while a script is running
    uInt32 execDepth{0};
    // Filename prefix used by saveSnap/dump during script execution
    string execPrefix;

    StringList myWatches;

    /** List either plain traps (listCond=false) or conditional trapIfs
        (listCond=true) */
    void listTraps(bool listCond);

    /** Return a parenthesized label annotation for the address range,
        or an empty string if neither endpoint has a user-defined label */
    string trapStatus(uInt32 begin, uInt32 end, bool read, bool write) const;

    /** Emit one timer row into commandResult in fixed-width tabular format */
    void printTimer(uInt32 idx, bool showHeader = true);

    /** Emit the full timer table (header + all rows) into commandResult */
    void listTimers();

    /** Return a sequence of "timer …" command strings that recreate all
        current timers; suitable for embedding in a saved script file */
    string getTimerCmds();

    /** Clear commandResult, write the error message (red), and append the
        command's example string if one is defined */
    void outputCommandError(string_view errorMsg, int command);

    /** Shared implementation for the disassembly-annotation commands
        (code, data, gfx, col, …); applies the given access type to args[0..1] */
    void executeDirective(Device::AccessType type);

    // List of available command methods
    void executeA();
    void executeAud();
    void executeAutoSave();
    void executeBase();
    void executeBCol();
    void executeBreak();
    void executeBreakIf();
    void executeBreakLabel();
    void executeC();
    void executeCheat();
    void executeClearBreaks();
    void executeClearConfig();
    void executeClearHistory();
    void executeClearSaveStateIfs();
    void executeClearTimers();
    void executeClearTraps();
    void executeClearWatches();
    void executeCls();
    void executeCode();
    void executeCol();
    void executeColorTest();
    void executeD();
    void executeData();
    void executeDebugColors();
    void executeDefine();
    void executeDelBreakIf();
    void executeDelFunction();
    void executeDelSaveStateIf();
    void executeDelTimer();
    void executeDelTrap();
    void executeDelWatch();
    void executeDisAsm();
    void executeDump();
    void executeExec();
    void executeExitRom();
    void executeFrame();
    void executeFunction();
    void executeGfx();
    void executeHelp();
    void executeJoy0Up();
    void executeJoy0Down();
    void executeJoy0Left();
    void executeJoy0Right();
    void executeJoy0Fire();
    void executeJoy1Up();
    void executeJoy1Down();
    void executeJoy1Left();
    void executeJoy1Right();
    void executeJoy1Fire();
    void executeJump();
    void executeListBreaks();
    void executeListConfig();
    void executeListFunctions();
    void executeListSaveStateIfs();
    void executeListTimers();
    void executeListTraps();
    void executeLoadAllStates();
    void executeLoadConfig();
    void executeLoadState();
    void executeLogBreaks();
    void executeLogExec();
    void executeLogTrace();
    void executeN();
    void executePalette();
    void executePc();
    void executePCol();
    void executePGfx();
    void executePrint();
    void executePrintTimer();
    void executeRam();
    void executeReset();
    void executeResetTimers();
    void executeRewind();
    void executeRiot();
    void executeRom();
    void executeRow();
    void executeRun();
    void executeRunTo();
    void executeRunToPc();
    void executeS();
    void executeSave();
    void executeSaveAccess();
    void executeSaveAllStates();
    void executeSaveConfig();
    void executeSaveDisassembly();
    void executeSaveRom();
    void executeSaveSes();
    void executeSaveSnap();
    void executeSaveState();
    void executeSaveStateIf();
    void executeScanLine();
    void executeStep();
    void executeStepWhile();
    void executeSwchb();
    void executeTia();
    void executeTimer();
    void executeTrace();
    void executeTrap();
    void executeTrapIf();
    void executeTrapRead();
    void executeTrapReadIf();
    void executeTrapWrite();
    void executeTrapWriteIf();
    void executeTraps(bool read, bool write, string_view command,
                      bool cond = false);
    /** Apply or remove read/write traps for [begin, end] and all their mirrors
        across the full 64K address space */
    void executeTrapRW(uInt32 begin, uInt32 end, bool read, bool write,
                       bool add = true);
    void executeType();
    void executeUHex();
    void executeUndef();
    void executeUnwind();
    void executeV();
    void executeWatch();
    void executeWinds(bool unwind);
    void executeX();
    void executeY();
    void executeZ();

  private:
    // Following constructors and assignment operators not supported
    DebuggerParser() = delete;
    DebuggerParser(const DebuggerParser&) = delete;
    DebuggerParser(DebuggerParser&&) = delete;
    DebuggerParser& operator=(const DebuggerParser&) = delete;
    DebuggerParser& operator=(DebuggerParser&&) = delete;
};

#endif  // DEBUGGER_PARSER_HXX

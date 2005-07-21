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
// $Id: DebuggerParser.hxx,v 1.37 2005-07-21 03:26:58 urchlay Exp $
//============================================================================

#ifndef DEBUGGER_PARSER_HXX
#define DEBUGGER_PARSER_HXX

class Debugger;
struct Command;

#include "bspf.hxx"
#include "Array.hxx"

typedef enum {
	kBASE_16,
	kBASE_10,
	kBASE_2,
	kBASE_DEFAULT
} BaseFormat;

class DebuggerParser
{
	public:
		DebuggerParser(Debugger* debugger);
		~DebuggerParser();

		string run(const string& command);
		int decipher_arg(const string &str);

		void setBase(BaseFormat base) { defaultBase = base; }
		BaseFormat base()             { return defaultBase; }
		string showWatches();
		string addWatch(string watch);
		string delWatch(int which);
		void delAllWatches();
		int countCompletions(const char *in);
		const char *getCompletions();
		const char *getCompletionPrefix();
		string exec(const string& cmd, bool verbose=true);

		static inline string red(string msg ="") {
			// This is TIA color 0x34. The octal value is 0x80+(0x34>>1).
			return "\232" + msg;
		}

		static inline string inverse(string msg ="") {
         // ASCII DEL char, decimal 127
			return "\177" + msg;
		}

	private:
		bool getArgs(const string& command);
		bool validateArgs(int cmd);
		int conv_hex_digit(char d);
		bool subStringMatch(const string& needle, const string& haystack);
		string disasm();
		string listBreaks();
		string listTraps();
		string eval();
		string dump();
		string trapStatus(int addr);

		Debugger* debugger;

		bool done;

		string verb;
		string commandResult;

		IntArray args;
		StringList argStrings;
		int argCount;

		BaseFormat defaultBase;
		StringList watches;
		static Command commands[];

      string completions;
      string compPrefix;

		void executeA();
		void executeBank();
		void executeBase();
		void executeBreak();
		void executeBreakif();
		void executeC();
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
		void executeHeight();
		void executeHelp();
		void executeList();
		void executeListbreaks();
		void executeListtraps();
		void executeListwatches();
		void executeLoadlist();
		void executeLoadsym();
		void executeLoadstate();
		void executeN();
		void executePc();
		void executePrint();
		void executeRam();
		void executeReload();
		void executeReset();
		void executeRiot();
		void executeRom();
		void executeRun();
		void executeRunTo();
		void executeS();
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
};

// TODO: put in separate header file Command.hxx
#define kMAX_ARG_TYPES 10

// These next two deserve English explanations:

// pointer to DebuggerParser instance method, no args, returns void.
typedef void (DebuggerParser::*METHOD)();

// call the pointed-to method on the this object. Whew.
#define CALL_METHOD(method) ( (this->*method)() )

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

struct Command {
	string cmdString;
	string description;
	bool parmsRequired;
	parameters parms[kMAX_ARG_TYPES];
	METHOD executor;
};


#endif

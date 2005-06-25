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
// $Id: DebuggerParser.hxx,v 1.17 2005-06-25 06:27:01 urchlay Exp $
//============================================================================

#ifndef DEBUGGER_PARSER_HXX
#define DEBUGGER_PARSER_HXX

class Debugger;
struct Command;

#include "bspf.hxx"
#include "EquateList.hxx"
#include "Array.hxx"

typedef enum {
	kBASE_16,
	kBASE_10,
	kBASE_2,
	kBASE_DEFAULT
} BaseFormat;

typedef GUI::Array<int> IntArray;
typedef GUI::Array<string> StringArray;

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


	private:
		bool getArgs(const string& command);
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

		IntArray args;
		StringArray argStrings;
		int argCount;

		BaseFormat defaultBase;
		StringArray watches;
		static Command commands[];

		void executeA();
		void executeBase();
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
} argType;

struct Command {
	string cmdString;
	string description;
	argType args[kMAX_ARG_TYPES];
	METHOD executor;
};


#endif

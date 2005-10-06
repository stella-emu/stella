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
// $Id: DebuggerParser.cxx,v 1.84 2005-10-06 17:28:54 stephena Exp $
//============================================================================

#include "bspf.hxx"
#include <iostream>
#include <fstream>
#include "Debugger.hxx"
#include "CpuDebug.hxx"
#include "DebuggerParser.hxx"
#include "YaccParser.hxx"
#include "M6502.hxx"
#include "Expression.hxx"
#include "CheetahCheat.hxx"
#include "Cheat.hxx"
#include "RomWidget.hxx"

#include "DebuggerParser.hxx"

Command DebuggerParser::commands[] = {
	{
		"a",
		"Set Accumulator to value xx",
		true,
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeA
	},

	{
		"bank",
		"Show # of banks (with no args), Switch to bank (with 1 arg)",
		false,
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeBank
	},

	{
		"base",
		"Set default base (hex, dec, or bin)",
		true,
		true,
		{ kARG_BASE_SPCL, kARG_END_ARGS },
		&DebuggerParser::executeBase
	},

	{
		"break",
		"Set/clear breakpoint at address (default: current pc)",
		false,
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeBreak
	},

	{
		"breakif",
		"Set breakpoint on condition",
		true,
		false,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeBreakif
	},

	{
		"c",
		"Carry Flag: set (to 0 or 1), or toggle (no arg)",
		false,
		true,
		{ kARG_BOOL, kARG_END_ARGS },
		&DebuggerParser::executeC
	},

	{
		"cheetah",
		"Use Cheetah cheat code (see http://members.cox.net/rcolbert/)",
		false,
		false,
		// lame: accept 0-4 args instead of inventing a kARG_MULTI_LABEL type
		{ kARG_LABEL, kARG_LABEL, kARG_LABEL, kARG_LABEL, kARG_END_ARGS },
		&DebuggerParser::executeCheetah
	},

	{
		"clearbreaks",
		"Clear all breakpoints",
		false,
		true,
		{ kARG_END_ARGS },
		&DebuggerParser::executeClearbreaks
	},

	{
		"cleartraps",
		"Clear all traps",
		false,
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeCleartraps
	},

	{
		"clearwatches",
		"Clear all watches",
		false,
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeClearwatches
	},

	{
		"colortest",
		"Color Test",
		true,
		false,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeColortest
	},

	{
		"d",
		"Decimal Flag: set (to 0 or 1), or toggle (no arg)",
		false,
		true,
		{ kARG_BOOL, kARG_END_ARGS },
		&DebuggerParser::executeD
	},

	{
		"define",
		"Define label",
		true,
		true,
		{ kARG_LABEL, kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeDefine
	},

	{
		"delbreakif",
		"Delete conditional break created with breakif",
		true,
		false,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeDelbreakif
	},

	{
		"delwatch",
		"Delete watch",
		true,
		false,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeDelwatch
	},

	{
		"disasm",
		"Disassemble from address (default=pc)",
		false,
		false,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeDisasm
	},

	{
		"dump",
		"Dump 128 bytes of memory at address",
		true,
		false,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeDump
	},

	{
		"exec",
		"Execute script file",
		true,
		true,
		{ kARG_FILE, kARG_END_ARGS },
		&DebuggerParser::executeExec
	},

	{
		"frame",
		"Advance emulation by xx frames (default=1)",
		false,
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeFrame
	},

	{
		"function",
		"Define expression as a function for later use",
		false,
		false,
		{ kARG_LABEL, kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeFunction
	},

	{
		"height",
		"Change height of debugger window",
		true,
		false,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeHeight
	},

	{
		"help",
		"This cruft",
		false,
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeHelp
	},

	{
		"list",
		"List source (if loaded with loadlst)",
		false,
		false,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeList
	},

	{
		"listbreaks",
		"List breakpoints",
		false,
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeListbreaks
	},

	{
		"listtraps",
		"List traps",
		false,
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeListtraps
	},

	{
		"listwatches",
		"List watches",
		false,
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeListwatches
	},

	{
		"loadstate",
		"Load emulator state (0-9)",
		true,
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeLoadstate
	},

	{
		"loadlist",
		"Load DASM listing file",
		true,
		true,
		{ kARG_FILE, kARG_END_ARGS },
		&DebuggerParser::executeLoadlist
	},

	{
		"loadsym",
		"Load symbol file",
		true,
		true,
		{ kARG_FILE, kARG_END_ARGS },
		&DebuggerParser::executeLoadsym
	},

	{
		"n",
		"Negative Flag: set (to 0 or 1), or toggle (no arg)",
		false,
		true,
		{ kARG_BOOL, kARG_END_ARGS },
		&DebuggerParser::executeN
	},

	{
		"pc",
		"Set Program Counter to address",
		true,
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executePc
	},

	{
		"poke",
		"Set address to value. Can give multiple values (for address+1, etc)",
		true,
		true,
		{ kARG_WORD, kARG_MULTI_BYTE },
		&DebuggerParser::executeRam
	},

	{
		"print",
		"Evaluate and print expression in hex/dec/binary",
		true,
		false,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executePrint
	},

	{
		"ram",
		"Show RAM contents (no args), or set address xx to value yy",
		false,
		true,
		{ kARG_WORD, kARG_MULTI_BYTE },
		&DebuggerParser::executeRam
	},

	{
		"reload",
		"Reload ROM and symbol file",
		false,
		true,
		{ kARG_END_ARGS },
		&DebuggerParser::executeReload
	},

	{
		"reset",
		"Reset 6507 to init vector (does not reset TIA, RIOT)",
		false,
		true,
		{ kARG_END_ARGS },
		&DebuggerParser::executeReset
	},

	{
		"riot",
		"Show RIOT timer/input status",
		false,
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeRiot
	},

	{
		"rom",
		"Change ROM contents",
		true,
		true,
		{ kARG_WORD, kARG_MULTI_BYTE },
		&DebuggerParser::executeRom
	},

	{
		"run",
		"Exit debugger, return to emulator",
		false,
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeRun
	},

	{
		"runto",
		"Run until first occurrence of string in disassembly",
		false,
		true,
		{ kARG_LABEL, kARG_END_ARGS },
		&DebuggerParser::executeRunTo
	},

	{
		"s",
		"Set Stack Pointer to value xx",
		true,
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeS
	},

	{
		"save",
		"Save breaks, watches, traps as a .stella script file",
		true,
		false,
		{ kARG_FILE, kARG_END_ARGS },
		&DebuggerParser::executeSave
	},

	{
		"saverom",
		"Save (possibly patched) ROM to file",
		true,
		false,
		{ kARG_FILE, kARG_END_ARGS },
		&DebuggerParser::executeSaverom
	},

	{
		"saveses",
		"Save console session to file",
		true,
		false,
		{ kARG_FILE, kARG_END_ARGS },
		&DebuggerParser::executeSaveses
	},

	{
		"savestate",
		"Save emulator state (valid args 0-9)",
		true,
		false,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeSavestate
	},

	{
		"savesym",
		"Save symbols to file",
		true,
		false,
		{ kARG_FILE, kARG_END_ARGS },
		&DebuggerParser::executeSavesym
	},

	{
		"scanline",
		"Advance emulation by xx scanlines (default=1)",
		false,
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeScanline
	},

	{
		"step",
		"Single step CPU (optionally, with count)",
		false,
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeStep
	},

	{
		"tia",
		"Show TIA state (NOT FINISHED YET)",
		false,
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeTia
	},

	{
		"trace",
		"Single step CPU (optionally, with count), subroutines count as one instruction",
		false,
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeTrace
	},

	{
		"trap",
		"Trap read and write accesses to address",
		true,
		false,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeTrap
	},

	{
		"trapread",
		"Trap read accesses to address",
		true,
		false,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeTrapread
	},

	{
		"trapwrite",
		"Trap write accesses to address",
		true,
		false,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeTrapwrite
	},

	{
		"undef",
		"Undefine label (if defined)",
		true,
		true,
		{ kARG_LABEL, kARG_END_ARGS },
		&DebuggerParser::executeUndef
	},

	{
		"v",
		"Overflow Flag: set (to 0 or 1), or toggle (no arg)",
		false,
		true,
		{ kARG_BOOL, kARG_END_ARGS },
		&DebuggerParser::executeV
	},

	{
		"watch",
		"Print contents of address before every prompt",
		true,
		false,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeWatch
	},

	{
		"x",
		"Set X Register to value xx",
		true,
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeX
	},

	{
		"y",
		"Set Y Register to value xx",
		true,
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeY
	},

	{
		"z",
		"Zero Flag: set (to 0 or 1), or toggle (no arg)",
		false,
		true,
		{ kARG_BOOL, kARG_END_ARGS },
		&DebuggerParser::executeZ
	},

	{
		"",
		"",
		false,
		false,
		{ kARG_END_ARGS },
		NULL
	}

};

// Constants for argument processing
enum {
  kIN_COMMAND,
  kIN_SPACE,
  kIN_BRACE,
  kIN_ARG
};

DebuggerParser::DebuggerParser(Debugger* d)
  	: debugger(d)
{
	done = false;
	defaultBase = kBASE_16;
}

DebuggerParser::~DebuggerParser() {
	args.clear();
	argStrings.clear();
	watches.clear();
}

int DebuggerParser::conv_hex_digit(char d) {
	if(d >= '0' && d <= '9')
		return d - '0';
	else if(d >= 'a' && d <= 'f')
		return d - 'a' + 10;
	else if(d >= 'A' && d <= 'F')
		return d - 'A' + 10;
	else return -1;
}

// Evaluate expression. Expressions always evaluate to a 16-bit value if
// they're valid, or -1 if they're not.

// decipher_arg may be called by the GUI as needed. It is also called
// internally by DebuggerParser::run()
int DebuggerParser::decipher_arg(const string &str) {
	bool derefByte=false, derefWord=false, lobyte=false, hibyte=false, bin=false, dec=false;
	int result;
	string arg = str;

	if(defaultBase == kBASE_2) {
		bin=true; dec=false;
	} else if(defaultBase == kBASE_10) {
		bin=false; dec=true;
	} else {
		bin=false; dec=false;
	}

	if(arg.substr(0, 1) == "*") {
		derefByte = true;
		arg.erase(0, 1);
	} else if(arg.substr(0, 1) == "@") {
		derefWord = true;
		arg.erase(0, 1);
	}

	if(arg.substr(0, 1) == "<") {
		lobyte = true;
		arg.erase(0, 1);
	} else if(arg.substr(0, 1) == ">") {
		hibyte = true;
		arg.erase(0, 1);
	}

	if(arg.substr(0, 1) == "%") {
		bin = true;
		dec = false;
		arg.erase(0, 1);
	} else if(arg.substr(0, 1) == "#") {
		dec = true;
		bin = false;
		arg.erase(0, 1);
	} else if(arg.substr(0, 1) == "$") {
		dec = false;
		bin = false;
		arg.erase(0, 1);
	}

	// sanity check mutually exclusive options:
	if(derefByte && derefWord) return -1;
	if(lobyte && hibyte) return -1;
	if(bin && dec) return -1;

	// Special cases (registers):
	CpuState& state = (CpuState&) debugger->cpuDebug().getState();
	if(arg == "a") result = state.A;
	else if(arg == "x") result = state.X;
	else if(arg == "y") result = state.Y;
	else if(arg == "p") result = state.PS;
	else if(arg == "s") result = state.SP;
	else if(arg == "pc" || arg == ".") result = state.PC;
	else { // Not a special, must be a regular arg: check for label first
		const char *a = arg.c_str();
		result = debugger->equateList->getAddress(arg);

		if(result < 0) { // if not label, must be a number
			if(bin) { // treat as binary
				result = 0;
				while(*a != '\0') {
					result <<= 1;
					switch(*a++) {
						case '1':
							result++;
							break;

						case '0':
							break;

						default:
							return -1;
					}
				}
			} else if(dec) {
				result = 0;
				while(*a != '\0') {
					int digit = (*a++) - '0';
					if(digit < 0 || digit > 9)
						return -1;

					result = (result * 10) + digit;
				}
			} else { // must be hex.
				result = 0;
				while(*a != '\0') {
					int hex = conv_hex_digit(*a++);
					if(hex < 0)
						return -1;

					result = (result << 4) + hex;
				}
			}
		}
	}

	if(lobyte) result &= 0xff;
	else if(hibyte) result = (result >> 8) & 0xff;

	// dereference if we're supposed to:
	if(derefByte) result = debugger->peek(result);
	if(derefWord) result = debugger->dpeek(result);

	return result;
}

bool DebuggerParser::getArgs(const string& command) {
	int state = kIN_COMMAND;
	string curArg = "";
	verb = "";
	const char *c = command.c_str();

	argStrings.clear();
	args.clear();

	// cerr << "Parsing \"" << command << "\"" << endl;

	// First, pick apart string into space-separated tokens.
	// The first token is the command verb, the rest go in an array
	do {
		// cerr << "State " << state << ", *c '" << *c << "'" << endl;
		switch(state) {
			case kIN_COMMAND:
				if(*c == ' ')
					state = kIN_SPACE;
				else
					verb += *c;
				break;

			case kIN_SPACE:
				if(*c == '{')
					state = kIN_BRACE;
				else if(*c != ' ') {
					state = kIN_ARG;
					curArg += *c;
				}
				break;

			case kIN_BRACE:
				if(*c == '}' || *c == '\0') {
					state = kIN_SPACE;
					argStrings.push_back(curArg);
					//	cerr << "{" << curArg << "}" << endl;
					curArg = "";
				} else {
					curArg += *c;
				}
				break;

			case kIN_ARG:
				if(*c == ' ' || *c == '\0') {
					state = kIN_SPACE;
					argStrings.push_back(curArg);
					curArg = "";
				} else {
					curArg += *c;
				}
				break;
		} // switch(state)
	} while(*c++ != '\0');

	argCount = argStrings.size();

	/*
	// Now decipher each argument, in turn.
	for(int i=0; i<argCount; i++) {
		int temp = decipher_arg(argStrings[i]);
		args.push_back(temp); // value maybe -1, if not expression argument
		                      // (validate_args will decide whether that's OK, not us.)
	}
	*/

	for(int i=0; i<argCount; i++) {
		int err = YaccParser::parse(argStrings[i].c_str());
		if(err) {
			args.push_back(-1);
		} else {
			Expression *e = YaccParser::getResult();
			args.push_back( e->evaluate() );
			delete e;
		}
	}

	return true;
}

bool DebuggerParser::subStringMatch(const string& needle, const string& haystack) {
	const char *hs = haystack.c_str();
	const char *n = needle.c_str();

	if(STR_N_CASE_CMP(n, hs, strlen(n)) == 0)
		return true;

	return false;
}

string DebuggerParser::listBreaks() {
	char buf[255];
	int count = 0;
	string ret = "";

	for(unsigned int i=0; i<0x10000; i++) {
		if(debugger->breakPoints->isSet(i)) {
			sprintf(buf, "%s ", debugger->equateList->getFormatted(i, 4));
			ret += buf;
			if(! (++count % 8) ) ret += "\n";
		}
	}
	/*
	if(count)
		return ret;
	else
		return "no breakpoints set";
		*/
	if(count)
		ret = "breaks:\n" + ret;

	StringList conds = debugger->cpuDebug().m6502().getCondBreakNames();
	if(conds.size() > 0) {
		ret += "\nbreakifs:\n";
		for(unsigned int i=0; i<conds.size(); i++) {
			ret += debugger->valueToString(i);
			ret += ": ";
			ret += conds[i];
			if(i != (conds.size() - 1)) ret += "\n";
		}
	}

	if(ret == "")
		return "no breakpoints set";
	else
		return ret;
}

string DebuggerParser::listTraps() {
	int count = 0;
	string ret;

	for(unsigned int i=0; i<0x10000; i++) {
		if(debugger->readTrap(i) || debugger->writeTrap(i)) {
			ret += trapStatus(i);
			ret += "\n";
			count++;
		}
	}

	if(count)
		return ret;
	else
		return "no traps set";
}

string DebuggerParser::disasm() {
	int start, lines = 20;

	if(argCount == 0) {
		start = debugger->cpuDebug().pc();
	} else if(argCount == 1) {
		start = args[0];
	} else if(argCount == 2) {
		start = args[0];
		lines = args[1];
	} else {
		return "wrong number of arguments";
	}

	return debugger->disassemble(start, lines);
}

string DebuggerParser::dump() {
	string ret;
	for(int i=0; i<8; i++) {
		int start = args[0] + i*16;
		ret += debugger->valueToString(start);
		ret += ": ";
		for(int j=0; j<16; j++) {
			ret += debugger->valueToString( debugger->peek(start+j) );
			ret += " ";
			if(j == 7) ret += "- ";
		}
		if(i != 7) ret += "\n";
	}
	return ret;
}

string DebuggerParser::eval() {
	char buf[50];
	string ret;
	for(int i=0; i<argCount; i++) {
		string label = debugger->equates()->getLabel(args[i]);
		if(label != "") {
			ret += label;
			ret += ": ";
		}
		ret += "$";
		if(args[i] < 0x100) {
			ret += Debugger::to_hex_8(args[i]);
			ret += " %";
			ret += Debugger::to_bin_8(args[i]);
		} else {
			ret += Debugger::to_hex_16(args[i]);
			ret += " %";
			ret += Debugger::to_bin_16(args[i]);
		}
		sprintf(buf, " #%d", args[i]);
		ret += buf;
		if(i != argCount - 1) ret += "\n";
	}
	return ret;
}

string DebuggerParser::showWatches() {
	string ret;
	char buf[10];

	for(unsigned int i=0; i<watches.size(); i++) {
		if(watches[i] != "") {
			// Clear the args, since we're going to pass them to eval()
			argStrings.clear();
			args.clear();

			sprintf(buf, "%d", i+1);
			argCount = 1;
			argStrings.push_back(watches[i]);
			args.push_back(decipher_arg(argStrings[0]));
			if(args[0] < 0) {
				ret += "BAD WATCH ";
				ret += buf;
				ret += ": " + argStrings[0] + "\n";
			} else {
				ret += " watch #";
				ret += buf;
				ret += " (" + argStrings[0] + ") -> " + eval() + "\n";
			}
		}
	}
	return ret;
}

string DebuggerParser::addWatch(string watch) {
	watches.push_back(watch);
	return "Added watch";
}

void DebuggerParser::delAllWatches() {
	watches.clear();
}

string DebuggerParser::delWatch(int which) {
	which--;
	if(which < 0 || which >= (int)watches.size())
		return "no such watch";
	else
		watches.remove_at(which);

	return "removed watch";
}

string DebuggerParser::trapStatus(int addr) {
	string result;
	result += debugger->valueToString(addr);
	result += ": ";
	bool r = debugger->readTrap(addr);
	bool w = debugger->writeTrap(addr);
	if(r && w)
		result += "read|write";
	else if(r)
		result += "read";
	else if(w)
		result += "     write";
	else
		result += "   none   ";

	string l = debugger->equateList->getLabel(addr);
	if(l != "") {
		result += "  (";
		result += l;
		result += ")";
	}

	return result;
}

bool DebuggerParser::validateArgs(int cmd)
{
  // cerr << "entering validateArgs(" << cmd << ")" << endl;
  bool required = commands[cmd].parmsRequired;
  parameters *p = commands[cmd].parms;

  if(argCount == 0)
  {
    if(required)
    {
      commandResult = red("missing required argument(s)");
      return false; // needed args. didn't get 'em.
    }
    else
      return true;  // no args needed, no args got
  }

  // Figure out how many arguments are required by the command
  int count = 0, argRequiredCount = 0;
  while(*p != kARG_END_ARGS && *p != kARG_MULTI_BYTE)
  {
    count++;
    *p++;
  }

  // Evil hack: some commands intentionally take multiple arguments
  // In this case, the required number of arguments is unbounded
  argRequiredCount = (*p == kARG_END_ARGS) ? count : argCount;

  p = commands[cmd].parms;
  int curCount = 0;

  do {
    if(curCount >= argCount)
      break;

    int curArgInt     = args[curCount];
    string& curArgStr = argStrings[curCount];

    switch(*p)
    {
      case kARG_WORD:
        if(curArgInt < 0 || curArgInt > 0xffff)
        {
          commandResult = red("invalid word argument (must be 0-$ffff)");
          return false;
        }
        break;

      case kARG_BYTE:
        if(curArgInt < 0 || curArgInt > 0xff)
        {
          commandResult = red("invalid byte argument (must be 0-$ff)");
          return false;
        }
        break;

      case kARG_BOOL:
        if(curArgInt != 0 && curArgInt != 1)
        {
          commandResult = red("invalid boolean argument (must be 0 or 1)");
          return false;
        }
        break;

      case kARG_BASE_SPCL:
        if(curArgInt != 2 && curArgInt != 10 && curArgInt != 16
           && curArgStr != "hex" && curArgStr != "dec" && curArgStr != "bin")
        {
          commandResult = red("invalid base (must be #2, #10, #16, \"bin\", \"dec\", or \"hex\")");
          return false;
        }
        break;

      case kARG_LABEL:
      case kARG_FILE:
        break; // TODO: validate these (for now any string's allowed)

      case kARG_MULTI_BYTE:
      case kARG_MULTI_WORD:
        break; // FIXME: validate these (for now, any number's allowed)

      case kARG_END_ARGS:
        break;
    }
    curCount++;
    *p++;

  } while(*p != kARG_END_ARGS && curCount < argRequiredCount);

/*
cerr << "curCount         = " << curCount << endl
     << "argRequiredCount = " << argRequiredCount << endl
     << "*p               = " << *p << endl << endl;
*/

  if(curCount < argRequiredCount)
  {
    commandResult = red("missing required argument(s)");
    return false;
  }
  else if(argCount > curCount)
  {
    commandResult = red("too many arguments");
    return false;
  }

  return true;
}

// main entry point: PromptWidget calls this method.
string DebuggerParser::run(const string& command) {

	/*
		// this was our parser test code. Left for reference.
	static Expression *lastExpression;

	// special case: parser testing
	if(strncmp(command.c_str(), "expr ", 5) == 0) {
		delete lastExpression;
		commandResult = "parser test: status==";
		int status = YaccParser::parse(command.c_str() + 5);
		commandResult += debugger->valueToString(status);
		commandResult += ", result==";
		if(status == 0) {
			lastExpression = YaccParser::getResult();
			commandResult += debugger->valueToString(lastExpression->evaluate());
		} else {
			//	delete lastExpression; // NO! lastExpression isn't valid (not 0 either)
			                          // It's the result of casting the last token
			                          // to Expression* (because of yacc's union).
			                          // As such, we can't and don't need to delete it
			                          // (However, it means yacc leaks memory on error)
			commandResult += "ERROR - ";
			commandResult += YaccParser::errorMessage();
		}
		return commandResult;
	}

	if(command == "expr") {
		if(lastExpression)
			commandResult = "result==" + debugger->valueToString(lastExpression->evaluate());
		else
			commandResult = "no valid expr";
		return commandResult;
	}
	*/

	getArgs(command);
#ifdef EXPR_REF_COUNT
	extern int refCount;
	cerr << "Expression count: " << refCount << endl;
#endif
	commandResult = "";

	int i=0;
	do {
		if( subStringMatch(verb, commands[i].cmdString.c_str()) ) {
			if( validateArgs(i) )
				CALL_METHOD(commands[i].executor);

			if( commands[i].refreshRequired )
				debugger->myBaseDialog->loadConfig();

			return commandResult;
		}

	} while(commands[++i].cmdString != "");

	commandResult = "No such command (try \"help\")";
	return commandResult;
}

// completion-related stuff:
int DebuggerParser::countCompletions(const char *in) {
	int count = 0, i = 0;
	completions = compPrefix = "";

	// cerr << "Attempting to complete \"" << in << "\"" << endl;
	do {
		const char *l = commands[i].cmdString.c_str();

		if(STR_N_CASE_CMP(l, in, strlen(in)) == 0) {
			if(compPrefix == "")
				compPrefix += l;
			else {
				int nonMatch = 0;
				const char *c = compPrefix.c_str();
				while(*c != '\0' && tolower(*c) == tolower(l[nonMatch])) {
					c++;
					nonMatch++;
				}
				compPrefix.erase(nonMatch, compPrefix.length());
				// cerr << "compPrefix==" << compPrefix << endl;
			}

			if(count++) completions += "  ";
			completions += l;
		}
	} while(commands[++i].cmdString != "");

	// cerr << "Found " << count << " label(s):" << endl << completions << endl;
	return count;
}

const char *DebuggerParser::getCompletions() {
	return completions.c_str();
}

const char *DebuggerParser::getCompletionPrefix() {
	return compPrefix.c_str();
}

string DebuggerParser::exec(const string& cmd, bool verbose) {
	string file = cmd;
	string ret;
	int count = 0;
	char buffer[256]; // FIXME: static buffers suck

	if( file.find_last_of('.') == string::npos ) {
		file += ".stella";
	}

	ifstream in(file.c_str());
	if(!in.is_open())
		return red("file \"" + file + "\" not found.");

	while( !in.eof() ) {
		if(!in.getline(buffer, 255))
			break;

		count++;
		if(verbose) {
			ret += "exec> ";
			ret += buffer;
			ret += "\n";
			ret += run(buffer);
			ret += "\n";
		}
	}
	ret += "Executed ";
	ret += debugger->valueToString(count);
	ret += " commands from \"";
	ret += file;
	ret += "\"\n";
	return ret;
}

bool DebuggerParser::saveScriptFile(string file) {
	if( file.find_last_of('.') == string::npos ) {
		file += ".stella";
	}

	ofstream out(file.c_str());

	FunctionDefMap funcs = debugger->getFunctionDefMap();
	for(FunctionDefMap::const_iterator i = funcs.begin(); i != funcs.end(); ++i)
		out << "function " << i->first << " { " << i->second << " }" << endl;

	for(unsigned int i=0; i<watches.size(); i++)
		out << "watch " << watches[i] << endl;

	for(unsigned int i=0; i<0x10000; i++)
		if(debugger->breakPoint(i))
			out << "break #" << i << endl;

	for(unsigned int i=0; i<0x10000; i++) {
		bool r = debugger->readTrap(i);
		bool w = debugger->writeTrap(i);

		if(r && w)
			out << "trap #" << i << endl;
		else if(r)
			out << "trapread #" << i << endl;
		else if(w)
			out << "trapwrite #" << i << endl;
	}

	StringList conds = debugger->cpuDebug().m6502().getCondBreakNames();
	for(unsigned int i=0; i<conds.size(); i++)
		out << "breakif {" << conds[i] << "}" << endl;

	bool ok = out.good();
	out.close();
	return ok;
}

////// executor methods for commands[] array. All are void, no args.

// "a"
void DebuggerParser::executeA() {
	debugger->cpuDebug().setA((uInt8)args[0]);
}

// "bank"
void DebuggerParser::executeBank() {
	int banks = debugger->bankCount();
	if(argCount == 0) {
		commandResult += debugger->getCartType();
		commandResult += ": ";
		if(banks < 2)
			commandResult += red("bankswitching not supported by this cartridge");
		else {
			commandResult += debugger->valueToString(debugger->getBank());
			commandResult += "/";
			commandResult += debugger->valueToString(banks);
		}
	} else {
		if(args[0] >= banks) {
			commandResult += red("invalid bank number (must be 0 to ");
			commandResult += debugger->valueToString(banks - 1);
			commandResult += ")";
		} else if(debugger->setBank(args[0])) {
			commandResult += "switched bank OK";
		} else {
			commandResult += red("unknown error switching banks");
		}
	}
}

// "base"
void DebuggerParser::executeBase() {
	if(args[0] == 2 || argStrings[0] == "bin")
		setBase(kBASE_2);
	else if(args[0] == 10 || argStrings[0] == "dec")
		setBase(kBASE_10);
	else if(args[0] == 16 || argStrings[0] == "hex")
		setBase(kBASE_16);

	commandResult = "default base set to ";
	switch(defaultBase) {
		case kBASE_2:
			commandResult += "#2/bin";
			break;

		case kBASE_10:
			commandResult += "#10/dec";
			break;

		case kBASE_16:
			commandResult += "#16/hex";
			break;

		default:
			commandResult += red("UNKNOWN");
			break;
	}
}

// "break"
void DebuggerParser::executeBreak() {
	int bp;
	if(argCount == 0)
		bp = debugger->cpuDebug().pc();
	else
		bp = args[0];

	debugger->toggleBreakPoint(bp);
	debugger->myRom->invalidate();

	if(debugger->breakPoint(bp))
		commandResult = "Set";
	else
		commandResult = "Cleared";

	commandResult += " breakpoint at ";
	commandResult += debugger->valueToString(bp);
}

// "breakif"
void DebuggerParser::executeBreakif() {
	int res = YaccParser::parse(argStrings[0].c_str());
	if(res == 0) {
		// I hate this().method().chaining().crap()
		unsigned int ret = debugger->cpuDebug().m6502().addCondBreak(
				YaccParser::getResult(), argStrings[0] );
		commandResult = "Added breakif ";
		commandResult += debugger->valueToString(ret);
	} else {
		commandResult = red("invalid expression");
	}
}

// "c"
void DebuggerParser::executeC() {
	if(argCount == 0)
		debugger->cpuDebug().toggleC();
	else if(argCount == 1)
		debugger->cpuDebug().setC(args[0]);
}

// "cheetah"
// (see http://members.cox.net/rcolbert/chtdox.htm)
void DebuggerParser::executeCheetah() {
	if(argCount == 0) {
		commandResult = red("Missing cheat code");
		return;
	}

	for(int arg = 0; arg < argCount; arg++) {
		string& cheat = argStrings[arg];
		Cheat *c = Cheat::parse(debugger->getOSystem(), cheat);
		if(c) {
			c->enable();
			commandResult = "Cheetah code " + cheat + " enabled\n";
		} else {
			commandResult = red("Invalid cheetah code " + cheat + "\n");
		}
	}
}

// "clearbreaks"
void DebuggerParser::executeClearbreaks() {
	debugger->clearAllBreakPoints();
	debugger->cpuDebug().m6502().clearCondBreaks();
	commandResult = "all breakpoints cleared";
}

// "cleartraps"
void DebuggerParser::executeCleartraps() {
	debugger->clearAllTraps();
	commandResult = "all traps cleared";
}

// "clearwatches"
void DebuggerParser::executeClearwatches() {
	delAllWatches();
	commandResult = "all watches cleared";
}

// "colortest"
void DebuggerParser::executeColortest() {
	commandResult = "test color: ";
	commandResult += char((args[0]>>1) | 0x80);
	commandResult += inverse("        ");
}

// "d"
void DebuggerParser::executeD() {
	if(argCount == 0)
		debugger->cpuDebug().toggleD();
	else if(argCount == 1)
		debugger->cpuDebug().setD(args[0]);
}

// "define"
void DebuggerParser::executeDefine() {
	// TODO: check if label already defined?
	debugger->addLabel(argStrings[0], args[1]);
	debugger->myRom->invalidate();
	commandResult = "label " + argStrings[0] + " defined as " + debugger->valueToString(args[1]);
}

// "delbreakif"
void DebuggerParser::executeDelbreakif() {
	debugger->cpuDebug().m6502().delCondBreak(args[0]);
}

// "delwatch"
void DebuggerParser::executeDelwatch() {
	commandResult = delWatch(args[0]);
}

// "disasm"
void DebuggerParser::executeDisasm() {
	commandResult = disasm();
}

// "dump"
void DebuggerParser::executeDump() {
	commandResult = dump();
}

// "exec"
void DebuggerParser::executeExec() {
	commandResult = exec(argStrings[0]);
}

// "height"
void DebuggerParser::executeHeight() {
	int height = debugger->setHeight(args[0]);
	commandResult = "height set to " + debugger->valueToString(height, kBASE_10);
}

// "help"
void DebuggerParser::executeHelp() {
	static char buf[256];
	int i = 0;
	do {
		sprintf(buf, "%13s - %s\n",
			commands[i].cmdString.c_str(),
			commands[i].description.c_str());

		commandResult += buf;
	} while(commands[++i].cmdString != "");

	commandResult += "\nBuilt-in functions:\n";
	commandResult += debugger->builtinHelp();
}

// "frame"
void DebuggerParser::executeFrame() {
	int count = 1;
	if(argCount != 0) count = args[0];
	debugger->nextFrame(count);
	commandResult = "advanced ";
	commandResult += debugger->valueToString(count);
	commandResult += " frame";
	if(count != 1) commandResult += "s";
}

// "function"
void DebuggerParser::executeFunction() {
	if(args[0] >= 0) {
		commandResult = red("name already in use");
		return;
	}

	int res = YaccParser::parse(argStrings[1].c_str());
	if(res == 0) {
		debugger->addFunction(argStrings[0], argStrings[1], YaccParser::getResult());
		commandResult = "Added function " + argStrings[0];
	} else {
		commandResult = red("invalid expression");
	}
}

// "list"
void DebuggerParser::executeList() {
	for(int i=args[0] - 2; i<args[0] + 3; i++)
		commandResult += debugger->getSourceLines(i);
}

// "listbreaks"
void DebuggerParser::executeListbreaks() {
	commandResult = listBreaks();
}

// "listtraps"
void DebuggerParser::executeListtraps() {
	commandResult = listTraps();
}

// "listwatches"
void DebuggerParser::executeListwatches() {
	// commandResult = listWatches();
	commandResult = red("command not yet implemented (sorry)");
}

// "loadstate"
void DebuggerParser::executeLoadstate() {
	if(args[0] >= 0 && args[0] <= 9) {
		debugger->loadState(args[0]);
		commandResult = "state loaded";
	} else {
		commandResult = red("invalid slot (must be 0-9)");
	}
}

// "loadlist"
void DebuggerParser::executeLoadlist() {
	commandResult = debugger->loadListFile(argStrings[0]);
}

// "loadsym"
void DebuggerParser::executeLoadsym() {
	commandResult = debugger->equateList->loadFile(argStrings[0]);
}

// "n"
void DebuggerParser::executeN() {
	if(argCount == 0)
		debugger->cpuDebug().toggleN();
	else if(argCount == 1)
		debugger->cpuDebug().setN(args[0]);
}

// "pc"
void DebuggerParser::executePc() {
	debugger->cpuDebug().setPC(args[0]);
}

// "print"
void DebuggerParser::executePrint() {
	commandResult = eval();
}

// "ram"
void DebuggerParser::executeRam() {
	if(argCount == 0)
		commandResult = debugger->dumpRAM();
	else
		commandResult = debugger->setRAM(args);
}

// "reload"
void DebuggerParser::executeReload() {
	debugger->reloadROM();
	debugger->start();
	commandResult = "reloaded";
}

// "reset"
void DebuggerParser::executeReset() {
	debugger->reset();
	commandResult = "reset CPU";
}

// "riot"
void DebuggerParser::executeRiot() {
	commandResult = debugger->riotState();
}

// "rom"
void DebuggerParser::executeRom() {
	int addr = args[0];
	for(int i=1; i<argCount; i++) {
		if( !(debugger->patchROM(addr++, args[i])) ) {
			commandResult = red("patching ROM unsupported for this cart type");
			return;
		}
	}

	// Normally the run() method calls loadConfig() on the debugger,
	// which results in all child widgets being redrawn.
	// The RomWidget is a special case, since we don't want to re-disassemble
	// any more than necessary.  So we only do it by calling the following
	// method ...
	debugger->myRom->invalidate();

	commandResult = "changed ";
	commandResult += debugger->valueToString( args.size() - 1 );
	commandResult += " location(s)";
}

// "run"
void DebuggerParser::executeRun() {
	debugger->saveOldState();
	debugger->quit();
	commandResult = "exiting debugger";
}

// "runto"
void DebuggerParser::executeRunTo() {
	bool done = false;
	int cycles = 0, count = 0;

	do {
		cycles += debugger->step();
		if(++count % 100 == 0)
			debugger->prompt()->putchar('.');
		string next = debugger->disassemble(debugger->cpuDebug().pc(), 1);
		done = (next.find(argStrings[0]) != string::npos);
	} while(!done);

	commandResult = "executed ";
	commandResult += debugger->valueToString(cycles);
	commandResult += " cycles";
}

// "s"
void DebuggerParser::executeS() {
	debugger->cpuDebug().setSP((uInt8)args[0]);
}

// "save"
void DebuggerParser::executeSave() {
	if(saveScriptFile(argStrings[0]))
		commandResult = "saved script to file " + argStrings[0];
	else
		commandResult = red("I/O error");
}

// "saverom"
void DebuggerParser::executeSaverom() {
  if(debugger->saveROM(argStrings[0]))
    commandResult = "saved ROM as " + argStrings[0];
  else
    commandResult = red("failed to save ROM");
}

// "saveses"
void DebuggerParser::executeSaveses() {
	if(debugger->prompt()->saveBuffer(argStrings[0]))
		commandResult = "saved session to file " + argStrings[0];
	else
		commandResult = red("I/O error");
}

// "savestate"
void DebuggerParser::executeSavestate() {
	if(args[0] >= 0 && args[0] <= 9) {
		debugger->saveState(args[0]);
		commandResult = "state saved";
	} else {
		commandResult = red("invalid slot (must be 0-9)");
	}
}

// "savesym"
void DebuggerParser::executeSavesym() {
	if(debugger->equateList->saveFile(argStrings[0]))
		commandResult = "saved symbols to file " + argStrings[0];
	else
		commandResult = red("I/O error");
}

// "scanline"
void DebuggerParser::executeScanline() {
	int count = 1;
	if(argCount != 0) count = args[0];
	debugger->nextScanline(count);
	commandResult = "advanced ";
	commandResult += debugger->valueToString(count);
	commandResult += " scanline";
	if(count != 1) commandResult += "s";
}

// "step"
void DebuggerParser::executeStep() {
	int cycles = debugger->step();
	commandResult = "executed ";
	commandResult += debugger->valueToString(cycles);
	commandResult += " cycles";
}

// "tia"
void DebuggerParser::executeTia() {
	commandResult = debugger->dumpTIA();
}

// "trace"
void DebuggerParser::executeTrace() {
	int cycles = debugger->trace();
	commandResult = "executed ";
	commandResult += debugger->valueToString(cycles);
	commandResult += " cycles";
}

// "trap"
void DebuggerParser::executeTrap() {
	debugger->toggleReadTrap(args[0]);
	debugger->toggleWriteTrap(args[0]);
	commandResult = trapStatus(args[0]);
}

// "trapread"
void DebuggerParser::executeTrapread() {
	debugger->toggleReadTrap(args[0]);
	commandResult = trapStatus(args[0]);
}

// "trapwrite"
void DebuggerParser::executeTrapwrite() {
	debugger->toggleWriteTrap(args[0]);
	commandResult = trapStatus(args[0]);
}

// "undef"
void DebuggerParser::executeUndef() {
	if(debugger->equateList->undefine(argStrings[0]))
	{
		debugger->myRom->invalidate();
		commandResult = argStrings[0] + " now undefined";
	}
	else
		commandResult = red("no such label");
}

// "v"
void DebuggerParser::executeV() {
	if(argCount == 0)
		debugger->cpuDebug().toggleV();
	else if(argCount == 1)
		debugger->cpuDebug().setV(args[0]);
}

// "watch"
void DebuggerParser::executeWatch() {
	addWatch(argStrings[0]);
	commandResult = "added watch \"" + argStrings[0] + "\"";
}

// "x"
void DebuggerParser::executeX() {
	debugger->cpuDebug().setX((uInt8)args[0]);
}

// "y"
void DebuggerParser::executeY() {
	debugger->cpuDebug().setY((uInt8)args[0]);
}

// "z"
void DebuggerParser::executeZ() {
	if(argCount == 0)
		debugger->cpuDebug().toggleZ();
	else if(argCount == 1)
		debugger->cpuDebug().setZ(args[0]);
}


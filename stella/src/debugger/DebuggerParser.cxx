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
// $Id: DebuggerParser.cxx,v 1.36 2005-06-27 01:36:00 urchlay Exp $
//============================================================================

#include "bspf.hxx"
#include "Debugger.hxx"
#include "DebuggerParser.hxx"
#include "D6502.hxx"
#include "EquateList.hxx"

// TODO: finish this, replace run() and getArgs() with versions
// that use this table.
Command DebuggerParser::commands[] = {
	{
		"a",
		"Set Accumulator to value xx",
		true,
		{ kARG_BYTE, kARG_END_ARGS },
		&DebuggerParser::executeA
	},

	{
		"base",
		"Set default base (hex, dec, or bin)",
		true,
		{ kARG_BASE_SPCL, kARG_END_ARGS },
		&DebuggerParser::executeBase
	},

	{
		"break",
		"Set/clear breakpoint at address (default=pc)",
		false,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeBreak
	},

	{
		"c",
		"Carry Flag: set (to 0 or 1), or toggle (no arg)",
		false,
		{ kARG_BOOL, kARG_END_ARGS },
		&DebuggerParser::executeC
	},

	{
		"clearbreaks",
		"Clear all breakpoints",
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeClearbreaks
	},

	{
		"cleartraps",
		"Clear all traps",
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeCleartraps
	},

	{
		"clearwatches",
		"Clear all watches",
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeClearwatches
	},

	{
		"d",
		"Decimal Flag: set (to 0 or 1), or toggle (no arg)",
		false,
		{ kARG_BOOL, kARG_END_ARGS },
		&DebuggerParser::executeD
	},

	{
		"define",
		"Define label",
		true,
		{ kARG_LABEL, kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeDefine
	},

	{
		"delwatch",
		"Delete watch",
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeDelwatch
	},

	{
		"disasm",
		"Disassemble from address (default=pc)",
		false,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeDisasm
	},

	{
		"dump",
		"Dump 128 bytes of memory at address",
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeDump
	},

	{
		"frame",
		"Advance emulation by xx frames (default=1)",
		false,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeFrame
	},

	// TODO: height command

	{
		"help",
		"This cruft",
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeHelp
	},

	{
		"listbreaks",
		"List breakpoints",
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeListbreaks
	},

	{
		"listtraps",
		"List traps",
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeListtraps
	},

	{
		"listwatches",
		"List watches",
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeListwatches
	},

	{
		"loadsym",
		"Load symbol file",
		true,
		{ kARG_FILE, kARG_END_ARGS },
		&DebuggerParser::executeLoadsym
	},

	{
		"n",
		"Negative Flag: set (to 0 or 1), or toggle (no arg)",
		false,
		{ kARG_BOOL, kARG_END_ARGS },
		&DebuggerParser::executeN
	},

	{
		"pc",
		"Set Program Counter to address",
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executePc
	},

	{
		"print",
		"Evaluate and print expression in hex/dec/binary",
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executePrint
	},

	{
		"ram",
		"Show RAM contents (no args), or set RAM address xx to value yy",
		false,
		{ kARG_WORD, kARG_MULTI_BYTE },
		&DebuggerParser::executeRam
	},

	{
		"reload",
		"Reload ROM and symbol file",
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeReload
	},

	{
		"reset",
		"Reset 6507 to init vector (does not reset TIA, RIOT)",
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeReset
	},

	{
		"run",
		"Exit debugger, return to emulator",
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeRun
	},

	{
		"s",
		"Set Stack Pointer to value xx",
		true,
		{ kARG_BYTE, kARG_END_ARGS },
		&DebuggerParser::executeS
	},

	{
		"saveses",
		"Save console session to file",
		true,
		{ kARG_FILE, kARG_END_ARGS },
		&DebuggerParser::executeSaveses
	},

	{
		"savesym",
		"Save symbols to file",
		true,
		{ kARG_FILE, kARG_END_ARGS },
		&DebuggerParser::executeSavesym
	},

	{
		"step",
		"Single step CPU (optionally, with count)",
		false,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeStep
	},

	{
		"tia",
		"Show TIA state (NOT FINISHED YET)",
		false,
		{ kARG_END_ARGS },
		&DebuggerParser::executeTia
	},

	{
		"trace",
		"Single step CPU (optionally, with count), subroutines count as one instruction",
		false,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeTrace
	},

	{
		"trap",
		"Trap read and write accesses to address",
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeTrap
	},

	{
		"trapread",
		"Trap read accesses to address",
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeTrapread
	},

	{
		"trapwrite",
		"Trap write accesses to address",
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeTrapwrite
	},

	{
		"undef",
		"Undefine label (if defined)",
		true,
		{ kARG_LABEL, kARG_END_ARGS },
		&DebuggerParser::executeUndef
	},

	{
		"v",
		"Overflow Flag: set (to 0 or 1), or toggle (no arg)",
		false,
		{ kARG_BOOL, kARG_END_ARGS },
		&DebuggerParser::executeV
	},

	{
		"watch",
		"Print contents of address before every prompt",
		true,
		{ kARG_WORD, kARG_END_ARGS },
		&DebuggerParser::executeWatch
	},

	{
		"x",
		"Set X Register to value xx",
		true,
		{ kARG_BYTE, kARG_END_ARGS },
		&DebuggerParser::executeX
	},

	{
		"y",
		"Set Y Register to value xx",
		true,
		{ kARG_BYTE, kARG_END_ARGS },
		&DebuggerParser::executeY
	},

	{
		"z",
		"Zero Flag: set (to 0 or 1), or toggle (no arg)",
		false,
		{ kARG_BOOL, kARG_END_ARGS },
		&DebuggerParser::executeZ
	},

	{
		"",
		"",
		false,
		{ kARG_END_ARGS },
		NULL
	}

};

// Constants for argument processing
enum {
  kIN_COMMAND,
  kIN_SPACE,
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
	if(arg == "a") result = debugger->getA();
	else if(arg == "x") result = debugger->getX();
	else if(arg == "y") result = debugger->getY();
	else if(arg == "p") result = debugger->getPS();
	else if(arg == "s") result = debugger->getSP();
	else if(arg == "pc" || arg == ".") result = debugger->getPC();
	else { // Not a special, must be a regular arg: check for label first
		const char *a = arg.c_str();
		result = debugger->equateList->getAddress(a);

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
				if(*c != ' ')
					state = kIN_ARG;
					curArg += *c;
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

	//for(int i=0; i<argCount; i++)
		//cerr << "argStrings[" << i << "] == \"" << argStrings[i] << "\"" << endl;

	// Now decipher each argument, in turn.
	for(int i=0; i<argCount; i++) {
		int temp = decipher_arg(argStrings[i]);
		args.push_back(temp); // value maybe -1, if not expression argument
		                      // (validate_args will decide whether that's OK, not us.)
		//cerr << "args[" << i << "] == " << args[i] << endl;
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
	string ret;

	for(unsigned int i=0; i<0x10000; i++) {
		if(debugger->breakPoints->isSet(i)) {
			sprintf(buf, "%s ", debugger->equateList->getFormatted(i, 4));
			ret += buf;
			if(! (++count % 8) ) ret += "\n";
		}
	}
	if(count)
		return ret;
	else
		return "no breakpoints set";
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
		start = debugger->getPC();
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
		char *label = debugger->equates()->getLabel(args[i]);
		if(label != NULL) {
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
	string ret = "\n";
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
				ret += "  watch #";
				ret += buf;
				ret += " (" + argStrings[0] + ") -> " + eval() + "\n";
			}
		}
	}
	// get rid of trailing \n
	ret.erase(ret.length()-1, 1);
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

	char *l = debugger->equateList->getLabel(addr);
	if(l != NULL) {
		result += "  (";
		result += l;
		result += ")";
	}

	return result;
}

bool DebuggerParser::validateArgs(int cmd) {
	// cerr << "entering validateArgs(" << cmd << ")" << endl;
	bool required = commands[cmd].parmsRequired;
	parameters *p = commands[cmd].parms;

	if(argCount == 0) {
		if(required) {
			commandResult = "missing required argument(s)";
			return false; // needed args. didn't get 'em.
		} else {
			return true;  // no args needed, no args got
		}
	}

	int curCount = 0;

	do {
		if(argCount == curCount) {
			return true;
		}

		int curArgInt = args[curCount];
		string curArgStr = argStrings[curCount];

		switch(*p) {
			case kARG_WORD:
				if(curArgInt < 0 || curArgInt > 0xffff) {
					commandResult = "invalid word argument (must be 0-$ffff)";
					return false;
				}
				break;

			case kARG_BYTE:
				if(curArgInt < 0 || curArgInt > 0xff) {
					commandResult = "invalid byte argument (must be 0-$ff)";
					return false;
				}
				break;

			case kARG_BOOL:
				if(curArgInt != 0 && curArgInt != 1) {
					commandResult = "invalid boolean argument (must be 0 or 1)";
					return false;
				}
				break;

			case kARG_BASE_SPCL:
				if(curArgInt != 2 && curArgInt != 10 && curArgInt != 16
					&& curArgStr != "hex" && curArgStr != "dec" && curArgStr != "bin")
				{
					commandResult = "invalid base (must be #2, #10, #16, \"bin\", \"dec\", or \"hex\")";
					return false;
				}
				break;

			case kARG_LABEL:
			case kARG_FILE:
				break; // TODO: validate these (for now any string's allowed)

			case kARG_MULTI_BYTE:
			case kARG_MULTI_WORD:
				break; // FIXME: implement!

			default:
				commandResult = "too many arguments";
				return false;
				break;
		}

		curCount++;

	} while(*p++ != kARG_END_ARGS);

	if(curCount < argCount) {
		commandResult = "too many arguments";
		return false;
	}

	return true;
}

// main entry point: PromptWidget calls this method.
string DebuggerParser::run(const string& command) {
	int i=0;

	getArgs(command);
	commandResult = "";

	do {
		if( subStringMatch(verb, commands[i].cmdString.c_str()) ) {
			if( validateArgs(i) )
				CALL_METHOD(commands[i].executor);

			return commandResult;
		}

	} while(commands[++i].cmdString != "");

	commandResult = "No such command (try \"help\")";
	return commandResult;
}

////// executor methods for commands[] array. All are void, no args.

// "a"
void DebuggerParser::executeA() {
	debugger->setA(args[0]);
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
			commandResult += "UNKNOWN";
			break;
	}
}

// "break"
void DebuggerParser::executeBreak() {
	int bp;
	if(argCount == 0)
		bp = debugger->getPC();
	else
		bp = args[0];

	debugger->toggleBreakPoint(bp);

	if(debugger->breakPoint(bp))
		commandResult = "Set";
	else
		commandResult = "Cleared";

	commandResult += " breakpoint at ";
	commandResult += debugger->valueToString(bp);
}

// "c"
void DebuggerParser::executeC() {
	if(argCount == 0)
		debugger->toggleC();
	else if(argCount == 1)
		debugger->setC(args[0]);
}

// "clearbreaks"
void DebuggerParser::executeClearbreaks() {
	debugger->clearAllBreakPoints();
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

// "d"
void DebuggerParser::executeD() {
	if(argCount == 0)
		debugger->toggleD();
	else if(argCount == 1)
		debugger->setD(args[0]);
}

// "define"
void DebuggerParser::executeDefine() {
	// TODO: check if label already defined?
	debugger->addLabel(argStrings[0], args[1]);
	commandResult = "label " + argStrings[0] + " defined as " + debugger->valueToString(args[1]);
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
	commandResult = "command not yet implemented (sorry)";
}

// "loadsym"
void DebuggerParser::executeLoadsym() {
	commandResult = debugger->equateList->loadFile(argStrings[0]);
}

// "n"
void DebuggerParser::executeN() {
	if(argCount == 0)
		debugger->toggleN();
	else if(argCount == 1)
		debugger->setN(args[0]);
}

// "pc"
void DebuggerParser::executePc() {
	debugger->setPC(args[0]);
}

// "print"
void DebuggerParser::executePrint() {
	commandResult = eval();
}

// "ram"
void DebuggerParser::executeRam() {
	if(argCount == 0)
		commandResult = debugger->dumpRAM(kRamStart);
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

// "run"
void DebuggerParser::executeRun() {
	debugger->quit();
	commandResult = "exiting debugger";
}

// "s"
void DebuggerParser::executeS() {
	debugger->setSP(args[0]);
}

// "saveses"
void DebuggerParser::executeSaveses() {
	if(debugger->prompt()->saveBuffer(argStrings[0]))
		commandResult = "saved session to file " + argStrings[0];
	else
		commandResult = "I/O error";
}

// "savesym"
void DebuggerParser::executeSavesym() {
	if(debugger->equateList->saveFile(argStrings[0]))
		commandResult = "saved symbols to file " + argStrings[0];
	else
		commandResult = "I/O error";
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
		commandResult = argStrings[0] + " now undefined";
	else
		commandResult = "no such label";
}

// "v"
void DebuggerParser::executeV() {
	if(argCount == 0)
		debugger->toggleV();
	else if(argCount == 1)
		debugger->setV(args[0]);
}

// "watch"
void DebuggerParser::executeWatch() {
	addWatch(argStrings[0]);
	commandResult = "added watch \"" + argStrings[0] + "\"";
}

// "x"
void DebuggerParser::executeX() {
	debugger->setX(args[0]);
}

// "y"
void DebuggerParser::executeY() {
	debugger->setY(args[0]);
}

// "z"
void DebuggerParser::executeZ() {
	if(argCount == 0)
		debugger->toggleZ();
	else if(argCount == 1)
		debugger->setZ(args[0]);
}


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
// $Id: DebuggerParser.cxx,v 1.30 2005-06-23 02:56:45 urchlay Exp $
//============================================================================

#include "bspf.hxx"
#include "Debugger.hxx"
#include "DebuggerParser.hxx"
#include "D6502.hxx"
#include "EquateList.hxx"

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

// Evaluate expression.
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
		arg.erase(0, 1);
	} else if(arg.substr(0, 1) == "#") {
		dec = true;
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
	else if(arg == "p") result = debugger->getP();
	else if(arg == "s") result = debugger->getS();
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
		if(temp < 0) {
			return false;
		}
		args.push_back(temp);
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
	else result += "   none   ";

	char *l = debugger->equateList->getLabel(addr);
	if(l != NULL) {
		result += "  (";
		result += l;
		result += ")";
	}

	return result;
}

string DebuggerParser::run(const string& command) {
	string result;

	if(!getArgs(command)) {
		// commands that take filenames or other arbitrary strings go here.
		if(subStringMatch(verb, "loadsym")) {
			result = debugger->equateList->loadFile(argStrings[0]);
			return result;
		} else if(subStringMatch(verb, "savesym")) {
			if(debugger->equateList->saveFile(argStrings[0]))
				return "saved symbols to file " + argStrings[0];
			else
				return "I/O error";
		} else if(subStringMatch(verb, "define")) {
         int arg = decipher_arg(argStrings[1]);
			if(arg < 0)
				return "invalid argument";

			debugger->addLabel(argStrings[0], arg);
			return "label " + argStrings[0] + " defined as " + debugger->valueToString(arg);
		} else if(subStringMatch(verb, "saveses")) {
			if(debugger->prompt()->saveBuffer(argStrings[0]))
				return "saved session to file " + argStrings[0];
			else
				return "I/O error";
		} else {
			return "invalid label or address";
		}
	}

	// "verb" is the command, stripped of any arguments.
	// In the if/else below, put shorter command names first.
	// In case of conflicts (e.g. user enters "t", possible
	// commands are "tia" and "trace"), try to guess which
	// will be used more often, and put it first. The user
	// can always disambiguate: "ti" is short for "tia", or
	// "tr" for "trace".

	// TODO: de-uglify this somehow. (it may not be worth doing?)

	if(subStringMatch(verb, "a")) {
      if(argCount == 1)
			if(args[0] <= 0xff)
				debugger->setA(args[0]);
			else
				return "value out of range (must be 00 - ff)";
		else
			return "one argument required";
	} else if(subStringMatch(verb, "c")) {
		debugger->toggleC();
	} else if(subStringMatch(verb, "z")) {
		debugger->toggleZ();
	} else if(subStringMatch(verb, "n")) {
		debugger->toggleN();
	} else if(subStringMatch(verb, "v")) {
		debugger->toggleV();
	} else if(subStringMatch(verb, "d")) {
		debugger->toggleD();
	} else if(subStringMatch(verb, "pc")) {
      if(argCount == 1)
			debugger->setPC(args[0]);
		else
			return "one argument required";
	} else if(subStringMatch(verb, "x")) {
      if(argCount == 1)
			if(args[0] <= 0xff)
				debugger->setX(args[0]);
			else
				return "value out of range (must be 00 - ff)";
		else
			return "one argument required";
	} else if(subStringMatch(verb, "y")) {
      if(argCount == 1)
			if(args[0] <= 0xff)
				debugger->setY(args[0]);
			else
				return "value out of range (must be 00 - ff)";
		else
			return "one argument required";
	} else if(subStringMatch(verb, "s")) {
      if(argCount == 1)
			if(args[0] <= 0xff)
				debugger->setS(args[0]);
			else
				return "value out of range (must be 00 - ff)";
		else
			return "one argument required";
	} else if(subStringMatch(verb, "step")) {
		char buf[12];
		if(argCount > 0)
			return "step takes no arguments";
		sprintf(buf, "#%d", debugger->step());
		result = "executed ";
		result += buf;
		result += " cycles";
	} else if(subStringMatch(verb, "trace")) {
		char buf[12];
		if(argCount > 0)
			return "trace takes no arguments";
		sprintf(buf, "#%d", debugger->trace());
		result = "executed ";
		result += buf;
		result += " cycles";
	} else if(subStringMatch(verb, "ram")) {
		if(argCount == 0)
			result = debugger->dumpRAM(kRamStart);
		else if(argCount == 1)
			return "missing data (need 2 or more args)";
		else
			result = debugger->setRAM(args);
	} else if(subStringMatch(verb, "tia")) {
		result = debugger->dumpTIA();
	} else if(subStringMatch(verb, "reset")) {
		debugger->reset();
	} else if(subStringMatch(verb, "reload")) {
		debugger->reloadROM();
		debugger->start();
	} else if(subStringMatch(verb, "trap")) {
		if(argCount != 1)
			return "one argument required";

		debugger->toggleReadTrap(args[0]);
		debugger->toggleWriteTrap(args[0]);
		return trapStatus(args[0]);
	} else if(subStringMatch(verb, "trapwrite")) {
		if(argCount != 1)
			return "one argument required";

		debugger->toggleWriteTrap(args[0]);
		return trapStatus(args[0]);
	} else if(subStringMatch(verb, "trapread")) {
		if(argCount != 1)
			return "one argument required";

		debugger->toggleReadTrap(args[0]);
		return trapStatus(args[0]);
	} else if(subStringMatch(verb, "cleartraps")) {
		debugger->clearAllTraps();
		return "cleared all traps";
	} else if(subStringMatch(verb, "break")) {
		int bp = -1;

		if(argCount > 1) {
			return "zero or one arguments required";
		} else if(argCount == 0) {
			bp = -1;
		} else if(argCount == 1) {
			bp = args[0];
		}

		debugger->toggleBreakPoint(bp);

		if(debugger->breakPoint(bp))
			return "Set breakpoint";
		else
			return "Cleared breakpoint";
	} else if(subStringMatch(verb, "listbreaks")) {
		return listBreaks();
	} else if(subStringMatch(verb, "listtraps")) {
		return listTraps();
	} else if(subStringMatch(verb, "disasm")) {
		return disasm();
	} else if(subStringMatch(verb, "frame")) {
		// FIXME: make multiple frames work!
		int count = 1;
		if(argCount != 0) count = args[0];
		for(int i=0; i<count; i++)
			debugger->nextFrame();
		return "advanced frame";
	} else if(subStringMatch(verb, "clearbreaks")) {
		debugger->clearAllBreakPoints();
	} else if(subStringMatch(verb, "clearwatches")) {
		delAllWatches();
		return "cleared all watches";
	} else if(subStringMatch(verb, "watch")) {
		if(argCount != 1)
			return "one argument required";

		addWatch(argStrings[0]);
	} else if(subStringMatch(verb, "delwatch")) {
		if(argCount == 1)
			return delWatch(args[0]);
		else
			return "one argument required";
	} else if(subStringMatch(verb, "define")) {
		return argStrings[0] + " already defined";
	} else if(subStringMatch(verb, "undef")) {
		if(debugger->equateList->undefine(argStrings[0]))
			return argStrings[0] + " now undefined";
		else
			return "no such label";
	} else if(subStringMatch(verb, "height")) {
		if(argCount != 1)
			return "one argument required";

		return "This command is not yet supported. Start Stella with the -debugheight option instead.";
/*
		if(debugger->setHeight(args[0]))
			//return "Stella will use the new height next time it starts.";
			return "OK";
		else
			return "bad height (use 0 for default, min height #383)";
*/
	} else if(subStringMatch(verb, "dump")) {
		if(argCount != 1)
			return "one argument required";
		else
			return dump();
	} else if(subStringMatch(verb, "print")) {
		if(argCount < 1)
			return "one or more arguments required";
		else
			return eval();
	} else if(subStringMatch(verb, "base")) {
		switch(args[0]) {
			case 2:
				setBase(kBASE_2);
				break;

			case 10:
				setBase(kBASE_10);
				break;

			case 16:
				setBase(kBASE_16);
				break;

			default:
				return "invalid base (must be #2, #10, or #16)";
				break;
		}
		char buf[5];
		sprintf(buf, "#%2d", args[0]);
		result += "Set base ";
		result += buf;
		return result;
	} else if(subStringMatch(verb, "listsym")) {
		return debugger->equateList->dumpAll();
	} else if(subStringMatch(verb, "quit") || subStringMatch(verb, "run")) {
		debugger->quit();
		return "";
	} else if(subStringMatch(verb, "help") || verb == "?") {
		// please leave each option on its own line so they're
		// easy to sort - bkw
		return
			"Commands are case-insensitive and may be abbreviated (e.g. \"tr\" for \"trace\").\n"
			"Arguments are either labels or numeric constants, and may be\n"
			"prefixed with a * to dereference, < or > for low/high byte,\n"
			"and/or $/#/% for hex/dec/binary.\n\n"
			"a xx         - Set Accumulator to xx\n"
			"*bank xx      - Switch to ROM bank xx\n"
			"base xx      - Set default input base (#2=binary, #10=decimal, #16=hex)\n"
			"break        - Set/clear breakpoint at current PC\n"
			"break xx     - Set/clear breakpoint at address xx\n"
			"c            - Toggle Carry Flag\n"
			"*cartinfo     - Show cartridge information\n"
			"clearbreaks  - Clear all breakpoints\n"
			"cleartraps   - Clear all traps\n"
			"clearwatches - Clear all watches\n"
			"d            - Toggle Decimal Flag\n"
			"define ll xx - Define label ll with value xx\n"
			"delwatch xx  - Delete watch xx\n"
			"disasm       - Disassemble (from current PC)\n"
			"disasm xx    - Disassemble (from address xx)\n"
			"dump xx      - Dump 128 bytes of memory starting at xx (may be ROM, TIA, RAM)\n"
			"frame        - Advance to next TIA frame, then break\n"
			"frame xx     - Advance TIA by xx frames, then break\n"
			"height xx    - Set height of debugger window in pixels (NOT WORKING)\n"
			"listbreaks   - List all breakpoints\n"
			"listtraps    - List all traps\n"
			"listsym      - List all currently defined symbols\n"
			"loadsym f    - Load DASM symbols from file f\n"
			"n            - Toggle Negative Flag\n"
			"pc xx        - Set Program Counter to xx\n"
			"print xx     - Evaluate and print expression xx in hex/decimal/binary\n"
			//"poke xx yy   - Write data yy to address xx (may be ROM, TIA, etc)\n"
			"ram          - Show RIOT RAM contents\n"
			"ram xx yy    - Set RAM location xx to value yy (multiple values allowed)\n"
			"reload       - Reload ROM and symbol file (resets debugger)\n"
			"reset        - Jump to 6502 init vector (does not reset TIA/RIOT)\n"
			"run          - Exit debugger (back to emulator)\n"
			"s xx         - Set Stack Pointer to xx\n"
			"saveses f    - Save console session to file f\n"
			"savesym f    - Save symbols to file f\n"
			"step         - Single-step\n"
			"tia          - Show TIA register contents (NOT FINISHED YET)\n"
			"trace        - Single-step treating subroutine calls as 1 instruction\n"
			"trap xx      - Trap any access to location xx (enter debugger on access)\n"
			"trapread xx  - Trap any read access from location xx\n"
			"trapwrite xx - Trap any write access to location xx\n"
			"undef ll     - Undefine label ll (if defined)\n"
			"v            - Toggle Overflow Flag\n"
			"watch xx     - Print contents of location xx before every prompt\n"
			"x xx         - Set X register to xx\n"
			"y xx         - Set Y register to xx\n"
			"z            - Toggle Zero Flag\n"
			;
	} else {
		return "unimplemented command (try \"help\")";
	}

	return result;
}

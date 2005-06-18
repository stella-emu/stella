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
// $Id: DebuggerParser.cxx,v 1.13 2005-06-18 17:28:18 urchlay Exp $
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
  kIN_ARG_START,
  kIN_ARG_CONT
};


DebuggerParser::DebuggerParser(Debugger* d)
  	: debugger(d)
{
	done = false;
}

DebuggerParser::~DebuggerParser() {
}

string DebuggerParser::currentAddress() {
	return "currentAddress()";
}

void DebuggerParser::setDone() {
	done = true;
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

// Given a string argument that's either a label,
// hex value, or a register, either dereference the label or convert
// the hex to an int. Returns -1 on error.
int DebuggerParser::decipher_arg(string &arg, bool deref) {
	const char *a = arg.c_str();
	int address;

	// Special cases (registers):
	if(arg == "a") address = debugger->getA();
	else if(arg == "x") address = debugger->getX();
	else if(arg == "y") address = debugger->getY();
	else if(arg == "p") address = debugger->getP();
	else if(arg == "s") address = debugger->getS();
	else if(arg == "pc") address = debugger->getPC();
	else { // normal addresses: check for label first
		address = debugger->equateList->getAddress(a);

		// if not label, must be hex.
		if(address < 0) {
			address = 0;
			while(*a != '\0') {
				int hex = conv_hex_digit(*a++);
				if(hex < 0)
					return -1;

				address = (address << 4) + hex;
			}
		}
	}

	// dereference if we're supposed to:
	if(deref) address = debugger->peek(address);

	return address;
}

bool DebuggerParser::getArgs(const string& command) {
	int state = kIN_COMMAND;
	bool deref = false;
	string curArg = "";
	argCount = 0;
	verb = "";
	const char *c = command.c_str();

	// cerr << "Parsing \"" << command << "\"" << endl;

	while(*c != '\0') {
		// cerr << "State " << state << ", *c '" << *c << "'" << endl;
		switch(state) {
			case kIN_COMMAND:
				if(*c == ' ')
					state = kIN_SPACE;
				else
					verb += *c;
				c++;
				break;

			case kIN_SPACE:
				if(*c == ' ')
					c++;
				else
					state = kIN_ARG_START;
				break;

			case kIN_ARG_START:
				curArg = "";
				state = kIN_ARG_CONT;
				// FIXME: actually use this.
				if(*c == '*') {
					deref = true;
					c++;
				} else {
					deref = false;
				} // FALL THROUGH!

			case kIN_ARG_CONT:
				if(isalpha(*c) || isdigit(*c) || *c == '_') {
					curArg += *c++;
					// cerr << "curArg: " << curArg << endl;
				} else {
					int a = decipher_arg(curArg, deref);
					if(a < 0)
						return false;
					args[argCount++] = a;
					curArg = "";
					state = kIN_SPACE;
				}

		}

		if(argCount == 10) {
			cerr << "reached max 10 args" << endl;
			return true;
		}
	}

	// pick up last arg, if any:
	if(state == kIN_ARG_CONT || state == kIN_ARG_START)
		if( (args[argCount++] = decipher_arg(curArg, deref)) < 0)
			return false;

	// for(int i=0; i<argCount; i++)
		// cerr << "args[" << i << "] == " << args[i] << endl;

	// cerr << endl;
	return true;
}

bool DebuggerParser::subStringMatch(const string& needle, const string& haystack) {
	const char *hs = haystack.c_str();
	const char *n = needle.c_str();

	if(strncasecmp(n, hs, strlen(n)) == 0)
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

string DebuggerParser::eval() {
	char buf[10];
	string ret;
	for(int i=0; i<argCount; i++) {
		char *label = debugger->equates()->getLabel(args[i]);
		if(label != NULL) {
			ret += label;
			ret += ": ";
		}
		if(args[i] < 0x100) {
			ret += Debugger::to_hex_8(args[i]);
			ret += " ";
			ret += Debugger::to_bin_8(args[i]);
		} else {
			ret += Debugger::to_hex_16(args[i]);
			ret += " ";
			ret += Debugger::to_bin_16(args[i]);
		}
		sprintf(buf, " %d", args[i]);
		ret += buf;
		if(i != argCount - 1) ret += "\n";
	}
	return ret;
}

string DebuggerParser::run(const string& command) {
	string result;

	// special case command, takes a filename instead of an address:
	if(subStringMatch("loadsym ", command)) {
		result = command;
		result.erase(0, 8);
		result = debugger->equateList->loadFile(result);
		return result;
	}

	if(!getArgs(command))
		return "invalid label or address";

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
		if(argCount > 0)
			return "step takes no arguments";
		debugger->step();
		result = "OK";
	} else if(subStringMatch(verb, "trace")) {
		if(argCount > 0)
			return "trace takes no arguments";
		debugger->trace();
		result = "OK";
	} else if(subStringMatch(verb, "ram")) {
		if(argCount == 0)
			result = debugger->dumpRAM(kRamStart);
		else if(argCount == 1)
			return "missing data (need 2 or more args)";
		else
			result = debugger->setRAM(argCount, args);
	} else if(subStringMatch(verb, "tia")) {
		result = debugger->dumpTIA();
	} else if(subStringMatch(verb, "reset")) {
		debugger->reset();
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
	} else if(subStringMatch(verb, "disasm")) {
		return disasm();
	} else if(subStringMatch(verb, "frame")) {
		/*
		// FIXME: make multiple frames work!
		int count = 0;
		if(argCount != 0) count = args[0];
		for(int i=0; i<count; i++)
			debugger->nextFrame();
		*/
		debugger->nextFrame();
		return "OK";
	} else if(subStringMatch(verb, "clearbreaks")) {
		debugger->clearAllBreakPoints();
		return "cleared all breakpoints";
	} else if(subStringMatch(verb, "eval")) {
		if(argCount < 1)
			return "one or more arguments required";
		else
			return eval();
	} else if(subStringMatch(verb, "quit") || subStringMatch(verb, "run")) {
		debugger->quit();
		return "";
	} else if(subStringMatch(verb, "help") || verb == "?") {
		// please leave each option on its own line so they're
		// easy to sort - bkw
		return
			"Commands are case-insensitive and may be abbreviated.\n"
			"Arguments are either labels or hex constants, and may be\n"
			"prefixed with a * to dereference.\n"
			"a xx        - Set Accumulator to xx\n"
			"break       - Set/clear breakpoint at current PC\n"
			"break xx    - Set/clear breakpoint at address xx\n"
			"c           - Toggle Carry Flag\n"
			"clearbreaks - Clear all breakpoints\n"
			"d           - Toggle Decimal Flag\n"
			"disasm      - Disassemble (from current PC)\n"
			"disasm xx   - Disassemble (from address xx)\n"
			"eval xx     - Evaluate expression xx\n"
			"frame       - Advance to next TIA frame, then break\n"
			"listbreaks  - List all breakpoints\n"
			"loadsym f   - Load DASM symbols from file f\n"
			"n           - Toggle Negative Flag\n"
			"pc xx       - Set Program Counter to xx\n"
			"ram         - Show RIOT RAM contents\n"
			"ram xx yy   - Set RAM location xx to value yy (multiple values allowed)\n"
			"reset       - Jump to 6502 init vector (does not reset TIA/RIOT)\n"
			"run         - Exit debugger (back to emulator)\n"
			"s xx        - Set Stack Pointer to xx\n"
			"step        - Single-step\n"
			"tia         - Show TIA register contents\n"
			"trace       - Single-step treating subroutine calls as 1 instruction\n"
			"v           - Toggle Overflow Flag\n"
			"x xx        - Set X register to xx\n"
			"y xx        - Set Y register to xx\n"
			"z           - Toggle Zero Flag\n"
			;
	} else {
		return "unimplemented command (try \"help\")";
	}

	return result;
}

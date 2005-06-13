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
// $Id: DebuggerParser.cxx,v 1.2 2005-06-13 02:47:44 urchlay Exp $
//============================================================================

#include "bspf.hxx"
#include "Debugger.hxx"
#include "DebuggerParser.hxx"

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
	else return 0;
}

void DebuggerParser::getArgs(const string& command) {
	int state = kIN_COMMAND;
	int deref = 0;
	int curArg = 0;
	argCount = 0;
	verb = "";
	const char *c = command.c_str();

	// cerr << "Parsing \"" << command << "\"" << endl;

	while(*c != '\0') {
		// cerr << "State " << state << ", *c " << *c << endl;
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
				curArg = 0;
				state = kIN_ARG_CONT;
				if(*c == '*') {
					deref = 1;
					c++;
				} else {
					deref = 0;
				} // FALL THROUGH!

			case kIN_ARG_CONT:
				if(isxdigit(*c)) {
					int dig = conv_hex_digit(*c);
					curArg = (curArg << 4) + dig;
					*c++;
				} else {
					args[argCount++] = curArg;
					state = kIN_SPACE;
				}
				break;
		}

		if(argCount == 10) {
			cerr << "reached max 10 args" << endl;
			return;
		}
	}

	// pick up last arg, if any:
	if(state == kIN_ARG_CONT || state == kIN_ARG_START)
		args[argCount++] = curArg;

	// for(int i=0; i<argCount; i++)
		// cerr << "args[" << i << "] == " << args[i] << endl;

	// cerr << endl;
}

bool DebuggerParser::subStringMatch(const string& needle, const string& haystack) {
	const char *hs = haystack.c_str();
	const char *n = needle.c_str();

	if(strncasecmp(n, hs, strlen(n)) == 0)
		return true;

	return false;
}

string DebuggerParser::run(const string& command) {
	string result;

	getArgs(command);

	// "verb" is the command, stripped of any arguments.
	// In the if/else below, put shorter command names first.
	// In case of conflicts (e.g. user enters "t", possible
   // commands are "tia" and "trace"), try to guess which
	// will be used more often, and put it first. The user
	// can always disambiguate: "ti" is short for "tia", or
	// "tr" for "trace".

	// TODO: de-uglify this somehow. (it may not be worth doing?)

	if(subStringMatch(verb, "quit")) {
		debugger->quit();
		return "";
	} else if(subStringMatch(verb, "a")) {
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
	} else if(subStringMatch(verb, "help") || verb == "?") {
		// please leave each option on its own line so they're
		// easy to sort - bkw
		return
			"a xx      - Set Accumulator to xx\n"
			// "break     - Show all breakpoints\n"
			// "break xx  - Set/clear breakpoint at address xx\n"
			"c         - Toggle Carry Flag\n"
			"d         - Toggle Decimal Flag\n"
			"n         - Toggle Negative Flag\n"
			"pc xx     - Set Program Counter to xx\n"
			"ram       - Show RIOT RAM contents\n"
			"ram xx yy - Set RAM location xx to value yy (multiple values may be given)\n"
			"reset     - Jump to 6502 init vector (does not reset TIA/RIOT)\n"
			"s xx      - Set Stack Pointer to xx\n"
			"step      - Single-step\n"
			"tia       - Show TIA register contents\n"
			"trace     - Single-step treating subroutine calls as one instruction\n"
			"v         - Toggle Overflow Flag\n"
			"x xx      - Set X register to xx\n"
			"y xx      - Set Y register to xx\n"
			"z         - Toggle Zero Flag\n"
			;
	} else {
		return "unimplemented command (try \"help\")";
	}

	result += debugger->state();

	return result;
}

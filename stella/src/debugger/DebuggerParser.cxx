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
// $Id: DebuggerParser.cxx,v 1.1 2005-06-12 18:18:00 stephena Exp $
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
	const char *c = command.c_str();

	cerr << "Parsing \"" << command << "\"" << endl;

	while(*c != '\0') {
		// cerr << "State " << state << ", *c " << *c << endl;
		switch(state) {
			case kIN_COMMAND:
				if(*c == ' ') state = kIN_SPACE;
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

	cerr << endl;
}

string DebuggerParser::run(const string& command) {
	string result;

	getArgs(command);
	if(command == "quit") {
		// TODO: use lookup table to determine which command to run
		debugger->quit();
		return "";
	} else if(command == "trace") {
		debugger->trace();
		result = "OK";
	} else if(command == "ram") {
		result = debugger->dumpRAM(kRamStart);
	} else if(command == "tia") {
		result = debugger->dumpTIA();
	} else {
		result = "unimplemented command";
	}

	result += debugger->state();

	return result;
}

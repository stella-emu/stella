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
// $Id: DebuggerParser.cxx,v 1.3 2005-06-11 20:02:25 urchlay Exp $
//============================================================================

#include "bspf.hxx"
#include "DebuggerParser.hxx"
#include "DebuggerCommand.hxx"
#include "DCmdQuit.hxx"
#include "DCmdTrace.hxx"
#include "D6502.hxx"

DebuggerParser::DebuggerParser(OSystem *os)
  //	: quitCmd(NULL)
{
	done = false;
	quitCmd = new DCmdQuit(this);
	//	changeCmd = new DCmdChange(this);
	//	stepCmd = new DCmdStep(this);
	traceCmd = new DCmdTrace(this);
	myOSystem = os;
}

DebuggerParser::~DebuggerParser() {
	delete quitCmd;
	delete traceCmd;
}

string DebuggerParser::currentAddress() {
	return "currentAddress()";
}

void DebuggerParser::setDone() {
	done = true;
}

OSystem *DebuggerParser::getOSystem() {
	return myOSystem;
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

#define IN_COMMAND 0
#define IN_SPACE 1
#define IN_ARG_START 2
#define IN_ARG_CONT 3

void DebuggerParser::getArgs(const string& command) {
	int state = IN_COMMAND;
	int deref = 0;
	int curArg = 0;
	argCount = 0;
	const char *c = command.c_str();

	cerr << "Parsing \"" << command << "\"" << endl;

	while(*c != '\0') {
		// cerr << "State " << state << ", *c " << *c << endl;
		switch(state) {
			case IN_COMMAND:
				if(*c == ' ') state = IN_SPACE;
				c++;
				break;

			case IN_SPACE:
				if(*c == ' ')
					c++;
				else
					state = IN_ARG_START;
				break;

			case IN_ARG_START:
				curArg = 0;
				state = IN_ARG_CONT;
				if(*c == '*') {
					deref = 1;
					c++;
				} else {
					deref = 0;
				} // FALL THROUGH!

			case IN_ARG_CONT:
				if(isxdigit(*c)) {
					int dig = conv_hex_digit(*c);
					curArg = (curArg << 4) + dig;
					*c++;
				} else {
					args[argCount++] = curArg;
					state = IN_SPACE;
				}
				break;
		}

		if(argCount == 10) {
			cerr << "reached max 10 args" << endl;
			return;
		}
	}

	// pick up last arg, if any:
	if(state == IN_ARG_CONT || state == IN_ARG_START)
		args[argCount++] = curArg;

	// for(int i=0; i<argCount; i++)
		// cerr << "args[" << i << "] == " << args[i] << endl;

	cerr << endl;
}

char *to_hex_8(int i) {
	static char out[3];
	sprintf(out, "%02x", i);
	return out;
}

char *to_hex_16(int i) {
	static char out[5];
	sprintf(out, "%04x", i);
	return out;
}

string DebuggerParser::run(const string& command) {
	char disbuf[255];
	string result;
	// yuck!
	D6502 *dcpu = new D6502(&(myOSystem->console().system()));

	getArgs(command);
	if(command == "quit") {
		// TODO: use lookup table to determine which DebuggerCommand to run
		result = quitCmd->execute(argCount, args);
	} else if(command == "trace") {
		result = traceCmd->execute(argCount, args);
	} else {
		result = "unimplemented command";
	}
	result += "\nPC=";
	result += to_hex_16(dcpu->pc());
	result += " A=";
	result += to_hex_8(dcpu->a());
	result += " X=";
	result += to_hex_8(dcpu->x());
	result += " Y=";
	result += to_hex_8(dcpu->y());
	result += " S=";
	result += to_hex_8(dcpu->sp());
	result += " P=";
	result += to_hex_8(dcpu->ps());
	result += "\n  ";
	dcpu->disassemble(dcpu->pc(), disbuf);
	result += disbuf;

	delete dcpu;
	return result;
}

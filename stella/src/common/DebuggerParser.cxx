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
// $Id: DebuggerParser.cxx,v 1.2 2005-06-09 15:08:22 stephena Exp $
//============================================================================

#include "bspf.hxx"
#include "DebuggerParser.hxx"
#include "DebuggerCommand.hxx"
#include "DCmdQuit.hxx"

DebuggerParser::DebuggerParser()
  : quitCmd(NULL)
{
	done = false;
	quitCmd = new DCmdQuit(this);
}

DebuggerParser::~DebuggerParser() {
	delete quitCmd;
}

string DebuggerParser::currentAddress() {
	return "currentAddress()";
}

void DebuggerParser::setDone() {
	done = true;
}

string DebuggerParser::run(const string& command) {
	if(command == "quit") {
		// TODO: use lookup table to determine which DebuggerCommand to run
		return quitCmd->execute();
	} else {
		return "unimplemented command";
	}
}

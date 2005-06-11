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
// $Id: DebuggerParser.hxx,v 1.3 2005-06-11 20:02:25 urchlay Exp $
//============================================================================

#ifndef DEBUGGER_PARSER_HXX
#define DEBUGGER_PARSER_HXX

#include "bspf.hxx"
#include "DebuggerCommand.hxx"
#include "OSystem.hxx"

class DebuggerParser
{
	public:
		DebuggerParser(OSystem *os);
		~DebuggerParser();

		string currentAddress();
		void setDone();
		string run(const string& command);
		void getArgs(const string& command);
		OSystem *getOSystem();

	private:
		int DebuggerParser::conv_hex_digit(char d);
		DebuggerCommand *changeCmd;
		DebuggerCommand *traceCmd;
		DebuggerCommand *stepCmd;
		DebuggerCommand *quitCmd;
		bool done;
		OSystem *myOSystem;
		int args[10]; // FIXME: should be dynamic
		int argCount;
};

#endif

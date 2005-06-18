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
// $Id: DebuggerParser.hxx,v 1.8 2005-06-18 19:00:44 urchlay Exp $
//============================================================================

#ifndef DEBUGGER_PARSER_HXX
#define DEBUGGER_PARSER_HXX

class Debugger;

#include "bspf.hxx"
#include "EquateList.hxx"

#define kMAX_ARGS 100

class DebuggerParser
{
	public:
		DebuggerParser(Debugger* debugger);
		~DebuggerParser();

		string currentAddress();
		void setDone();
		string run(const string& command);
		bool getArgs(const string& command);

	private:
		int DebuggerParser::conv_hex_digit(char d);
		bool DebuggerParser::subStringMatch(const string& needle, const string& haystack);
		int decipher_arg(string &arg, bool deref, bool lobyte, bool hibyte, bool bin);
		string disasm();
		string listBreaks();
		string eval();

		Debugger* debugger;

		bool done;

		string verb;
		int args[kMAX_ARGS+1]; // FIXME: should be dynamic
		int argCount;
};

#endif

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
// $Id: DebuggerCommand.hxx,v 1.2 2005-06-09 15:08:22 stephena Exp $
//============================================================================

#ifndef DEBUGGER_COMMAND_HXX
#define DEBUGGER_COMMAND_HXX

#include "bspf.hxx"

class DebuggerParser;

class DebuggerCommand
{
	public:
		DebuggerCommand();
		DebuggerCommand(DebuggerParser* p);

		virtual string getName() = 0;
		virtual int getArgCount() = 0;
		virtual string execute() = 0;

	protected:
		DebuggerParser *parser;
};

#endif

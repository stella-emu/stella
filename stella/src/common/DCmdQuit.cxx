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
// $Id: DCmdQuit.cxx,v 1.3 2005-06-11 20:02:25 urchlay Exp $
//============================================================================

#include "bspf.hxx"
#include "DCmdQuit.hxx"
#include "DebuggerParser.hxx"



DCmdQuit::DCmdQuit(DebuggerParser* p) {
	parser = p;
}

string DCmdQuit::getName() {
	return "Quit";
}

int DCmdQuit::getArgCount() {
	return 0;
}

string DCmdQuit::execute(int c, int *args) {
	parser->setDone();
	return "If you quit the debugger, I'll summon Satan all over your hard drive!";
}


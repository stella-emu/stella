
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
// $Id: DCmdTrace.cxx,v 1.1 2005-06-11 20:02:25 urchlay Exp $
//============================================================================

#include "bspf.hxx"
#include "DCmdTrace.hxx"
#include "DebuggerParser.hxx"
#include "Console.hxx"
#include "System.hxx"
#include "M6502.hxx"



DCmdTrace::DCmdTrace(DebuggerParser* p) {
	parser = p;
}

string DCmdTrace::getName() {
	return "Trace";
}

int DCmdTrace::getArgCount() {
	return 0;
}

string DCmdTrace::execute(int c, int *args) {
	parser->getOSystem()->console().system().m6502().execute(1);
	return "OK";
}


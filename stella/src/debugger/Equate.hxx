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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Equate.hxx,v 1.5 2006-12-08 16:49:00 stephena Exp $
//============================================================================

#ifndef EQUATE_HXX
#define EQUATE_HXX

#include "bspf.hxx"

const int EQF_READ = 1;  // Address can be read from
const int EQF_WRITE = 2; // Address can be written to
const int EQF_USER = 4;  // Equate is user-defined, not built-in

// When used in a search, EQF_ANY matches any type of label
const int EQF_ANY = 0;

// When used in a search, EQF_ROM matches only ROM addresses,
// and EQF_RAM only matches RAM addresses. Both RAM and ROM addresses
// are by definition user-defined, since the built-in equates are
// for the TIA and RIOT only.
const int EQF_ROM = EQF_READ | EQF_USER;
const int EQF_RAM = EQF_WRITE | EQF_READ | EQF_USER;

struct Equate {
	string label;
	int address;
	int flags;
};

#endif

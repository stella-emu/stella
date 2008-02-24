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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Equate.hxx,v 1.8 2008-02-24 16:51:52 stephena Exp $
//============================================================================

#ifndef EQUATE_HXX
#define EQUATE_HXX

#include "bspf.hxx"

enum {
  EQF_ANY   = 1 << 0,   // matches any type of label
  EQF_READ  = 1 << 1,   // address can be read from
  EQF_WRITE = 1 << 2,   // address can be written to
  EQF_USER  = 1 << 3,   // equate is user-defined, not built-in

// When used in a search, EQF_ROM matches only ROM addresses,
// and EQF_RAM only matches RAM addresses. Both RAM and ROM addresses
// are by definition user-defined, since the built-in equates are
// for the TIA and RIOT only.
  EQF_ROM = EQF_READ | EQF_USER,
  EQF_RAM = EQF_WRITE | EQF_READ | EQF_USER
};

struct Equate {
  string label;
  int address;
  int flags;
};

#endif

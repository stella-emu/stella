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
// $Id: BankRomCheat.cxx,v 1.2 2005-09-26 19:10:37 stephena Exp $
//============================================================================

#include "BankRomCheat.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BankRomCheat::BankRomCheat(OSystem *os, string code)
{
  myOSystem = os;
  _enabled = false;

  if(code.length() == 7)
    code = "0" + code;

  bank = unhex(code.substr(0, 2));
  address = 0xf000 + unhex(code.substr(2, 3));
  value = unhex(code.substr(5, 2));
  count = unhex(code.substr(7, 1)) + 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BankRomCheat::~BankRomCheat()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BankRomCheat::enabled()
{
  return _enabled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BankRomCheat::enable()
{
  int oldBank = myOSystem->console().cartridge().bank();
  myOSystem->console().cartridge().bank(bank);

  for(int i=0; i<count; i++)
  {
    savedRom[i] = myOSystem->console().cartridge().peek(address + i);
    myOSystem->console().cartridge().patch(address + i, value);
  }
  myOSystem->console().cartridge().bank(oldBank);

  return _enabled = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BankRomCheat::disable()
{
  int oldBank = myOSystem->console().cartridge().bank();
  myOSystem->console().cartridge().bank(bank);
  for(int i=0; i<count; i++)
		myOSystem->console().cartridge().patch(address + i, savedRom[i]);

  myOSystem->console().cartridge().bank(oldBank);

  return _enabled = false;
}

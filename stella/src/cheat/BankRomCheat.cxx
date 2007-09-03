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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: BankRomCheat.cxx,v 1.4 2007-09-03 18:37:19 stephena Exp $
//============================================================================

#include "Console.hxx"
#include "BankRomCheat.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BankRomCheat::BankRomCheat(OSystem* os, const string& name, const string& code)
  : Cheat(os, name, code)
{
  if(myCode.length() == 7)
    myCode = "0" + code;

  bank = unhex(myCode.substr(0, 2));
  address = 0xf000 + unhex(myCode.substr(2, 3));
  value = unhex(myCode.substr(5, 2));
  count = unhex(myCode.substr(7, 1)) + 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BankRomCheat::~BankRomCheat()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BankRomCheat::enable()
{
  evaluate();
  return myEnabled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BankRomCheat::disable()
{
  int oldBank = myOSystem->console().cartridge().bank();
  myOSystem->console().cartridge().bank(bank);
  for(int i=0; i<count; i++)
		myOSystem->console().cartridge().patch(address + i, savedRom[i]);

  myOSystem->console().cartridge().bank(oldBank);

  return myEnabled = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BankRomCheat::evaluate()
{
  if(!myEnabled)
  {
    int oldBank = myOSystem->console().cartridge().bank();
    myOSystem->console().cartridge().bank(bank);

    for(int i=0; i<count; i++)
    {
      savedRom[i] = myOSystem->console().cartridge().peek(address + i);
      myOSystem->console().cartridge().patch(address + i, value);
    }
    myOSystem->console().cartridge().bank(oldBank);

    myEnabled = true;
  }
}

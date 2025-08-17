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
// Copyright (c) 1995-2024 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Console.hxx"
#include "Cart.hxx"
#include "OSystem.hxx"
#include "PatchRomCheat.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PatchRomCheat::PatchRomCheat(OSystem& os, string_view name, string_view code)
  : Cheat(os, name, code)
{
  address = static_cast<uInt16>(BSPF::stoi<16>(myCode.substr(0, 4)));
  new_value = static_cast<uInt8>(BSPF::stoi<16>(myCode.substr(4, 2)));
  original_value = static_cast<uInt8>(BSPF::stoi<16>(myCode.substr(6, 2)));
  throwaway = static_cast<uInt8>(BSPF::stoi<16>(myCode.substr(8, 1)) + 1); //used to make it an 9 number code only
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PatchRomCheat::enable()
{
  evaluate();
  return myEnabled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PatchRomCheat::disable()
{
  if(myEnabled && myOSystem.console().cartridge().peek(address) == new_value)
	myOSystem.console().cartridge().patch(address, original_value);
 
  return myEnabled = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PatchRomCheat::evaluate()
{
  if(!myEnabled && myOSystem.console().cartridge().peek(address) == original_value)
  {
    myOSystem.console().cartridge().patch(address, new_value);
	myEnabled = true;
  }
}

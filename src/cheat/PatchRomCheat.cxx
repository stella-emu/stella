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
// Copyright (c) 1995-2025 by Bradford W. Mott, Stephen Anthony
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
  : Cheat(os, name, code),
    address{static_cast<uInt16>(BSPF::stoi<16>(myCode.substr(0, 4)))},
    new_value{static_cast<uInt8>(BSPF::stoi<16>(myCode.substr(4, 2)))},
    original_value{static_cast<uInt8>(BSPF::stoi<16>(myCode.substr(6, 2)))}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PatchRomCheat::enable()
{
  //Need to add a better check here so that it only euns evaluates if actual cart size is => address
  if (myOSystem.console().cartridge().maxSize() >= address)
    evaluate();

  return myEnabled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PatchRomCheat::disable()
{
  //Need to add a better check here so that it only does this if actual cart size is => address
  if (myOSystem.console().cartridge().maxSize() >= address)
  {
    if (myEnabled && myOSystem.console().cartridge().peek(address, false) == new_value)
      myOSystem.console().cartridge().patch(address, original_value, false);
  }
  return myEnabled = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PatchRomCheat::evaluate()
{
  if (myOSystem.console().cartridge().peek(address, false) == original_value)
  {
    myOSystem.console().cartridge().patch(address, new_value, false);
    myEnabled = true;
  }
  else if (myOSystem.console().cartridge().peek(address, false) == new_value)
  {
    myEnabled = true;
  }
  else
  {
    myEnabled = false; //There's an error with the cheat so dont poke it
  }
}

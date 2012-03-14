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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "Control.hxx"
#include "System.hxx"
#include "CompuMate.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompuMate::CompuMate(CartridgeCM& cart, const Event& event,
                     const System& system)
  : myCart(cart),
    myEvent(event),
    mySystem(system),
    myLeftController(0),
    myRightController(0),
    myCycleAtLastUpdate(0),
    myColumn(0)
{
  myLeftController = new CMControl(*this, Controller::Left, event, system);
  myRightController = new CMControl(*this, Controller::Right, event, system);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompuMate::update()
{
  uInt32 cycle = mySystem.cycles();

  // Only perform update once for both ports in the same cycle
  if(myCycleAtLastUpdate != cycle)
  {
    myCycleAtLastUpdate = cycle;
    return;
  }
  myCycleAtLastUpdate = cycle;

  // Handle SWCHA changes - the following comes almost directly from z26
  Controller& lp = *myLeftController;
  Controller& rp = *myRightController;
  uInt8 IOPortA = (lp.read() << 4) | rp.read();

  lp.myAnalogPinValue[Controller::Nine] = Controller::maximumResistance;
  lp.myAnalogPinValue[Controller::Five] = Controller::minimumResistance;
  lp.myDigitalPinState[Controller::Six] = true;

  rp.myAnalogPinValue[Controller::Nine] = Controller::minimumResistance;
  rp.myAnalogPinValue[Controller::Five] = Controller::maximumResistance;
  rp.myDigitalPinState[Controller::Six] = true;

  uInt8& column = myCart.myColumn;
  if(IOPortA & 0x20) column = 0;
  if(IOPortA & 0x40) column = (column + 1) % 10;

  IOPortA = IOPortA | (0x0c & 0x7f);

#if 0
  switch(column)
  {
    case 0:
      if (KeyTable[Key7]) lp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeyU]) IOPortA = IOPortA & 0xfb;
      if (KeyTable[KeyJ]) rp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeyM]) IOPortA = IOPortA & 0xf7;
      break;
    case 1:
      if (KeyTable[Key6]) lp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeyY]) IOPortA = IOPortA & 0xfb;
      if (KeyTable[KeyH]) rp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeyN]) IOPortA = IOPortA & 0xf7;
      break;
    case 2:
      if (KeyTable[Key8]) lp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeyI]) IOPortA = IOPortA & 0xfb;
      if (KeyTable[KeyK]) rp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeyComma]) IOPortA = IOPortA & 0xf7;
      break;
    case 3:
      if (KeyTable[Key2]) lp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeyW]) IOPortA = IOPortA & 0xfb;
      if (KeyTable[KeyS]) rp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeyX]) IOPortA = IOPortA & 0xf7;
      break;
    case 4:
      if (KeyTable[Key3]) lp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeyE]) IOPortA = IOPortA & 0xfb;
      if (KeyTable[KeyD]) rp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeyC]) IOPortA = IOPortA & 0xf7;
      break;
    case 5:
      if (KeyTable[Key0]) lp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeyP]) IOPortA = IOPortA & 0xfb;
      if (KeyTable[KeyColon]) rp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeySlash]) IOPortA = IOPortA & 0xf7;
      break;
    case 6:
      if (KeyTable[Key9]) lp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeyO]) IOPortA = IOPortA & 0xfb;
      if (KeyTable[KeyL]) rp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeyDot]) IOPortA = IOPortA & 0xf7;
      break;
    case 7:
      if (KeyTable[Key5]) lp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeyT]) IOPortA = IOPortA & 0xfb;
      if (KeyTable[KeyG]) rp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeyB]) IOPortA = IOPortA & 0xf7;
      break;
    case 8:
      if (KeyTable[Key1]) lp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeyQ]) IOPortA = IOPortA & 0xfb;
      if (KeyTable[KeyA]) rp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeyZ]) IOPortA = IOPortA & 0xf7;
      break;
    case 9:
      if (KeyTable[Key4]) lp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeyR]) IOPortA = IOPortA & 0xfb;
      if (KeyTable[KeyF]) rp.myDigitalPinState[Controller::Six] = false;
      if (KeyTable[KeyV]) IOPortA = IOPortA & 0xf7;
      break;
    default:
      break;
  }
#endif
  // Convert back to digital pins
  rp.myDigitalPinState[Controller::One]   = IOPortA & 0x01;
  rp.myDigitalPinState[Controller::Two]   = IOPortA & 0x02;
  rp.myDigitalPinState[Controller::Three] = IOPortA & 0x04;
  rp.myDigitalPinState[Controller::Four]  = IOPortA & 0x08;
}

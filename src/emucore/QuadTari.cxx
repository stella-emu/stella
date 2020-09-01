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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Event.hxx"
#include "System.hxx"
#include "TIA.hxx"
#include "FrameBuffer.hxx"
#include "Joystick.hxx"
#include "QuadTari.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTari::QuadTari(Jack jack, const Event& event, const System& system)
  : Controller(jack, event, system, Controller::Type::QuadTari)
{
  // TODO: support multiple controller types
  if(myJack == Jack::Left)
  {
    myFirstController = make_unique<Joystick>(Jack::Left, event, system);
    mySecondController = make_unique<Joystick>(Jack::Right, event, system); // TODO: use P2 mapping
  }
  else
  {
    myFirstController = make_unique<Joystick>(Jack::Right, event, system);
    mySecondController = make_unique<Joystick>(Jack::Left, event, system); // TODO: use P3 mapping
  }
  // QuadTari auto detection setting
  setPin(AnalogPin::Five, MIN_RESISTANCE);
  setPin(AnalogPin::Nine, MAX_RESISTANCE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTari::read(DigitalPin pin)
{
  // We need to override the Controller::read() method, since the QuadTari
  // can switch the controller multiple times per frame
  // (we can't just read 60 times per second in the ::update() method)

  // If bit 7 of VBlank is not set, read first, else second controller
  if(!(mySystem.tia().registerValue(VBLANK) & 0x80))
    return myFirstController->read(pin);
  else
    return mySecondController->read(pin);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTari::update()
{
  myFirstController->update();
  mySecondController->update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string QuadTari::name() const
{
  return "QuadTari (" + myFirstController->name() + "/" + mySecondController->name() + ")";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTari::isAnalog() const
{
  // For now, use mouse for first controller only
  return myFirstController->isAnalog();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTari::setMouseControl(
    Controller::Type xtype, int xid, Controller::Type ytype, int yid)
{
  // Use mouse for first controller only (TODO: support multiple controller types)
  myFirstController->setMouseControl(Controller::Type::Joystick, xid, Controller::Type::Joystick, yid);

  return true;
}

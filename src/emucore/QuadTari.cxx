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
QuadTari::QuadTari(Jack jack, const Event& event, const System& system,
                   const Controller::Type firstType, const Controller::Type secondType)
  : Controller(jack, event, system, Controller::Type::QuadTari)
{
  // TODO: support multiple controller types
  switch(firstType)
  {
    case Controller::Type::Joystick:
      myFirstController = make_unique<Joystick>(myJack, event, system);
      break;

    default:
      // TODO
      break;
  }
  switch(secondType)
  {
    case Controller::Type::Joystick:
      mySecondController = make_unique<Joystick>(myJack, event, system, true); // use alternative mapping
      break;

    default:
      // TODO
      break;
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

  constexpr int MIN_CYCLES = 20 * 76; // minimal cycles required for stable input switch (TODO: define cycles)
  bool readFirst;

  if(mySystem.tia().dumpPortsCycles() < MIN_CYCLES)
    // Random controller if read too soon after dump ports changed
    readFirst = mySystem.randGenerator().next() % 2;
  else
    // If bit 7 of VBlank is not set, read first, else second controller
    readFirst = !(mySystem.tia().registerValue(VBLANK) & 0x80);

  if(readFirst)
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
  // Use mouse for first controller only
  if(xtype == Controller::Type::QuadTari && ytype == Controller::Type::QuadTari)
    return myFirstController->setMouseControl(myFirstController->type(), xid,
                                              myFirstController->type(), yid);
  else
    // required for creating the MouseControl mode list
    return myFirstController->setMouseControl(xtype, xid, ytype, yid);
}

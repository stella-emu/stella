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
#include "Joystick.hxx"
#include "QuadTari.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTari::QuadTari(Jack jack, const Event& event, const System& system)
  : Controller(jack, event, system, Controller::Type::QuadTari)
{
  // TODO: allow multiple controller types
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTari::read(DigitalPin pin)
{
  // We need to override the Controller::read() method, since the QuadTari
  // can switch the controller multiple times per frame
  // (we can't just read 60 times per second in the ::update() method)

  if(true) // TODO handle controller switch
    return myFirstController->read(pin);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTari::update()
{
  myFirstController->update();
  mySecondController->update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTari::isAnalog() const
{
  // TODO: does this work?
  return myFirstController->isAnalog() || mySecondController->isAnalog();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTari::setMouseControl(
    Controller::Type xtype, int xid, Controller::Type ytype, int yid)
{
  // TODO: does this work?
  myFirstController->setMouseControl(xtype, xid, ytype, yid);
  mySecondController->setMouseControl(xtype, xid, ytype, yid);

  return true;
}

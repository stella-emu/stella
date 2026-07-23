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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Joy2BPlus.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Joy2BPlus::Joy2BPlus(Jack jack, const Event& event, const System& system)
  : Joystick(jack, event, system, Controller::Type::Joy2BPlus)
{
  if(myJack == Jack::Left)
  {
    myButtonCEvent = Event::LeftJoystickFire5;
    myButton3Event = Event::LeftJoystickFire9;
  }
  else
  {
    myButtonCEvent = Event::RightJoystickFire5;
    myButton3Event = Event::RightJoystickFire9;
  }

  setPin(AnalogPin::Nine, AnalogReadout::connectToVcc());
  setPin(AnalogPin::Five, AnalogReadout::connectToVcc());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Joy2BPlus::updateButtons()
{
  // The single fire button is also triggered by the left mouse button
  updateFire(false);

  // The Joy 2B+ has two more buttons, connected to the inputs usually used by
  // paddles; button C also responds to the right mouse button.  Both are
  // analog inputs, so they stay static (not replayed within the input window).
  const bool buttonCPressed = myEvent.get(myButtonCEvent) != 0 ||
      mousePressed(Event::MouseButtonRightValue);
  const bool button3Pressed = myEvent.get(myButton3Event) != 0;
  setPin(AnalogPin::Nine, buttonCPressed
                            ? AnalogReadout::connectToGround()
                            : AnalogReadout::connectToVcc());
  setPin(AnalogPin::Five, button3Pressed
                            ? AnalogReadout::connectToGround()
                            : AnalogReadout::connectToVcc());
}

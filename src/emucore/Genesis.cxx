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

#include "Genesis.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Genesis::Genesis(Jack jack, const Event& event, const System& system)
  : Joystick(jack, event, system, Controller::Type::Genesis),
    myButtonCEvent{(myJack == Jack::Left) ? Event::LeftJoystickFire5
                                          : Event::RightJoystickFire5}
{
  setPin(AnalogPin::Five, AnalogReadout::connectToVcc());
  setPin(AnalogPin::Nine, AnalogReadout::connectToVcc());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Genesis::updateButtons()
{
  // The single fire button is also triggered by the left mouse button
  updateFire(false);

  // The Genesis has one more button (C) that can be read by the 2600, but with
  // inverted logic compared to the BoosterGrip.  It also responds to the right
  // mouse button.  Being an analog input, it stays static (not replayed within
  // the input window).
  const bool buttonCPressed = myEvent.get(myButtonCEvent) != 0 ||
      mousePressed(Event::MouseButtonRightValue);
  setPin(AnalogPin::Five, buttonCPressed
                            ? AnalogReadout::connectToGround()
                            : AnalogReadout::connectToVcc());
}

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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Lightgun.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Lightgun::Lightgun(Jack jack, const Event& event, const System& system)
  : Controller(jack, event, system, Controller::Type::Lightgun)
{
  if (myJack == Jack::Left)
  {
    myFireEvent = Event::JoystickZeroFire;
    //myFireEvent = Event::LightgunZeroFire;
    myPosValue = Event::LightgunZeroPos;
  }
  else
  {
    myFireEvent = Event::JoystickOneFire;
    //myFireEvent = Event::LightgunOneFire;
    myPosValue = Event::LightgunOnePos;
  }

  // Digital pins 1, 2 and 6 are not connected (TOOD: check)
  //setPin(DigitalPin::One, true);
  //setPin(DigitalPin::Two, true);
  //setPin(DigitalPin::Six, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Lightgun::update()
{
  // |              | Left port   | Right port  |
  // | Fire button  | SWCHA bit 4 | SWCHA bit 0 | DP:1
  // | Detect light | INPT4 bit 7 | INPT5 bit 7 | DP:6

  myCharge = BSPF::clamp(myEvent.get(Event::MouseAxisXValue) * MOUSE_SENSITIVITY, TRIGMIN, TRIGMAX);

  if (myCharge != myLastCharge)
  {
    setPin(DigitalPin::Six, Int32(MAX_RESISTANCE * (myCharge / double(TRIGMAX))));
    myLastCharge = myCharge;
  }

  setPin(DigitalPin::One, myEvent.get(Event::MouseButtonLeftValue) ? false : true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Lightgun::setMouseSensitivity(int sensitivity)
{
  MOUSE_SENSITIVITY = BSPF::clamp(sensitivity, 1, MAX_MOUSE_SENSE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Lightgun::MOUSE_SENSITIVITY = -1;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//const Controller::DigitalPin Lightgun::ourButtonPin = DigitalPin::One; // TODO


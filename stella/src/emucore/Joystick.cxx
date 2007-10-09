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
// $Id: Joystick.cxx,v 1.9 2007-10-09 23:56:57 stephena Exp $
//============================================================================

#include "Event.hxx"
#include "Joystick.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Joystick::Joystick(Jack jack, const Event& event)
  : Controller(jack, event, Controller::Joystick)
{
  if(myJack == Left)
  {
    myUpEvent    = Event::JoystickZeroUp;
    myDownEvent  = Event::JoystickZeroDown;
    myLeftEvent  = Event::JoystickZeroLeft;
    myRightEvent = Event::JoystickZeroRight;
    myFireEvent  = Event::JoystickZeroFire1;
  }
  else
  {
    myUpEvent    = Event::JoystickOneUp;
    myDownEvent  = Event::JoystickOneDown;
    myLeftEvent  = Event::JoystickOneLeft;
    myRightEvent = Event::JoystickOneRight;
    myFireEvent  = Event::JoystickOneFire1;
  }

  // Analog pins are never used by the joystick
  myAnalogPinValue[Five] = myAnalogPinValue[Nine] = maximumResistance;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Joystick::~Joystick()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Joystick::update()
{
  myDigitalPinState[One]   = (myEvent.get(myUpEvent) == 0);
  myDigitalPinState[Two]   = (myEvent.get(myDownEvent) == 0);
  myDigitalPinState[Three] = (myEvent.get(myLeftEvent) == 0);
  myDigitalPinState[Four]  = (myEvent.get(myRightEvent) == 0);
  myDigitalPinState[Six]   = (myEvent.get(myFireEvent) == 0);
}

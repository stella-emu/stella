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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Driving.cxx,v 1.15 2008-03-02 19:20:50 stephena Exp $
//============================================================================

#include "Event.hxx"
#include "System.hxx"

#include "Driving.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Driving::Driving(Jack jack, const Event& event)
  : Controller(jack, event, Controller::Driving),
    myCounter(0)
{
  if(myJack == Left)
  {
    myCCWEvent   = Event::JoystickZeroLeft;
    myCWEvent    = Event::JoystickZeroRight;
    myFireEvent  = Event::JoystickZeroFire1;
    myXAxisValue = Event::SALeftAxis0Value;
    myYAxisValue = Event::SALeftAxis1Value;
  }
  else
  {
    myCCWEvent   = Event::JoystickOneLeft;
    myCWEvent    = Event::JoystickOneRight;
    myFireEvent  = Event::JoystickOneFire1;
    myXAxisValue = Event::SARightAxis0Value;
    myYAxisValue = Event::SARightAxis1Value;
  }

  // Digital pins 3 and 4 are not connected
  myDigitalPinState[Three] = myDigitalPinState[Four] = true;

  // Analog pins are not connected
  myAnalogPinValue[Five] = myAnalogPinValue[Nine] = maximumResistance;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Driving::~Driving()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Driving::update()
{
  int grayIndex = 0;

  // Digital events (from keyboard or joystick hats & buttons)
  myDigitalPinState[Six] = (myEvent.get(myFireEvent) == 0);

  int xaxis = myEvent.get(myXAxisValue);
  if(myEvent.get(myCCWEvent) != 0 || xaxis < -16384)     myCounter--;
  else if(myEvent.get(myCWEvent) != 0 || xaxis > 16384)  myCounter++;

  // Only consider the lower-most bits (corresponding to pins 1 & 2)
  myCounter &= 0x0f;
  grayIndex = myCounter >> 2;
 
  // Stelladaptor is the only controller that should set this
  int yaxis = myEvent.get(myYAxisValue);
  if(yaxis <= -16384-4096)
    grayIndex = 2;
  else if(yaxis > 16384+4096)
    grayIndex = 1;
  else if(yaxis >= 16384-4096)
    grayIndex = 0;
  else if(yaxis < -16384+4096)
    grayIndex = 3;
  
  // Gray codes for rotation
  static const uInt8 graytable[] = { 0x03, 0x01, 0x00, 0x02 };

  // Determine which bits are set
  uInt8 gray = graytable[grayIndex];
  myDigitalPinState[One] = (gray & 0x1) != 0;
  myDigitalPinState[Two] = (gray & 0x2) != 0;
}

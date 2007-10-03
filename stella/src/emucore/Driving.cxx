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
// $Id: Driving.cxx,v 1.12 2007-10-03 21:41:17 stephena Exp $
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
    myFireEvent  = Event::JoystickZeroFire;
    myValueEvent = Event::DrivingZeroValue;
  }
  else
  {
    myCCWEvent   = Event::JoystickOneLeft;
    myCWEvent    = Event::JoystickOneRight;
    myFireEvent  = Event::JoystickOneFire;
    myValueEvent = Event::DrivingOneValue;
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
  // TODO - this isn't working with Stelladaptor and real driving controllers

  // Gray codes for rotation
  static const uInt8 graytable[] = { 0x03, 0x01, 0x00, 0x02 };

  // Determine which gray code we're at
  if(myEvent.get(myCCWEvent) != 0)
    myCounter--;
  else if(myEvent.get(myCWEvent) != 0)
    myCounter++;

  // Only consider the lower-most bits (corresponding to pins 1 & 2)
  myCounter &= 0x0f;
  uInt8 gray = graytable[myCounter >> 2];

  // Determine which bits are set
  myDigitalPinState[One] = (gray & 0x1) != 0;
  myDigitalPinState[Two] = (gray & 0x2) != 0;

  myDigitalPinState[Six] = (myEvent.get(myFireEvent) == 0);
}


/*
bool Driving::read(DigitalPin pin)
{
  // Gray codes for clockwise rotation
  static const uInt8 clockwise[] = { 0x03, 0x01, 0x00, 0x02 };

  // Gray codes for counter-clockwise rotation
  static const uInt8 counterclockwise[] = { 0x03, 0x02, 0x00, 0x01 };

  // Delay used for moving through the gray code tables
  const uInt32 delay = 20;

  switch(pin)
  {
    case One:
      ++myCounter;

      if(myJack == Left)
      {
        if(myEvent.get(Event::DrivingZeroCounterClockwise) != 0)
        {
          return (counterclockwise[(myCounter / delay) & 0x03] & 0x01) != 0;
        }
        else if(myEvent.get(Event::DrivingZeroClockwise) != 0)
        {
          return (clockwise[(myCounter / delay) & 0x03] & 0x01) != 0;
        }
		else 
		  return(myEvent.get(Event::DrivingZeroValue) & 0x01);
      }
      else
      {
        if(myEvent.get(Event::DrivingOneCounterClockwise) != 0)
        {
          return (counterclockwise[(myCounter / delay) & 0x03] & 0x01) != 0;
        }
        else if(myEvent.get(Event::DrivingOneClockwise) != 0)
        {
          return (clockwise[(myCounter / delay) & 0x03] & 0x01) != 0;
        }
		else 
		  return(myEvent.get(Event::DrivingOneValue) & 0x01);
      }

    case Two:
      if(myJack == Left)
      {
        if(myEvent.get(Event::DrivingZeroCounterClockwise) != 0)
        {
          return (counterclockwise[(myCounter / delay) & 0x03] & 0x02) != 0;
        }
        else if(myEvent.get(Event::DrivingZeroClockwise) != 0)
        {
          return (clockwise[(myCounter / delay) & 0x03] & 0x02) != 0;
        }
		else 
		  return(myEvent.get(Event::DrivingZeroValue) & 0x02);
      }
      else
      {
        if(myEvent.get(Event::DrivingOneCounterClockwise) != 0)
        {
          return (counterclockwise[(myCounter / delay) & 0x03] & 0x02) != 0;
        }
        else if(myEvent.get(Event::DrivingOneClockwise) != 0)
        {
          return (clockwise[(myCounter / delay) & 0x03] & 0x02) != 0;
        }
		else 
		  return(myEvent.get(Event::DrivingOneValue) & 0x02);
      }
  }
}
*/

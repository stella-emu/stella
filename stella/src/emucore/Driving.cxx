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
// Copyright (c) 1995-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Driving.cxx,v 1.1.1.1 2001-12-27 19:54:21 bwmott Exp $
//============================================================================

#include <assert.h>
#include "Event.hxx"
#include "Driving.hxx"
#include "System.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Driving::Driving(Jack jack, const Event& event)
    : Controller(jack, event)
{
  myCounter = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Driving::~Driving()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
        if(myEvent.get(Event::JoystickZeroLeft) != 0)
        {
          return (counterclockwise[(myCounter / delay) & 0x03] & 0x01) != 0;
        }
        else if(myEvent.get(Event::JoystickZeroRight) != 0)
        {
          return (clockwise[(myCounter / delay) & 0x03] & 0x01) != 0;
        }
      }
      else
      {
        if(myEvent.get(Event::JoystickOneLeft) != 0)
        {
          return (counterclockwise[(myCounter / delay) & 0x03] & 0x01) != 0;
        }
        else if(myEvent.get(Event::JoystickOneRight) != 0)
        {
          return (clockwise[(myCounter / delay) & 0x03] & 0x01) != 0;
        }
      }

    case Two:
      if(myJack == Left)
      {
        if(myEvent.get(Event::JoystickZeroLeft) != 0)
        {
          return (counterclockwise[(myCounter / delay) & 0x03] & 0x02) != 0;
        }
        else if(myEvent.get(Event::JoystickZeroRight) != 0)
        {
          return (clockwise[(myCounter / delay) & 0x03] & 0x02) != 0;
        }
      }
      else
      {
        if(myEvent.get(Event::JoystickOneLeft) != 0)
        {
          return (counterclockwise[(myCounter / delay) & 0x03] & 0x02) != 0;
        }
        else if(myEvent.get(Event::JoystickOneRight) != 0)
        {
          return (clockwise[(myCounter / delay) & 0x03] & 0x02) != 0;
        }
      }

    case Three:
      return true;

    case Four:
      return true;

    case Six:
      return (myJack == Left) ? (myEvent.get(Event::JoystickZeroFire) == 0) : 
          (myEvent.get(Event::JoystickOneFire) == 0);

    default:
      return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 Driving::read(AnalogPin)
{
  // Analog pins are not connect in driving controller so we have 
  // infinite resistance 
  return maximumResistance;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Driving::write(DigitalPin, bool)
{
  // Writing doesn't do anything to the driving controller...
}


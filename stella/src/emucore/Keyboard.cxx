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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Keyboard.cxx,v 1.2 2005-06-16 01:11:27 stephena Exp $
//============================================================================

#include "Event.hxx"
#include "Keyboard.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Keyboard::Keyboard(Jack jack, const Event& event)
    : Controller(jack, event),
      myPinState(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Keyboard::~Keyboard()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Keyboard::read(DigitalPin pin)
{
  if(pin == Six)
  {
    if((myPinState & 0x01) == 0)
    {
      return (myJack == Left) ? (myEvent.get(Event::KeyboardZero3) == 0) :
          (myEvent.get(Event::KeyboardOne3) == 0);
    }
    else if((myPinState & 0x02) == 0)
    {
      return (myJack == Left) ? (myEvent.get(Event::KeyboardZero6) == 0) :
          (myEvent.get(Event::KeyboardOne6) == 0);
    }
    else if((myPinState & 0x04) == 0)
    {
      return (myJack == Left) ? (myEvent.get(Event::KeyboardZero9) == 0) :
          (myEvent.get(Event::KeyboardOne9) == 0);
    }
    else if((myPinState & 0x08) == 0)
    {
      return (myJack == Left) ? (myEvent.get(Event::KeyboardZeroPound) == 0) :
          (myEvent.get(Event::KeyboardOnePound) == 0);
    }
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 Keyboard::read(AnalogPin pin)
{
  if(pin == Nine)
  {
    if((myPinState & 0x01) == 0)
    {
      if(myJack == Left)
      {
        return (myEvent.get(Event::KeyboardZero1) != 0) ? 
            maximumResistance : minimumResistance;
      }
      else
      {
        return (myEvent.get(Event::KeyboardOne1) != 0) ? 
            maximumResistance : minimumResistance;
      }
    }
    else if((myPinState & 0x02) == 0)
    {
      if(myJack == Left)
      {
        return (myEvent.get(Event::KeyboardZero4) != 0) ? 
            maximumResistance : minimumResistance;
      }
      else
      {
        return (myEvent.get(Event::KeyboardOne4) != 0) ? 
            maximumResistance : minimumResistance;
      }
    }
    else if((myPinState & 0x04) == 0)
    {
      if(myJack == Left)
      {
        return (myEvent.get(Event::KeyboardZero7) != 0) ? 
            maximumResistance : minimumResistance;
      }
      else
      {
        return (myEvent.get(Event::KeyboardOne7) != 0) ? 
            maximumResistance : minimumResistance;
      }
    }
    else if((myPinState & 0x08) == 0)
    {
      if(myJack == Left)
      {
        return (myEvent.get(Event::KeyboardZeroStar) != 0) ? 
            maximumResistance : minimumResistance;
      }
      else
      {
        return (myEvent.get(Event::KeyboardOneStar) != 0) ? 
            maximumResistance : minimumResistance;
      }
    }
  }
  else
  {
    if((myPinState & 0x01) == 0)
    {
      if(myJack == Left)
      {
        return (myEvent.get(Event::KeyboardZero2) != 0) ? 
            maximumResistance : minimumResistance;
      }
      else
      {
        return (myEvent.get(Event::KeyboardOne2) != 0) ? 
            maximumResistance : minimumResistance;
      }
    }
    else if((myPinState & 0x02) == 0)
    {
      if(myJack == Left)
      {
        return (myEvent.get(Event::KeyboardZero5) != 0) ? 
            maximumResistance : minimumResistance;
      }
      else
      {
        return (myEvent.get(Event::KeyboardOne5) != 0) ? 
            maximumResistance : minimumResistance;
      }
    }
    else if((myPinState & 0x04) == 0)
    {
      if(myJack == Left)
      {
        return (myEvent.get(Event::KeyboardZero8) != 0) ? 
            maximumResistance : minimumResistance;
      }
      else
      {
        return (myEvent.get(Event::KeyboardOne8) != 0) ? 
            maximumResistance : minimumResistance;
      }
    }
    else if((myPinState & 0x08) == 0)
    {
      if(myJack == Left)
      {
        return (myEvent.get(Event::KeyboardZero0) != 0) ? 
            maximumResistance : minimumResistance;
      }
      else
      {
        return (myEvent.get(Event::KeyboardOne0) != 0) ? 
            maximumResistance : minimumResistance;
      }
    }
  }

  return maximumResistance;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Keyboard::write(DigitalPin pin, bool value)
{
  // Change the pin state based on value
  switch(pin)
  {
    case One:
      myPinState = (myPinState & 0x0E) | (value ? 0x01 : 0x00);
      break;
  
    case Two:
      myPinState = (myPinState & 0x0D) | (value ? 0x02 : 0x00);
      break;

    case Three:
      myPinState = (myPinState & 0x0B) | (value ? 0x04 : 0x00);
      break;
  
    case Four:
      myPinState = (myPinState & 0x07) | (value ? 0x08 : 0x00);
      break;

    default:
      break;
  } 
}


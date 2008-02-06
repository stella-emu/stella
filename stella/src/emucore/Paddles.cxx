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
// $Id: Paddles.cxx,v 1.11 2008-02-06 13:45:22 stephena Exp $
//============================================================================

#include "Event.hxx"
#include "Paddles.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Paddles::Paddles(Jack jack, const Event& event, bool swap)
  : Controller(jack, event, Controller::Paddles)
{
  // Swap the paddle events, from paddle 0 <=> 1 and paddle 2 <=> 3
  // Also consider whether this is the left or right port
  if(myJack == Left)
  {
    if(swap)
    {
      myP0ResEvent   = Event::PaddleZeroResistance;
      myP0FireEvent1 = Event::PaddleZeroFire;
      myP0FireEvent2 = Event::JoystickZeroFire1;

      myP1ResEvent   = Event::PaddleOneResistance;
      myP1FireEvent1 = Event::PaddleOneFire;
      myP1FireEvent2 = Event::JoystickZeroFire3;
    }
    else
    {
      myP0ResEvent   = Event::PaddleOneResistance;
      myP0FireEvent1 = Event::PaddleOneFire;
      myP0FireEvent2 = Event::JoystickZeroFire3;

      myP1ResEvent   = Event::PaddleZeroResistance;
      myP1FireEvent1 = Event::PaddleZeroFire;
      myP1FireEvent2 = Event::JoystickZeroFire1;
    }
  }
  else
  {
    if(swap)
    {
      myP0ResEvent   = Event::PaddleTwoResistance;
      myP0FireEvent1 = Event::PaddleTwoFire;
      myP0FireEvent2 = Event::JoystickOneFire1;

      myP1ResEvent   = Event::PaddleThreeResistance;
      myP1FireEvent1 = Event::PaddleThreeFire;
      myP1FireEvent2 = Event::JoystickOneFire3;
    }
    else
    {
      myP0ResEvent   = Event::PaddleThreeResistance;
      myP0FireEvent1 = Event::PaddleThreeFire;
      myP0FireEvent2 = Event::JoystickOneFire3;

      myP1ResEvent   = Event::PaddleTwoResistance;
      myP1FireEvent1 = Event::PaddleTwoFire;
      myP1FireEvent2 = Event::JoystickOneFire1;
    }
  }

  // Digital pins 1, 2 and 6 are not connected
  myDigitalPinState[One] =
  myDigitalPinState[Two] =
  myDigitalPinState[Six] = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Paddles::~Paddles()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::update()
{
  myDigitalPinState[Three] =
    (myEvent.get(myP0FireEvent1) == 0 && myEvent.get(myP0FireEvent2) == 0);
  myDigitalPinState[Four]  =
    (myEvent.get(myP1FireEvent1) == 0 && myEvent.get(myP1FireEvent2) == 0);

  myAnalogPinValue[Five] = myEvent.get(myP0ResEvent);
  myAnalogPinValue[Nine] = myEvent.get(myP1ResEvent);
}

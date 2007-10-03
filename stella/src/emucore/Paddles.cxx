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
// $Id: Paddles.cxx,v 1.9 2007-10-03 21:41:18 stephena Exp $
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
    myP1ResEvent =
      swap ? Event::PaddleZeroResistance : Event::PaddleOneResistance;
    myP2ResEvent =
      swap ? Event::PaddleOneResistance : Event::PaddleZeroResistance;
    myP1FireEvent = swap ? Event::PaddleZeroFire : Event::PaddleOneFire;
    myP2FireEvent = swap ? Event::PaddleOneFire : Event::PaddleZeroFire;
  }
  else
  {
    myP1ResEvent =
      swap ? Event::PaddleTwoResistance : Event::PaddleThreeResistance;
    myP2ResEvent =
      swap ? Event::PaddleThreeResistance : Event::PaddleTwoResistance;
    myP1FireEvent = swap ? Event::PaddleTwoFire : Event::PaddleThreeFire;
    myP2FireEvent = swap ? Event::PaddleThreeFire : Event::PaddleTwoFire;
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
  myDigitalPinState[Three] = (myEvent.get(myP1FireEvent) == 0);
  myDigitalPinState[Four]  = (myEvent.get(myP2FireEvent) == 0);

  myAnalogPinValue[Five] = myEvent.get(myP1ResEvent);
  myAnalogPinValue[Nine] = myEvent.get(myP2ResEvent);
}

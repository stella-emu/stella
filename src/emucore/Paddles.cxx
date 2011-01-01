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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#define TRIGMAX 240
#define TRIGMIN 1

#include "Event.hxx"
#include "Paddles.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Paddles::Paddles(Jack jack, const Event& event, const System& system, bool swap)
  : Controller(jack, event, system, Controller::Paddles)
{
  // Swap the paddle events, from paddle 0 <=> 1 and paddle 2 <=> 3
  // Also consider whether this is the left or right port
  if(myJack == Left)
  {
    if(!swap)
    {
      myP0AxisValue  = Event::SALeftAxis0Value;
      myP0DecEvent1  = Event::PaddleZeroDecrease;
      myP0DecEvent2  = Event::JoystickZeroRight;
      myP0IncEvent1  = Event::PaddleZeroIncrease;
      myP0IncEvent2  = Event::JoystickZeroLeft;
      myP0FireEvent1 = Event::PaddleZeroFire;
      myP0FireEvent2 = Event::JoystickZeroFire1;

      myP1AxisValue  = Event::SALeftAxis1Value;
      myP1DecEvent1  = Event::PaddleOneDecrease;
      myP1DecEvent2  = Event::JoystickZeroUp;
      myP1IncEvent1  = Event::PaddleOneIncrease;
      myP1IncEvent2  = Event::JoystickZeroDown;
      myP1FireEvent1 = Event::PaddleOneFire;
      myP1FireEvent2 = Event::JoystickZeroFire3;

      if(_MOUSEX_PADDLE < 0)  _MOUSEX_PADDLE = 0;
    }
    else
    {
      myP0AxisValue  = Event::SALeftAxis1Value;
      myP0DecEvent1  = Event::PaddleOneDecrease;
      myP0DecEvent2  = Event::JoystickZeroUp;
      myP0IncEvent1  = Event::PaddleOneIncrease;
      myP0IncEvent2  = Event::JoystickZeroDown;
      myP0FireEvent1 = Event::PaddleOneFire;
      myP0FireEvent2 = Event::JoystickZeroFire3;

      myP1AxisValue  = Event::SALeftAxis0Value;
      myP1DecEvent1  = Event::PaddleZeroDecrease;
      myP1DecEvent2  = Event::JoystickZeroRight;
      myP1IncEvent1  = Event::PaddleZeroIncrease;
      myP1IncEvent2  = Event::JoystickZeroLeft;
      myP1FireEvent1 = Event::PaddleZeroFire;
      myP1FireEvent2 = Event::JoystickZeroFire1;

      if(_MOUSEX_PADDLE < 0)  _MOUSEX_PADDLE = 1;
    }
  }
  else
  {
    if(!swap)
    {
      myP0AxisValue  = Event::SARightAxis0Value;
      myP0DecEvent1  = Event::PaddleTwoDecrease;
      myP0DecEvent2  = Event::JoystickOneRight;
      myP0IncEvent1  = Event::PaddleTwoIncrease;
      myP0IncEvent2  = Event::JoystickOneLeft;
      myP0FireEvent1 = Event::PaddleTwoFire;
      myP0FireEvent2 = Event::JoystickOneFire1;

      myP1AxisValue  = Event::SARightAxis1Value;
      myP1DecEvent1  = Event::PaddleThreeDecrease;
      myP1DecEvent2  = Event::JoystickOneUp;
      myP1IncEvent1  = Event::PaddleThreeIncrease;
      myP1IncEvent2  = Event::JoystickOneDown;
      myP1FireEvent1 = Event::PaddleThreeFire;
      myP1FireEvent2 = Event::JoystickOneFire3;

      if(_MOUSEX_PADDLE < 0)  _MOUSEX_PADDLE = 0;
    }
    else
    {
      myP0AxisValue  = Event::SARightAxis1Value;
      myP0DecEvent1  = Event::PaddleThreeDecrease;
      myP0DecEvent2  = Event::JoystickOneUp;
      myP0IncEvent1  = Event::PaddleThreeIncrease;
      myP0IncEvent2  = Event::JoystickOneDown;
      myP0FireEvent1 = Event::PaddleThreeFire;
      myP0FireEvent2 = Event::JoystickOneFire3;

      myP1AxisValue  = Event::SARightAxis0Value;
      myP1DecEvent1  = Event::PaddleTwoDecrease;
      myP1DecEvent2  = Event::JoystickOneRight;
      myP1IncEvent1  = Event::PaddleTwoIncrease;
      myP1IncEvent2  = Event::JoystickOneLeft;
      myP1FireEvent1 = Event::PaddleTwoFire;
      myP1FireEvent2 = Event::JoystickOneFire1;

      if(_MOUSEX_PADDLE < 0)  _MOUSEX_PADDLE = 1;
    }
  }

  // Digital pins 1, 2 and 6 are not connected
  myDigitalPinState[One] =
  myDigitalPinState[Two] =
  myDigitalPinState[Six] = true;

  // Digital emulation of analog paddle movement
  myKeyRepeat0 = myPaddleRepeat0 = myKeyRepeat1 = myPaddleRepeat1 = 0;

  myCharge[0] = myCharge[1] =
  myLastCharge[0] = myLastCharge[1] = TRIGMAX/2;  // half of maximum paddle charge
  myLeftMotion[0] = myLeftMotion[1] = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Paddles::~Paddles()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::update()
{
  myDigitalPinState[Three] = myDigitalPinState[Four] = true;

  // Digital events (from keyboard or joystick hats & buttons)
  myDigitalPinState[Three] =
    (myEvent.get(myP1FireEvent1) == 0 && myEvent.get(myP1FireEvent2) == 0);
  myDigitalPinState[Four]  =
    (myEvent.get(myP0FireEvent1) == 0 && myEvent.get(myP0FireEvent2) == 0);

  if(myKeyRepeat0)
  {
    myPaddleRepeat0++;
    if(myPaddleRepeat0 > _PADDLE_SPEED)  myPaddleRepeat0 = 2;
  }
  if(myKeyRepeat1)
  {
    myPaddleRepeat1++;
    if(myPaddleRepeat1 > _PADDLE_SPEED)  myPaddleRepeat1 = 2;
  }

  myKeyRepeat0 = 0;
  myKeyRepeat1 = 0;

  if(myEvent.get(myP0DecEvent1) || myEvent.get(myP0DecEvent2))
  {
    myKeyRepeat0 = 1;
    if(myCharge[0] > myPaddleRepeat0)
      myCharge[0] -= myPaddleRepeat0;
  }
  if(myEvent.get(myP0IncEvent1) || myEvent.get(myP0IncEvent2))
  {
    myKeyRepeat0 = 1;
    if((myCharge[0] + myPaddleRepeat0) < TRIGMAX)
      myCharge[0] += myPaddleRepeat0;
  }
  if(myEvent.get(myP1DecEvent1) || myEvent.get(myP1DecEvent2))
  {
    myKeyRepeat1 = 1;
    if(myCharge[1] > myPaddleRepeat1)
      myCharge[1] -= myPaddleRepeat1;
  }
  if(myEvent.get(myP1IncEvent1) || myEvent.get(myP1IncEvent2))
  {
    myKeyRepeat1 = 1;
    if((myCharge[1] + myPaddleRepeat1) < TRIGMAX)
      myCharge[1] += myPaddleRepeat1;
  }

  // Mouse events
  if(myJack == Left && (_MOUSEX_PADDLE == 0 || _MOUSEX_PADDLE == 1))
  {
    // TODO - add infrastructure to map mouse direction to increase or decrease charge
    myCharge[_MOUSEX_PADDLE] -= myEvent.get(Event::MouseAxisXValue);
    if(myCharge[_MOUSEX_PADDLE] < TRIGMIN) myCharge[_MOUSEX_PADDLE] = TRIGMIN;
    if(myCharge[_MOUSEX_PADDLE] > TRIGMAX) myCharge[_MOUSEX_PADDLE] = TRIGMAX;
    if(myEvent.get(Event::MouseButtonValue))
      myDigitalPinState[ourButtonPin[_MOUSEX_PADDLE]] = false;
  }
  else if(myJack == Right && (_MOUSEX_PADDLE == 2 || _MOUSEX_PADDLE == 3))
  {
    // TODO - add infrastructure to map mouse direction to increase or decrease charge
    myCharge[_MOUSEX_PADDLE-2] -= myEvent.get(Event::MouseAxisXValue);
    if(myCharge[_MOUSEX_PADDLE-2] < TRIGMIN) myCharge[_MOUSEX_PADDLE-2] = TRIGMIN;
    if(myCharge[_MOUSEX_PADDLE-2] > TRIGMAX) myCharge[_MOUSEX_PADDLE-2] = TRIGMAX;
    if(myEvent.get(Event::MouseButtonValue))
      myDigitalPinState[ourButtonPin[_MOUSEX_PADDLE-2]] = false;
  }

  // Axis events (possibly use analog values)
  int xaxis = myEvent.get(myP0AxisValue);
  int yaxis = myEvent.get(myP1AxisValue);

  // Filter out jitter by not allowing rapid direction changes
  int charge0 = ((32767 - xaxis) >> 8) & 0xff;
  if(charge0 - myLastCharge[0] > 0)  // we are moving left
  {
    if(!myLeftMotion[0])  // moving right before?
    {
      if(charge0 - myLastCharge[0] <= 4)
      {
        myCharge[0] = myLastCharge[0];
      }
      else
      {
        myCharge[0] = (charge0 + myLastCharge[0]) >> 1;
        myLastCharge[0] = charge0;
        myLeftMotion[0] = 1;
      }
    }
    else
    {
      myCharge[0] = (charge0 + myLastCharge[0]) >> 1;
      myLastCharge[0] = charge0;
    }
  }
  // Filter out jitter by not allowing rapid direction changes
  else if(charge0 - myLastCharge[0] < 0)  // we are moving right
  {
    if(myLeftMotion[0])  // moving left before?
    {
      if(myLastCharge[0] - charge0 <= 4)
      {
        myCharge[0] = myLastCharge[0];
      }
      else
      {
        myCharge[0] = (charge0 + myLastCharge[0]) >> 1;
        myLastCharge[0] = charge0;
        myLeftMotion[0] = 0; 
      }
    }
    else
    {
      myCharge[0] = (charge0 + myLastCharge[0]) >> 1;
      myLastCharge[0] = charge0;
    }
  }

  // Filter out jitter by not allowing rapid direction changes
  int charge1 = ((32767 - yaxis) >> 8) & 0xff;
  if(charge1 - myLastCharge[1] > 0)  // we are moving left
  {
    if(!myLeftMotion[1])  // moving right before?
    {
      if(charge1 - myLastCharge[1] <= 4)
      {
        myCharge[1] = myLastCharge[1];
      }
      else
      {
        myCharge[1] = (charge1 + myLastCharge[1]) >> 1;
        myLastCharge[1] = charge1;
        myLeftMotion[1] = 1; 
      }
    }
    else
    {
      myCharge[1] = (charge1 + myLastCharge[1]) >> 1;
      myLastCharge[1] = charge1;
    }
  }
  // Filter out jitter by not allowing rapid direction changes
  else if(charge1 - myLastCharge[1] < 0)  // we are moving right
  {
    if(myLeftMotion[1])  // moving left before?
    {
      if(myLastCharge[1] - charge1 <= 4)
      {
        myCharge[1] = myLastCharge[1];
      }
      else
      {
        myCharge[1] = (charge1 + myLastCharge[1]) >> 1;
        myLastCharge[1] = charge1;
        myLeftMotion[1] = 0; 
      }
    }
    else
    {
      myCharge[1] = (charge1 + myLastCharge[1]) >> 1;
      myLastCharge[1] = charge1;
    }
  }
 
  myAnalogPinValue[Five] = (Int32)(1400000 * (myCharge[1] / 255.0));
  myAnalogPinValue[Nine] = (Int32)(1400000 * (myCharge[0] / 255.0));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::setMouseIsPaddle(int number, int dir)
{
  // TODO - make mouse Y axis be actually used in the code above
  if(dir == 0)
    _MOUSEX_PADDLE = number;
  else
    _MOUSEY_PADDLE = number;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Paddles::_PADDLE_SPEED = 6;
int Paddles::_MOUSEX_PADDLE = -1;
int Paddles::_MOUSEY_PADDLE = -1;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Controller::DigitalPin Paddles::ourButtonPin[2] = { Four, Three };

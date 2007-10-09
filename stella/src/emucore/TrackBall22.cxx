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
// $Id: TrackBall22.cxx,v 1.2 2007-10-09 23:56:57 stephena Exp $
//============================================================================

#include "Event.hxx"
#include "TrackBall22.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TrackBall22::TrackBall22(Jack jack, const Event& event)
  : Controller(jack, event, Controller::TrackBall22),
    myHCounter(0),
    myVCounter(0)
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

  // Analog pins are never used by the CX-22
  myAnalogPinValue[Five] = myAnalogPinValue[Nine] = maximumResistance;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TrackBall22::~TrackBall22()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TrackBall22::update()
{
//db TrakBallTableTB_V[2][2] = {{0x00, 0x10},{0x20, 0x30}};	/* CX-22 */
//db TrakBallTableTB_H[2][2] = {{0x40, 0x00},{0xc0, 0x80}};	/* CX-22 */


  // Gray codes for rotation
  static const uInt8 graytable[] = { 0x00, 0x10, 0x20, 0x30 };

  // Determine which gray code we're at
  if(myEvent.get(myLeftEvent) != 0)
{
    myHCounter--;
cerr << "left\n";
}
  else if(myEvent.get(myRightEvent) != 0)
{
    myHCounter++;
cerr << "right\n";
}
  // Only consider the lower-most bits (corresponding to pins 1 & 2)
  myHCounter &= 0x0f;
  uInt8 gray = graytable[myHCounter >> 2];

  // Determine which bits are set
  myDigitalPinState[One]   = (gray & 0x1) != 0;
  myDigitalPinState[Two]   = (gray & 0x2) != 0;
  myDigitalPinState[Three] = (gray & 0x1) != 0;
  myDigitalPinState[Four]  = (gray & 0x2) != 0;

  myDigitalPinState[Six] = (myEvent.get(myFireEvent) == 0);
}

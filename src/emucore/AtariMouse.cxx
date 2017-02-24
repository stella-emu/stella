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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Event.hxx"
#include "System.hxx"
#include "TIA.hxx"
#include "AtariMouse.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AtariMouse::AtariMouse(Jack jack, const Event& event, const System& system)
  : Controller(jack, event, system, Controller::AtariMouse),
    myHCounter(0),
    myVCounter(0),
    myMouseEnabled(false)
{
  // This code in ::read() is set up to always return IOPortA values in
  // the lower 4 bits data value
  // As such, the jack type (left or right) isn't necessary here

  myTrakBallCountH = myTrakBallCountV = 0;
  myTrakBallLinesH = myTrakBallLinesV = 1;

  myTrakBallLeft = myTrakBallDown = myScanCountV = myScanCountH =
    myCountV = myCountH = 0;

  // Analog pins are never used by the Atari ST mouse controller
  myAnalogPinValue[Five] = myAnalogPinValue[Nine] = maximumResistance;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 AtariMouse::read()
{
  int scanline = mySystem.tia().scanlines();

  if(myScanCountV > scanline) myScanCountV = 0;
  if(myScanCountH > scanline) myScanCountH = 0;
  while((myScanCountV + myTrakBallLinesV) < scanline)
  {
    if(myTrakBallCountV)
    {
      if(myTrakBallDown) myCountV--;
      else               myCountV++;
      myTrakBallCountV--;
    }
    myScanCountV += myTrakBallLinesV;
  }

  while((myScanCountH + myTrakBallLinesH) < scanline)
  {
    if(myTrakBallCountH)
    {
      if(myTrakBallLeft) myCountH--;
      else               myCountH++;
      myTrakBallCountH--;
    }
    myScanCountH += myTrakBallLinesH;
  }

  myCountV &= 0x03;
  myCountH &= 0x03;

  static constexpr uInt32 ourTableH[4] = { 0x00, 0x80, 0xc0, 0x40 };
  static constexpr uInt32 ourTableV[4] = { 0x00, 0x10, 0x30, 0x20 };
  uInt8 IOPortA = ourTableV[myCountV] | ourTableH[myCountH];

  myDigitalPinState[One]   = IOPortA & 0x10;
  myDigitalPinState[Two]   = IOPortA & 0x20;
  myDigitalPinState[Three] = IOPortA & 0x40;
  myDigitalPinState[Four]  = IOPortA & 0x80;

  return (IOPortA >> 4);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariMouse::update()
{
  if(!myMouseEnabled)
    return;

  // Get the current mouse position
  myHCounter = myEvent.get(Event::MouseAxisXValue);
  myVCounter = myEvent.get(Event::MouseAxisYValue);

  if(myVCounter < 0) myTrakBallLeft = 1;
  else               myTrakBallLeft = 0;
  if(myHCounter < 0) myTrakBallDown = 0;
  else               myTrakBallDown = 1;
  myTrakBallCountH = abs(myVCounter >> 1);
  myTrakBallCountV = abs(myHCounter >> 1);
  myTrakBallLinesH = mySystem.tia().height() / (myTrakBallCountH + 1);
  if(myTrakBallLinesH == 0) myTrakBallLinesH = 1;
  myTrakBallLinesV = mySystem.tia().height() / (myTrakBallCountV + 1);
  if(myTrakBallLinesV == 0) myTrakBallLinesV = 1;

  // Get mouse button state
  myDigitalPinState[Six] = (myEvent.get(Event::MouseButtonLeftValue) == 0) &&
                           (myEvent.get(Event::MouseButtonRightValue) == 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AtariMouse::setMouseControl(
    Controller::Type xtype, int xid, Controller::Type ytype, int yid)
{
  // Currently, the various trakball controllers take full control of the
  // mouse, and use both mouse buttons for the single fire button
  // As well, there's no separate setting for x and y axis, so any
  // combination of Controller and id is valid
  myMouseEnabled = (xtype == myType || ytype == myType) &&
                   (xid != -1 || yid != -1);
  return true;
}

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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Event.hxx"
#include "TIA.hxx"
#include "FrameBuffer.hxx"

#include "Lightgun.hxx"

// |              | Left port   | Right port  |
// | Fire button  | SWCHA bit 4 | SWCHA bit 0 | DP:1
// | Detect light | INPT4 bit 7 | INPT5 bit 7 | DP:6

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Lightgun::Lightgun(Jack jack, const Event& event, const System& system, const FrameBuffer& frameBuffer)
  : Controller(jack, event, system, Controller::Type::Lightgun),
  myFrameBuffer(frameBuffer)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Lightgun::read(DigitalPin pin)
{
  // We need to override the Controller::read() method, since the lightgun
  // checks this multiple times per frame
  // (we can't just read 60 times per second in the ::update() method)
  switch (pin)
  {
    case DigitalPin::Six: // INPT4/5
    {
      TIA& tia = mySystem.tia();
      const Common::Rect& rect = myFrameBuffer.imageRect();
      // scale mouse coordinates into TIA coordinates
      Int32 xMouse = (myEvent.get(Event::MouseAxisXValue) - rect.x())
        * tia.width() / rect.w();
      Int32 yMouse = (myEvent.get(Event::MouseAxisYValue) - rect.y())
        * tia.height() / rect.h();

      // get adjusted TIA coordinates
      Int32 xTia = tia.clocksThisLine() - TIAConstants::H_BLANK_CLOCKS + X_OFS;
      Int32 yTia = tia.scanlines() - tia.startLine() + Y_OFS;

      if (xTia < 0)
        xTia += TIAConstants::H_CLOCKS;

      bool enable = !((xTia - xMouse) >= 0 && (xTia - xMouse) < 15 && (yTia - yMouse) >= 0);

      return enable;
    }
    default:
      return Controller::read(pin);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Lightgun::update()
{
  // we allow left and right mouse buttons for fire button
  setPin(DigitalPin::One, myEvent.get(Event::MouseButtonLeftValue)
         || myEvent.get(Event::MouseButtonRightValue));
}


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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================


#include "TIA.hxx"
#include "System.hxx"
#include "FrameBuffer.hxx"

#include "Lightgun.hxx"

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
    // Pin 6: INPT4/5
    case DigitalPin::Six:
    {
      const Common::Rect& rect = myFrameBuffer.imageRect();
      // scale mouse coordinates into TIA coordinates
      Int32 xpos = (myMouseX - rect.x()) * TIAConstants::H_PIXEL / rect.w();
      Int32 ypos = (myMouseY - rect.y()) * 210 / rect.h(); // TODO: replace "magic number"
      // get adjusted TIA coordinates
      Int32 x = mySystem.tia().clocksThisLine() - TIAConstants::H_BLANK_CLOCKS + X_OFS;
      Int32 y = mySystem.tia().scanlines() - mySystem.tia().startLine() + Y_OFS;

      if (x < 0)
        x += TIAConstants::H_CLOCKS;

      bool enable = !((x - xpos) >= 0 && (x - xpos) < 15 && (y - ypos) >= 0);

      return enable;
    }

    default:
      return Controller::read(pin);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Lightgun::update()
{
  // |              | Left port   | Right port  |
  // | Fire button  | SWCHA bit 4 | SWCHA bit 0 | DP:1
  // | Detect light | INPT4 bit 7 | INPT5 bit 7 | DP:6

  Int32 xVal = myEvent.get(Event::MouseAxisXValue);
  Int32 yVal = myEvent.get(Event::MouseAxisYValue);

  myMouseX = xVal ? xVal : myMouseX;
  myMouseY = yVal ? yVal : myMouseY;

  setPin(DigitalPin::One, myEvent.get(Event::MouseButtonLeftValue) || myEvent.get(Event::MouseButtonRightValue));
}


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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <cmath>
#include <climits>

#include "Control.hxx"
#include "Event.hxx"
#include "System.hxx"

#include "PointingDevice.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PointingDevice::PointingDevice(Jack jack, const Event& event,
                               const System& system, Controller::Type type,
                               float sensitivity)
  : Controller(jack, event, system, type),
    mySensitivity{sensitivity}
{
  // The code in ::read() is set up to always return IOPortA values in
  // the lower 4 bits data value
  // As such, the jack type (left or right) isn't necessary here
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 PointingDevice::read()
{
  // Elapsed CPU cycles since the start of the current frame; this is the
  // controller's only notion of time, just as a real quadrature encoder
  // emits transitions purely as a function of elapsed time
  const int elapsed = static_cast<int>(mySystem.cycles() - myFrameStartCycle);

  // Loop over all missed changes
  while(myCycleCountH < elapsed)
  {
    if(myTrackBallLeft) --myCountH;
    else                ++myCountH;

    // Define cycle of next change
    myCycleCountH += myTrackBallCyclesH;
  }

  // Loop over all missed changes
  while(myCycleCountV < elapsed)
  {
    if(myTrackBallDown) ++myCountV;
    else                --myCountV;

    // Define cycle of next change
    myCycleCountV += myTrackBallCyclesV;
  }

  myCountH &= 0b11;
  myCountV &= 0b11;

  const uInt8 portA = ioPortA(myCountH, myCountV, myTrackBallLeft, myTrackBallDown);

  setPin(DigitalPin::One,   portA & 0b0001);
  setPin(DigitalPin::Two,   portA & 0b0010);
  setPin(DigitalPin::Three, portA & 0b0100);
  setPin(DigitalPin::Four,  portA & 0b1000);

  return portA;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PointingDevice::update()
{
  // Snapshot the frame boundary on the system (CPU) clock.  Done before the
  // mouse-enabled check so elapsed time in read() is always measured against
  // the current frame, never a stale reference
  const uInt64 cycles = mySystem.cycles();
  const uInt64 cyclesLastFrame = cycles - myFrameStartCycle;
  myFrameStartCycle = cycles;

  if(!myMouseEnabled)
    return;

  // Update horizontal direction
  updateDirection( myEvent.get(Event::MouseAxisXMove), cyclesLastFrame,
      myHCounterRemainder, myTrackBallLeft, myTrackBallCyclesH,
      myCycleCountH, myFirstOffsetH);

  // Update vertical direction
  updateDirection(-myEvent.get(Event::MouseAxisYMove), cyclesLastFrame,
      myVCounterRemainder, myTrackBallDown, myTrackBallCyclesV,
      myCycleCountV, myFirstOffsetV);

  // We allow left and right mouse buttons for fire button
  setPin(DigitalPin::Six, !getAutoFireState(myEvent.get(Event::LeftJoystickFire) ||
    myEvent.get(Event::MouseButtonLeftValue) || myEvent.get(Event::MouseButtonRightValue)));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PointingDevice::setMouseControl(
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PointingDevice::setSensitivity(int sensitivity)
{
  BSPF::clamp(sensitivity, MIN_SENSE, MAX_SENSE, (MIN_SENSE + MAX_SENSE) / 2);
  TB_SENSITIVITY = sensitivity / 10.F;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PointingDevice::updateDirection(int counter, uInt64 cyclesLastFrame,
    float& counterRemainder, bool& trackBallDir, int& trackBallCycles,
    int& cycleCount, int& firstOffset)
{
  // Apply sensitivity and calculate remainder
  const float fTrackBallCount = counter * mySensitivity * TB_SENSITIVITY + counterRemainder;
  int trackBallCount = static_cast<int>(std::lround(fTrackBallCount));
  counterRemainder = fTrackBallCount - trackBallCount;

  if(trackBallCount)
  {
    trackBallDir = (trackBallCount > 0);
    trackBallCount = abs(trackBallCount);

    // Spread this frame's movement evenly across the (estimated) length of a
    // frame, measured in CPU cycles instead of scanlines
    trackBallCycles = static_cast<int>(cyclesLastFrame) / trackBallCount;

    // Set lower limit in case of (unrealistic) ultra fast mouse movements
    if(trackBallCycles == 0)
      trackBallCycles = 1;

    // Define cycle offset of first change
    cycleCount = (trackBallCycles * firstOffset) >> 12;
  }
  else
  {
    // Prevent any change
    cycleCount = INT_MAX;

    // Define offset factor for first change, move randomly forward by up to 1/8th
    firstOffset = (((firstOffset << 3) + mySystem.randGenerator().next() %
                  (1 << 12)) >> 3) & ((1 << 12) - 1);
  }
}

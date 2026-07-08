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

#include "Event.hxx"
#include "Paddles.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Paddles::Paddles(Jack jack, const Event& event, const System& system,
                 bool swappaddle, bool swapaxis, bool swapdir, bool altmap)
  : Controller(jack, event, system, Controller::Type::Paddles)
{
  // We must start with a physical valid resistance (e.g. 0);
  // see commit 38b452e1a047a0dca38c5bcce7c271d40f76736e for more information
  setPin(AnalogPin::Nine, AnalogReadout::connectToVcc());
  setPin(AnalogPin::Five, AnalogReadout::connectToVcc());

  // The following logic reflects that mapping paddles to different
  // devices can be extremely complex
  // As well, while many paddle games have horizontal movement of
  // objects (which maps nicely to horizontal movement of the joystick
  // or mouse), others have vertical movement
  // This vertical handling is taken care of by swapping the axes
  // On the other hand, some games treat paddle resistance differently,
  // (ie, increasing resistance can move an object right instead of left)
  // This is taken care of by swapping the direction of movement
  // Arrgh, did I mention that paddles are complex ...

  // As much as possible, precompute which events we care about for
  // a given port; this will speed up processing in update()

  // Clear some potentially unused events:
  myAAxisValue = myBAxisValue =
    myADecEvent = myAIncEvent =
    myAButton1Event = myAButton2Event =
    myBDecEvent = myBIncEvent = Event::NoType;

  // Consider whether this is the left or right port
  if(myJack == Jack::Left)
  {
    if(!altmap)
    {
      // First paddle is left A, second is left B
      myAAxisValue    = Event::LeftPaddleAAnalog;
      myBAxisValue    = Event::LeftPaddleBAnalog;
      myAFireEvent    = Event::LeftPaddleAFire;
      myAButton1Event = Event::LeftPaddleAButton1;
      myAButton2Event = Event::LeftPaddleAButton2;

      myBFireEvent    = Event::LeftPaddleBFire;

      // These can be affected by changes in axis orientation
      myAIncEvent     = Event::LeftPaddleAIncrease;
      myBDecEvent     = Event::LeftPaddleBDecrease;
      myBIncEvent     = Event::LeftPaddleBIncrease;
      myADecEvent     = Event::LeftPaddleADecrease;
    }
    else
    {
      // First paddle is QT 3A, second is QT 3B (fire buttons only)
      myAFireEvent = Event::QTPaddle3AFire;
      myBFireEvent = Event::QTPaddle3BFire;
    }
  }
  else    // Jack is right port
  {
    if(!altmap)
    {
      // First paddle is right A, second is right B
      myAAxisValue    = Event::RightPaddleAAnalog;
      myBAxisValue    = Event::RightPaddleBAnalog;
      myAFireEvent    = Event::RightPaddleAFire;
      myAButton1Event = Event::RightPaddleAButton1;
      myAButton2Event = Event::RightPaddleAButton2;

      myBFireEvent    = Event::RightPaddleBFire;

      // These can be affected by changes in axis orientation
      myADecEvent     = Event::RightPaddleADecrease;
      myAIncEvent     = Event::RightPaddleAIncrease;
      myBDecEvent     = Event::RightPaddleBDecrease;
      myBIncEvent     = Event::RightPaddleBIncrease;
    }
    else
    {
      // First paddle is QT 4A, second is QT 4B (fire buttons only)
      myAFireEvent = Event::QTPaddle4AFire;
      myBFireEvent = Event::QTPaddle4BFire;
    }
  }

  // Some games swap the paddles
  if(swappaddle)
  {
    // First paddle is right A|B, second is left A|B
    std::swap(myAAxisValue, myBAxisValue);
    std::swap(myAFireEvent, myBFireEvent);
    myAButton1Event = myAButton2Event = Event::NoType;
    std::swap(myADecEvent, myBDecEvent);
    std::swap(myAIncEvent, myBIncEvent);
  }

  // Direction of movement can be swapped
  // That is, moving in a certain direction on an axis can
  // result in either increasing or decreasing paddle movement
  if(swapdir)
  {
    std::swap(myADecEvent, myAIncEvent);
    std::swap(myBDecEvent, myBIncEvent);
  }

  // The following are independent of whether or not the port
  // is left or right
  MOUSE_SENSITIVITY = swapdir ? -std::abs(MOUSE_SENSITIVITY) :
                                 std::abs(MOUSE_SENSITIVITY);
  // MOUSE_SENSITIVITY is an integer (1..20) calibrated against the historical
  // 4096-step accumulator; divide once here to get the normalized per-pixel step.
  myMouseScale = MOUSE_SENSITIVITY / 4096.F;
  if(!swapaxis)
  {
    myAxisMouseMotion = Event::MouseAxisXMove;
    myAxisDigitalZero = 0;
    myAxisDigitalOne  = 1;
  }
  else
  {
    myAxisMouseMotion = Event::MouseAxisYMove;
    myAxisDigitalZero = 1;
    myAxisDigitalOne  = 0;
  }

  // Digital pins 1, 2 and 6 are (usually, see below) not connected
  //setPin(DigitalPin::One, true);
  //setPin(DigitalPin::Two, true);
  setPin(DigitalPin::Six, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::update()
{
  updateA();
  updateB();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::updateA()
{
  setPin(DigitalPin::Four, true);

  // Digital events (from keyboard or joystick hats & buttons).  Collect the
  // events driving the fire button (the fire event plus any mouse buttons
  // mapped to this paddle) so it can be bound for replay within the input window.
  std::array<Event::Type, MAX_PIN_EVENTS> fire{myAFireEvent};
  size_t n = 1;

  // Paddle movement is a very difficult thing to accurately emulate,
  // since it originally came from an analog device that had very
  // peculiar behaviour
  // Compounding the problem is the fact that we'd like to emulate
  // movement with 'digital' data (like from a keyboard or a digital
  // joystick axis), but also from a mouse (relative values)
  // and Stelladaptor-like devices (absolute analog values clamped to
  // a certain range)
  // And to top it all off, we don't want one devices input to conflict
  // with the others ...

  if(!updateAnalogAxesA())
  {
    updateMouseA(fire, n);
    updateDigitalAxesA();

    setPin(AnalogPin::Five, AnalogReadout::connectToVcc(
        AnalogReadout::MAX_POT_RESISTANCE * myPosition[0]));
  }

  // Bind the fire button for replay within the input window (static only under autofire)
  updateFireButton(DigitalPin::Four, myFireDelay, {fire.data(), n});

  // Joystick up/down pins when using a splitter:
  bindPin(DigitalPin::One, myAButton1Event);
  bindPin(DigitalPin::Two, myAButton2Event);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::conditionAxisInput(int lastAxis, int& newAxis)
{
  const float range = ANALOG_RANGE - analogDeadZone() * 2;

  // dead zone: strip center noise, shift remaining range to start at zero
  if(newAxis > analogDeadZone())
    newAxis -= analogDeadZone();
  else if(newAxis < -analogDeadZone())
    newAxis += analogDeadZone();
  else
    newAxis = 0;

  static constexpr std::array<float, MAX_DEJITTER - MIN_DEJITTER + 1> bFac = {
    // higher values mean more dejitter strength
    0.F, // off
    0.5F,  0.59F, 0.67F, 0.74F,  0.8F,
    0.85F, 0.89F, 0.92F, 0.94F, 0.95F
  };
  static constexpr std::array<float, MAX_DEJITTER - MIN_DEJITTER + 1> dFac = {
    // lower values mean more dejitter strength
    1.F, // off
    1.F / 181,  1.F /  256, 1.F / 362,  1.F / 512,  1.F / 724,
    1.F / 1024, 1.F / 1448, 1.F / 2048, 1.F / 2896, 1.F / 4096
  };
  const float baseFactor = bFac[DEJITTER_BASE];
  const float diffFactor = dFac[DEJITTER_DIFF];

  // dejitter: suppress small noisy changes, blend toward previous value
  const float dejitter = std::pow(baseFactor, std::abs(newAxis - lastAxis) * diffFactor);
  const int newVal = newAxis * (1 - dejitter) + lastAxis * dejitter;

  // only use new dejittered value for larger differences
  if(std::abs(newVal - newAxis) > 10)
    newAxis = newVal;

  // linearity: warp the response curve
  float linearVal = newAxis / (range / 2); // scale to -1.0..+1.0
  linearVal = std::copysign(std::pow(std::abs(linearVal), LINEARITY), linearVal);
  newAxis = linearVal * (range / 2);

  // rescale to full ANALOG_RANGE to compensate for the dead zone range reduction
  newAxis = newAxis * ANALOG_RANGE / range;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Paddles::updateAnalogAxesA()
{
  // Analog axis events from Stelladaptor-like devices,
  // (which includes analog USB controllers)
  // These devices generate data in the range -32768 to 32767,
  // so we have to scale appropriately
  // Since these events are generated and stored indefinitely,
  // we only process the first one we see (when it differs from
  // previous values by a pre-defined amount)
  // Otherwise, it would always override input from digital and mouse

  int sa_xaxis = myEvent.get(myAAxisValue);
  bool sa_changed = false;

  if(std::abs(myLastAxisX - sa_xaxis) > 10)
  {
    conditionAxisInput(myLastAxisX, sa_xaxis);
    setPin(AnalogPin::Five, AnalogReadout::connectToVcc(
        AnalogReadout::MAX_POT_RESISTANCE * BSPF::clamp(
            (ANALOG_MAX_VALUE - (sa_xaxis * SENSITIVITY + XCENTER)) / float{ANALOG_RANGE},
            0.F, 1.F)));
    sa_changed = true;
  }

  myLastAxisX = sa_xaxis;
  return sa_changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::updateMouseA(std::array<Event::Type, MAX_PIN_EVENTS>& fire, size_t& n)
{
  // Mouse motion events give relative movement
  // That is, they're only relevant if they're non-zero
  if(myMPaddleID == 0)
  {
    // We're in auto mode, where a single axis is used for one paddle only
    myPosition[myMPaddleID] = BSPF::clamp(
        myPosition[myMPaddleID] - myEvent.get(myAxisMouseMotion) * myMouseScale,
        0.F, POSITION_LIMIT);
    fire[n++] = Event::MouseButtonLeftValue;
    fire[n++] = Event::MouseButtonRightValue;
  }
  else
  {
    // Test for 'untied' mouse axis mode, where each axis is potentially
    // mapped to a separate paddle
    if(myMPaddleIDX == 0)
    {
      myPosition[myMPaddleIDX] = BSPF::clamp(
          myPosition[myMPaddleIDX] - myEvent.get(Event::MouseAxisXMove) * myMouseScale,
          0.F, POSITION_LIMIT);
      fire[n++] = Event::MouseButtonLeftValue;
    }
    if(myMPaddleIDY == 0)
    {
      myPosition[myMPaddleIDY] = BSPF::clamp(
          myPosition[myMPaddleIDY] - myEvent.get(Event::MouseAxisYMove) * myMouseScale,
          0.F, POSITION_LIMIT);
      fire[n++] = Event::MouseButtonRightValue;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::updateDigitalAxesA()
{
  // Finally, consider digital input, where movement happens
  // until a digital event is released
  if(myKeyRepeatA)
    myPaddleRepeatA = std::min(myPaddleRepeatA + DIGITAL_STEP / DIGITAL_SENSITIVITY,
                               DIGITAL_STEP);

  myKeyRepeatA = false;

  if(myEvent.get(myADecEvent))
  {
    myKeyRepeatA = true;
    myPosition[myAxisDigitalZero] = std::max(0.F,
        myPosition[myAxisDigitalZero] - myPaddleRepeatA);
  }
  if(myEvent.get(myAIncEvent))
  {
    myKeyRepeatA = true;
    myPosition[myAxisDigitalZero] = std::min(POSITION_LIMIT,
        myPosition[myAxisDigitalZero] + myPaddleRepeatA);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::updateB()
{
  setPin(DigitalPin::Three, true);

  // Digital events (from keyboard or joystick hats & buttons).  Collect the
  // events driving the fire button so it can be bound for replay within the
  // input window.
  std::array<Event::Type, MAX_PIN_EVENTS> fire{myBFireEvent};
  size_t n = 1;

  if(!updateAnalogAxesB())
  {
    updateMouseB(fire, n);
    updateDigitalAxesB();

    setPin(AnalogPin::Nine, AnalogReadout::connectToVcc(
        AnalogReadout::MAX_POT_RESISTANCE * myPosition[1]));
  }

  // Bind the fire button for replay within the input window (static only under autofire)
  updateFireButton(DigitalPin::Three, myFireDelayP1, {fire.data(), n});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Paddles::updateAnalogAxesB()
{
  int sa_yaxis = myEvent.get(myBAxisValue);
  bool sa_changed = false;

  if(std::abs(myLastAxisY - sa_yaxis) > 10)
  {
    conditionAxisInput(myLastAxisY, sa_yaxis);
    setPin(AnalogPin::Nine, AnalogReadout::connectToVcc(
        AnalogReadout::MAX_POT_RESISTANCE * BSPF::clamp(
            (ANALOG_MAX_VALUE - (sa_yaxis * SENSITIVITY + YCENTER)) / float{ANALOG_RANGE},
            0.F, 1.F)));
    sa_changed = true;
  }

  myLastAxisY = sa_yaxis;
  return sa_changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::updateMouseB(std::array<Event::Type, MAX_PIN_EVENTS>& fire, size_t& n)
{
  // Mouse motion events give relative movement
  // That is, they're only relevant if they're non-zero
  if(myMPaddleID == 1)
  {
    // We're in auto mode, where a single axis is used for one paddle only
    myPosition[myMPaddleID] = BSPF::clamp(
        myPosition[myMPaddleID] - myEvent.get(myAxisMouseMotion) * myMouseScale,
        0.F, POSITION_LIMIT);
    fire[n++] = Event::MouseButtonLeftValue;
    fire[n++] = Event::MouseButtonRightValue;
  }
  else
  {
    // Test for 'untied' mouse axis mode, where each axis is potentially
    // mapped to a separate paddle
    if(myMPaddleIDX == 1)
    {
      myPosition[myMPaddleIDX] = BSPF::clamp(
          myPosition[myMPaddleIDX] - myEvent.get(Event::MouseAxisXMove) * myMouseScale,
          0.F, POSITION_LIMIT);
      fire[n++] = Event::MouseButtonLeftValue;
    }
    if(myMPaddleIDY == 1)
    {
      myPosition[myMPaddleIDY] = BSPF::clamp(
          myPosition[myMPaddleIDY] - myEvent.get(Event::MouseAxisYMove) * myMouseScale,
          0.F, POSITION_LIMIT);
      fire[n++] = Event::MouseButtonRightValue;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::updateDigitalAxesB()
{
  // Finally, consider digital input, where movement happens
  // until a digital event is released
  if(myKeyRepeatB)
    myPaddleRepeatB = std::min(myPaddleRepeatB + DIGITAL_STEP / DIGITAL_SENSITIVITY,
                               DIGITAL_STEP);

  myKeyRepeatB = false;

  if(myEvent.get(myBDecEvent))
  {
    myKeyRepeatB = true;
    myPosition[myAxisDigitalOne] = std::max(0.F,
        myPosition[myAxisDigitalOne] - myPaddleRepeatB);
  }
  if(myEvent.get(myBIncEvent))
  {
    myKeyRepeatB = true;
    myPosition[myAxisDigitalOne] = std::min(POSITION_LIMIT,
        myPosition[myAxisDigitalOne] + myPaddleRepeatB);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Paddles::setMouseControl(
    Controller::Type xtype, int xid, Controller::Type ytype, int yid)
{
  // In 'automatic' mode, both axes on the mouse map to a single paddle,
  // and the paddle axis and direction settings are taken into account
  // This overrides any other mode
  if(xtype == Controller::Type::Paddles && ytype == Controller::Type::Paddles && xid == yid)
  {
    myMPaddleID = ((myJack == Jack::Left && (xid == 0 || xid == 1)) ||
                   (myJack == Jack::Right && (xid == 2 || xid == 3))
                  ) ? xid & 0x01 : -1;
    myMPaddleIDX = myMPaddleIDY = -1;
  }
  else
  {
    // The following is somewhat complex, but we need to pre-process as much
    // as possible, so that ::update() can run quickly
    myMPaddleID = -1;
    if(myJack == Jack::Left)
    {
      if(xtype == Controller::Type::Paddles)
        myMPaddleIDX = (xid == 0 || xid == 1) ? xid & 0x01 : -1;
      if(ytype == Controller::Type::Paddles)
        myMPaddleIDY = (yid == 0 || yid == 1) ? yid & 0x01 : -1;
    }
    else if(myJack == Jack::Right)
    {
      if(xtype == Controller::Type::Paddles)
        myMPaddleIDX = (xid == 2 || xid == 3) ? xid & 0x01 : -1;
      if(ytype == Controller::Type::Paddles)
        myMPaddleIDY = (yid == 2 || yid == 3) ? yid & 0x01 : -1;
    }
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::setAnalogXCenter(int xcenter)
{
  // convert into ~5 pixel steps
  XCENTER = BSPF::clamp(xcenter, MIN_ANALOG_CENTER, MAX_ANALOG_CENTER) * 860;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::setAnalogYCenter(int ycenter)
{
  // convert into ~5 pixel steps
  YCENTER = BSPF::clamp(ycenter, MIN_ANALOG_CENTER, MAX_ANALOG_CENTER) * 860;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float Paddles::setAnalogSensitivity(int sensitivity)
{
  return SENSITIVITY = analogSensitivityValue(sensitivity);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float Paddles::analogSensitivityValue(int sensitivity)
{
  // BASE_ANALOG_SENSE * (1.1 ^ 20) = 1.0
  return BASE_ANALOG_SENSE * std::pow(1.1F,
    static_cast<float>(BSPF::clamp(sensitivity, MIN_ANALOG_SENSE, MAX_ANALOG_SENSE)));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::setAnalogLinearity(int linearity)
{
  LINEARITY = 100.F / BSPF::clamp(linearity, MIN_ANALOG_LINEARITY, MAX_ANALOG_LINEARITY);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::setDejitterBase(int strength)
{
  DEJITTER_BASE = BSPF::clamp(strength, MIN_DEJITTER, MAX_DEJITTER);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::setDejitterDiff(int strength)
{
  DEJITTER_DIFF = BSPF::clamp(strength, MIN_DEJITTER, MAX_DEJITTER);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::setDigitalSensitivity(int sensitivity)
{
  DIGITAL_SENSITIVITY = BSPF::clamp(sensitivity, MIN_DIGITAL_SENSE, MAX_DIGITAL_SENSE);
  // Express the full-speed step directly in normalized [0,1] position units.
  // The formula (20 + sensitivity*8) / 4096 preserves the historical calibration.
  DIGITAL_STEP = (20.F + (DIGITAL_SENSITIVITY * 8)) / 4096.F;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Paddles::setDigitalPaddleRange(int range)
{
  POSITION_LIMIT = BSPF::clamp(range, MIN_MOUSE_RANGE, MAX_MOUSE_RANGE) / 100.F;
}

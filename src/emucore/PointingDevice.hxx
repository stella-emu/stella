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

#ifndef POINTING_DEVICE_HXX
#define POINTING_DEVICE_HXX

#include "bspf.hxx"
#include "Control.hxx"
#include "Event.hxx"

/**
  Common controller class for pointing devices (Atari Mouse, Amiga Mouse, TrakBall)
  This code was heavily borrowed from z26.

  @author  Stephen Anthony & z26 team
           Template-ification by Christian Speckner, based on ideas by
           Thomas Jentzsch
*/
template<class T>
class PointingDevice : public Controller
{
  public:
    PointingDevice(Jack jack, const Event& event, const System& system);
    virtual ~PointingDevice() = default;

  public:
    using Controller::read;

    /**
      Read the entire state of all digital pins for this controller.
      Note that this method must use the lower 4 bits, and zero the upper bits.

      @return The state of all digital pins
    */
    uInt8 read() override;

    /**
      Update the entire digital and analog pin state according to the
      events currently set.
    */
    void update() override;

    /**
      Determines how this controller will treat values received from the
      X/Y axis and left/right buttons of the mouse.  Since not all controllers
      use the mouse the same way (or at all), it's up to the specific class to
      decide how to use this data.

      In the current implementation, the left button is tied to the X axis,
      and the right one tied to the Y axis.

      @param xtype  The controller to use for x-axis data
      @param xid    The controller ID to use for x-axis data (-1 for no id)
      @param ytype  The controller to use for y-axis data
      @param yid    The controller ID to use for y-axis data (-1 for no id)

      @return  Whether the controller supports using the mouse
    */
    bool setMouseControl(Controller::Type xtype, int xid,
                         Controller::Type ytype, int yid) override;

  private:
    float myHCounterRemainder, myVCounterRemainder;

    // How many lines to wait between sending new horz and vert values
    int myTrakBallLinesH, myTrakBallLinesV;

    // Was TrakBall moved left or moved right instead
    uInt8 myTrakBallLeft;

    // Was TrakBall moved down or moved up instead
    uInt8 myTrakBallDown;

    // Counter to iterate through the gray codes
    uInt8 myCountH, myCountV;
    int myScanCountH, myScanCountV;

    // Whether to use the mouse to emulate this controller
    bool myMouseEnabled;

  private:
    // Following constructors and assignment operators not supported
    PointingDevice() = delete;
    PointingDevice(const PointingDevice<T>&) = delete;
    PointingDevice(PointingDevice<T>&&) = delete;
    PointingDevice& operator=(const PointingDevice<T>&) = delete;
    PointingDevice& operator=(PointingDevice<T>&&) = delete;
};

// ############################################################################
// Implementation
// ############################################################################


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class T>
PointingDevice<T>::PointingDevice(Jack jack, const Event& event, const System& system)
  : Controller(jack, event, system, T::controllerType),
    myHCounterRemainder(0.0),
    myVCounterRemainder(0.0),
    myTrakBallLinesH(1), myTrakBallLinesV(1),
    myTrakBallLeft(0), myTrakBallDown(0),
    myCountH(0), myCountV(0),
    myScanCountH(0), myScanCountV(0),
    myMouseEnabled(false)
{
  // The code in ::read() is set up to always return IOPortA values in
  // the lower 4 bits data value
  // As such, the jack type (left or right) isn't necessary here
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class T>
uInt8 PointingDevice<T>::read()
{
  int scanline = mySystem.tia().scanlines();

  // Loop over all missed changes
  while(myScanCountH < scanline)
  {
    if(myTrakBallLeft) myCountH--;
    else               myCountH++;

    // Define scanline of next change
    myScanCountH += myTrakBallLinesH;
  }

  // Loop over all missed changes
  while(myScanCountV < scanline)
  {
    if(myTrakBallDown) myCountV--;
    else               myCountV++;

    // Define scanline of next change
    myScanCountV += myTrakBallLinesV;
  }

  myCountH &= 0x03;
  myCountV &= 0x03;

  uInt8 ioPortA = T::ioPortA(myCountV, myCountH, myTrakBallDown, myTrakBallLeft);

  myDigitalPinState[One]   = ioPortA & 0x10;
  myDigitalPinState[Two]   = ioPortA & 0x20;
  myDigitalPinState[Three] = ioPortA & 0x40;
  myDigitalPinState[Four]  = ioPortA & 0x80;

  return (ioPortA >> 4);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class T>
void PointingDevice<T>::update()
{
  if(!myMouseEnabled)
    return;

  // Get the current mouse position
  int hCounter = myEvent.get(Event::MouseAxisXValue);
  int vCounter = myEvent.get(Event::MouseAxisYValue);

  // Apply sensitivity and calculate remainders
  float fTrakBallCountH = hCounter * T::trackballSensitivity + myHCounterRemainder;
  int trakBallCountH = std::lround(fTrakBallCountH);
  myHCounterRemainder = fTrakBallCountH - trakBallCountH;

  float fTrakBallCountV = vCounter * T::trackballSensitivity + myVCounterRemainder;
  int trakBallCountV = std::lround(fTrakBallCountV);
  myVCounterRemainder = fTrakBallCountV - trakBallCountV;

  if(trakBallCountH)
  {
    myTrakBallLeft = (trakBallCountH < 0) ? 0 : 1;
    trakBallCountH = abs(trakBallCountH);

    // Calculate lines to wait between sending new horz values
    myTrakBallLinesH = mySystem.tia().scanlinesLastFrame() / trakBallCountH;

    // Set lower limit in case of (unrealistic) ultra fast mouse movements
    if (myTrakBallLinesH == 0) myTrakBallLinesH = 1;

    // Define random scanline of first change
    myScanCountH = rand() % myTrakBallLinesH;
  }
  else
  {
    // Prevent any change
    myScanCountH = INT_MAX;
  }

  if(trakBallCountV)
  {
    myTrakBallDown = (trakBallCountV < 0) ? 1 : 0;
    trakBallCountV = abs(trakBallCountV);

    // Calculate lines to wait between sending new vert values
    myTrakBallLinesV = mySystem.tia().scanlinesLastFrame() / trakBallCountV;

    // Set lower limit in case of (unrealistic) ultra fast mouse movements
    if (myTrakBallLinesV == 0) myTrakBallLinesV = 1;

    // Define random scanline of first change
    myScanCountV = rand() % myTrakBallLinesV;
  }
  else
  {
    // Prevent any change
    myScanCountV = INT_MAX;
  }

  // Get mouse button state
  myDigitalPinState[Six] = (myEvent.get(Event::MouseButtonLeftValue) == 0) &&
                           (myEvent.get(Event::MouseButtonRightValue) == 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class T>
bool PointingDevice<T>::setMouseControl(
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

#endif // POINTING_DEVICE_HXX

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
  Common controller class for pointing devices (Atari Mouse, Amiga Mouse, Trak-Ball)
  This code was heavily borrowed from z26.

  @author  Stephen Anthony, Thomas Jentzsch & z26 team
           Template-ification by Christian Speckner
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
    void updateDirection(const int& counter, double& counterRemainder,
                         bool& trackBallDir, int& trackBallLines,
                         int& scanCount, int& firstScanOffset);

  private:
    double myHCounterRemainder, myVCounterRemainder;

    // How many lines to wait between sending new horz and vert values
    int myTrackBallLinesH, myTrackBallLinesV;

    // Was TrackBall moved left or moved right instead
    bool myTrackBallLeft;

    // Was TrackBall moved down or moved up instead
    bool myTrackBallDown;

    // Counter to iterate through the gray codes
    uInt8 myCountH, myCountV;

    // Next scanline for change
    int myScanCountH, myScanCountV;

    // Offset factor for first scanline, 0..(1 << 12 - 1)
    int myFirstScanOffsetH, myFirstScanOffsetV;

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
    myHCounterRemainder(0.0), myVCounterRemainder(0.0),
    myTrackBallLinesH(1), myTrackBallLinesV(1),
    myTrackBallLeft(false), myTrackBallDown(false),
    myCountH(0), myCountV(0),
    myScanCountH(0), myScanCountV(0),
    myFirstScanOffsetH(0), myFirstScanOffsetV(0),
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
    if(myTrackBallLeft) myCountH--;
    else                myCountH++;

    // Define scanline of next change 
    myScanCountH += myTrackBallLinesH;
  }

  // Loop over all missed changes
  while(myScanCountV < scanline)
  {
    if(myTrackBallDown) myCountV--;
    else                myCountV++;

    // Define scanline of next change 
    myScanCountV += myTrackBallLinesV;
  }

  myCountH &= 0x03;
  myCountV &= 0x03;

  uInt8 ioPortA = T::ioPortA(myCountV, myCountH, myTrackBallDown, myTrackBallLeft);

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

  // Update horizontal direction
  updateDirection( myEvent.get(Event::MouseAxisXValue), myHCounterRemainder,
      myTrackBallLeft, myTrackBallLinesH, myScanCountH, myFirstScanOffsetH);

  // Update vertical direction
  updateDirection(-myEvent.get(Event::MouseAxisYValue), myVCounterRemainder,
      myTrackBallDown, myTrackBallLinesV, myScanCountV, myFirstScanOffsetV);

  // Get mouse button state
  myDigitalPinState[Six] = (myEvent.get(Event::MouseButtonLeftValue)  == 0) &&
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class T>
void PointingDevice<T>::updateDirection(const int& counter, double& counterRemainder,
    bool& trackBallDir, int& trackBallLines, int& scanCount, int& firstScanOffset)
{
  // Apply sensitivity and calculate remainder
  float fTrackBallCount = counter * T::trackballSensitivity + counterRemainder;
  int trackBallCount = std::lround(fTrackBallCount);
  counterRemainder = fTrackBallCount - trackBallCount;

  if(trackBallCount)
  {
    trackBallDir = (trackBallCount > 0);
    trackBallCount = abs(trackBallCount);

    // Calculate lines to wait between sending new horz/vert values
    trackBallLines = mySystem.tia().scanlinesLastFrame() / trackBallCount;

    // Set lower limit in case of (unrealistic) ultra fast mouse movements
    if (trackBallLines == 0) trackBallLines = 1;

    // Define scanline of first change
    scanCount = (trackBallLines * firstScanOffset) >> 12;
  }
  else
  {
    // Prevent any change
    scanCount = INT_MAX;

    // Define offset factor for first change, move randomly forward by up to 1/8th
    firstScanOffset = (((firstScanOffset << 3) + rand() %
                      (1 << 12)) >> 3) & ((1 << 12) - 1);
  }
}

#endif // POINTING_DEVICE_HXX

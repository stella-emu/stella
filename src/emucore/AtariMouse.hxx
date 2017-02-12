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

#ifndef ATARIMOUSE_HXX
#define ATARIMOUSE_HXX

#include "bspf.hxx"
#include "Control.hxx"
#include "Event.hxx"

/**
  Trakball-like controller emulating the Atari ST mouse.
  This code was heavily borrowed from z26.

  @author  Stephen Anthony & z26 team
*/
class AtariMouse : public Controller
{
  public:
    /**
      Create a new AtariMouse controller plugged into the specified jack

      @param jack   The jack the controller is plugged into
      @param event  The event object to use for events
      @param system The system using this controller
    */
    AtariMouse(Jack jack, const Event& event, const System& system);
    virtual ~AtariMouse() = default;

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
    // Counter to iterate through the gray codes
    int myHCounter, myVCounter;

    // How many new horizontal and vertical values this frame
    int myTrakBallCountH, myTrakBallCountV;

    // How many lines to wait before sending new horz and vert val
    int myTrakBallLinesH, myTrakBallLinesV;

    // Was TrakBall moved left or moved right instead
    int myTrakBallLeft;

    // Was TrakBall moved down or moved up instead
    int myTrakBallDown;

    int myScanCountH, myScanCountV, myCountH, myCountV;

    // Whether to use the mouse to emulate this controller
    int myMouseEnabled;

  private:
    // Following constructors and assignment operators not supported
    AtariMouse() = delete;
    AtariMouse(const AtariMouse&) = delete;
    AtariMouse(AtariMouse&&) = delete;
    AtariMouse& operator=(const AtariMouse&) = delete;
    AtariMouse& operator=(AtariMouse&&) = delete;
};

#endif

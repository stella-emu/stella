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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef JOYSTICK_HXX
#define JOYSTICK_HXX

#include "bspf.hxx"
#include "Control.hxx"
#include "Event.hxx"

/**
  The standard Atari 2600 joystick controller.

  @author  Bradford W. Mott
  @version $Id$
*/
class Joystick : public Controller
{
  public:
    /**
      Create a new joystick controller plugged into the specified jack

      @param jack   The jack the controller is plugged into
      @param event  The event object to use for events
      @param system The system using this controller
    */
    Joystick(Jack jack, const Event& event, const System& system);

    /**
      Destructor
    */
    virtual ~Joystick();

  public:
    /**
      Update the entire digital and analog pin state according to the
      events currently set.
    */
    void update();

    /**
      Determines how this controller will treat values received from the
      X and Y axis of the mouse.  Since not all controllers use the mouse,
      it's up to the specific class to decide how to use this data.

      If either of the axis is set to 'Automatic', then we automatically
      use this number for the control type as follows:
        0 - paddle 0, joystick 0 (and controllers similar to a joystick)
        1 - paddle 1, joystick 1 (and controllers similar to a joystick)
        2 - paddle 2, joystick 0 (and controllers similar to a joystick)
        3 - paddle 3, joystick 1 (and controllers similar to a joystick)

      @param xaxis   How the controller should use x-axis data
      @param yaxis   How the controller should use y-axis data
      @param ctrlID  The controller ID to use axis 'auto' mode
    */
    void setMouseControl(
        MouseAxisControl xaxis, MouseAxisControl yaxis, int ctrlID = -1);

    /**
      Sets the deadzone amount for real analog joysticks.
      Technically, this isn't really used by the Joystick class at all,
      but it seemed like the best place to put it.
    */
    static void setDeadZone(int deadzone);
    inline static int deadzone() { return _DEAD_ZONE; }

  private:
    // Pre-compute the events we care about based on given port
    // This will eliminate test for left or right port in update()
    Event::Type myUpEvent, myDownEvent, myLeftEvent, myRightEvent,
                myXAxisValue, myYAxisValue, myFireEvent;

    // Controller to emulate in mouse axis 'automatic' mode
    int myControlID;  

    static int _DEAD_ZONE;
};

#endif

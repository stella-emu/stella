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

#ifndef MOUSE_CONTROL_HXX
#define MOUSE_CONTROL_HXX

class Console;
class Controller;
class Properties;

#include "bspf.hxx"
#include "Array.hxx"

/**
  The mouse can control various virtual 'controllers' in many different
  ways.  In 'auto' mode, the entire mouse (both axes and buttons) are used
  as one controller.  In per-ROM axis mode, each axis/button may control
  separate controllers.  As well, we'd like to switch dynamically between
  each of these modes at runtime.

  This class encapsulates all required info to implement this functionality.

  @author  Stephen Anthony
*/
class MouseControl
{
  public:
    /**
      Enumeration of mouse axis control types
    */
    enum Axis
    {
      Paddle0 = 0, Paddle1, Paddle2, Paddle3,
      Driving0, Driving1, Automatic, NoControl
    };

  public:
    /**
      Create a new MouseControl object

      @param console The console in use by the system
      @param mode    Contains information about how to use the mouse axes/buttons
    */
    MouseControl(Console& console, const string& mode);

    /**
      Destructor
    */
    virtual ~MouseControl();

  public:
    /**
      Cycle through each available mouse control mode

      @return  A message explaining the current mouse mode
    */
    const string& next();

  private:
    const Properties& myProps;
    Controller& myLeftController;
    Controller& myRightController;

    struct MouseMode {
      Axis xaxis, yaxis;
      int controlID;
      string message;

      MouseMode()
        : xaxis(NoControl),
          yaxis(NoControl),
          controlID(-1),
          message("Mouse input is disabled")  { }
      MouseMode(Axis x, Axis y, int id, const string& msg)
        : xaxis(x),
          yaxis(y),
          controlID(id),
          message(msg)  { }
    };

    int myCurrentModeNum;
    Common::Array<MouseMode> myModeList;
};

#endif

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

#include "Console.hxx"
#include "Control.hxx"
#include "Props.hxx"

#include "MouseControl.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MouseControl::MouseControl(Console& console, const string& mode)
  : myProps(console.properties()),
    myLeftController(console.controller(Controller::Left)),
    myRightController(console.controller(Controller::Right)),
    myCurrentModeNum(0)
{
cerr << "MouseControl c'tor: using mode = " << mode << endl;

  if(mode == "never")
  {
    MouseMode mmode;
    myModeList.push_back(mmode);
    return;
  }

  // First consider the possible modes for the mouse based on the left
  // and right controllers
  switch(myLeftController.type())
  {
    case Controller::Joystick:
    case Controller::BoosterGrip:
    case Controller::Genesis:
    case Controller::Driving:
    case Controller::TrackBall22:
    case Controller::TrackBall80:
    case Controller::AmigaMouse:
//    case Controller::Mindlink:
    {
      ostringstream msg;
      msg << "Mouse is left " << myLeftController.name() << " controller";
      MouseMode mmode(Automatic, Automatic, 0, msg.str());
      myModeList.push_back(mmode);
      break;
    }
    case Controller::Paddles:
    {
      MouseMode mmode0(Automatic, Automatic, 0, "Mouse is Paddle 0 controller");
      MouseMode mmode1(Automatic, Automatic, 1, "Mouse is Paddle 1 controller");
      myModeList.push_back(mmode0);
      myModeList.push_back(mmode1);
      break;
    }
    default:
      break;
  }
  switch(myRightController.type())
  {
    case Controller::Joystick:
    case Controller::BoosterGrip:
    case Controller::Genesis:
    case Controller::Driving:
    case Controller::TrackBall22:
    case Controller::TrackBall80:
    case Controller::AmigaMouse:
//    case Controller::Mindlink:
    {
      ostringstream msg;
      msg << "Mouse is right " << myRightController.name() << " controller";
      MouseMode mmode(Automatic, Automatic, 1, msg.str());
      myModeList.push_back(mmode);
      break;
    }
    case Controller::Paddles:
    {
      MouseMode mmode0(Automatic, Automatic, 2, "Mouse is Paddle 2 controller");
      MouseMode mmode1(Automatic, Automatic, 3, "Mouse is Paddle 3 controller");
      myModeList.push_back(mmode0);
      myModeList.push_back(mmode1);
      break;
    }
    default:
      break;
  }

  // Now add per-ROM setting (if one exists)
  if(mode != "auto")
  {
/*
    // Note: these constants are from Controller::MouseAxisType enum
    if(s.length() != 2 || s[0] < '0' || s[0] > '7' || s[1] < '0' || s[1] > '7')
      setInternal("mcontrol", "auto");
*/
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MouseControl::~MouseControl()
{
}

#if 0
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MouseControl::setMode(const string& mode)
{
cerr << "MouseControl::setMode: " << mode << endl;


  if(!&myOSystem->console())
    return;

  Controller& lc = myOSystem->console().controller(Controller::Left);
  Controller& rc = myOSystem->console().controller(Controller::Right);
  if(mode == "auto")
  {
    bool swap = myOSystem->console().properties().get(Controller_SwapPaddles) == "YES";
    lc.setMouseControl(Controller::Automatic, Controller::Automatic, swap ? 1 : 0);
    rc.setMouseControl(Controller::Automatic, Controller::Automatic, swap ? 1 : 0);
  }
  else
  {
    Controller::MouseAxisControl xaxis = (Controller::MouseAxisControl)
      ((int)mode[0] - '0');
    Controller::MouseAxisControl yaxis = (Controller::MouseAxisControl)
      ((int)mode[1] - '0');

    lc.setMouseControl(xaxis, yaxis);
    rc.setMouseControl(xaxis, yaxis);
  }
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& MouseControl::next()
{
  const MouseMode& mode = myModeList[myCurrentModeNum];
  myCurrentModeNum = (myCurrentModeNum + 1) % myModeList.size();

  return mode.message;

#if 0
  if(myOSystem->settings().getString("mcontrol") == "auto")
  {
    myOSystem->console().controller(Controller::Left).setMouseControl(
        Controller::Automatic, Controller::Automatic, paddle);
    myOSystem->console().controller(Controller::Right).setMouseControl(
        Controller::Automatic, Controller::Automatic, paddle);

    myOSystem->frameBuffer().showMessage(message);
  }
  else
  {
    myOSystem->frameBuffer().showMessage(
        "Mouse axis mode not auto, paddle not changed");
  }
#endif
}

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
  if(BSPF_equalsIgnoreCase(mode, "never"))
  {
    myModeList.push_back(MouseMode("Mouse input is disabled"));
    return;
  }
  else if(!BSPF_equalsIgnoreCase(mode, "auto") && mode.length() == 2 &&
          mode[0] >= '0' && mode[0] <= '5' &&
          mode[1] >= '0' && mode[1] <= '5')
  {
    Axis xaxis = (Axis) ((int)mode[0] - '0');
    Axis yaxis = (Axis) ((int)mode[1] - '0');
    ostringstream msg;
    msg << "Mouse X-axis is ";
    switch(xaxis)
    {
      case Paddle0:
        msg << "Paddle 0";  break;
      case Paddle1:
        msg << "Paddle 1";  break;
      case Paddle2:
        msg << "Paddle 2";  break;
      case Paddle3:
        msg << "Paddle 3";  break;
      case Driving0:
        msg << "Driving 0"; break;
      case Driving1:
        msg << "Driving 1"; break;
      default:              break;
    }
    msg << ", Y-axis is ";
    switch(yaxis)
    {
      case Paddle0:
        msg << "Paddle 0";  break;
      case Paddle1:
        msg << "Paddle 1";  break;
      case Paddle2:
        msg << "Paddle 2";  break;
      case Paddle3:
        msg << "Paddle 3";  break;
      case Driving0:
        msg << "Driving 0"; break;
      case Driving1:
        msg << "Driving 1"; break;
      default:              break;
    }
    myModeList.push_back(MouseMode(xaxis, yaxis, -1, msg.str()));
  }

  // Now consider the possible modes for the mouse based on the left
  // and right controllers
  bool noswap = BSPF_equalsIgnoreCase(myProps.get(Console_SwapPorts), "NO");
  if(noswap)
  {
    addLeftControllerModes(noswap);
    addRightControllerModes(noswap);
  }
  else
  {
    addRightControllerModes(noswap);
    addLeftControllerModes(noswap);
  }

  // If the mouse isn't used at all, we still need one item in the list
  if(myModeList.size() == 0)
    myModeList.push_back(MouseMode("Mouse not used for current controllers"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MouseControl::~MouseControl()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& MouseControl::next()
{
  const MouseMode& mode = myModeList[myCurrentModeNum];
  myCurrentModeNum = (myCurrentModeNum + 1) % myModeList.size();

  myLeftController.setMouseControl(mode.xaxis, mode.yaxis, mode.controlID);
  myRightController.setMouseControl(mode.xaxis, mode.yaxis, mode.controlID);

  return mode.message;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MouseControl::addLeftControllerModes(bool noswap)
{
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
      myModeList.push_back(MouseMode(Automatic, Automatic, noswap ? 0 : 1, msg.str()));
      break;
    }
    case Controller::Paddles:
      if(noswap)  addPaddleModes(0, 1, 0, 1);
      else        addPaddleModes(2, 3, 0, 1);
      break;
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MouseControl::addRightControllerModes(bool noswap)
{
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
      myModeList.push_back(MouseMode(Automatic, Automatic, noswap ? 1 : 0, msg.str()));
      break;
    }
    case Controller::Paddles:
      if(noswap)  addPaddleModes(2, 3, 2, 3);
      else        addPaddleModes(0, 1, 2, 3);
      break;
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MouseControl::addPaddleModes(int lport, int rport, int lname, int rname)
{
  ostringstream msg;
  msg << "Mouse is Paddle " << lname << " controller" << "(" << lport << ")";
  MouseMode mode0(Automatic, Automatic, lport, msg.str());

  msg.str("");
  msg << "Mouse is Paddle " << rname << " controller" << "(" << rport << ")";
  MouseMode mode1(Automatic, Automatic, rport, msg.str());

  if(BSPF_equalsIgnoreCase(myProps.get(Controller_SwapPaddles), "NO"))
  {
    myModeList.push_back(mode0);
    myModeList.push_back(mode1);
  }
  else
  {
    myModeList.push_back(mode1);
    myModeList.push_back(mode0);
  }
}

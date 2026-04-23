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

#include "Joystick.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Joystick::Joystick(Jack jack, const Event& event, const System& system, bool altmap)
  : Joystick(jack, event, system, Controller::Type::Joystick, altmap)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Joystick::Joystick(Jack jack, const Event& event, const System& system,
                   Controller::Type type, bool altmap)
  : Controller(jack, event, system, type)
{
  struct EventMap { Event::Type up, down, left, right, fire; };
  static constexpr BSPF::array2D<EventMap, 2, 2> eventMaps = {{
    // altmap = false
    {{ {Event::LeftJoystickUp,  Event::LeftJoystickDown,
        Event::LeftJoystickLeft, Event::LeftJoystickRight,
        Event::LeftJoystickFire},
       {Event::RightJoystickUp, Event::RightJoystickDown,
        Event::RightJoystickLeft, Event::RightJoystickRight,
        Event::RightJoystickFire} }},
    // altmap = true
    {{ {Event::QTJoystickThreeUp, Event::QTJoystickThreeDown,
        Event::QTJoystickThreeLeft, Event::QTJoystickThreeRight,
        Event::QTJoystickThreeFire},
       {Event::QTJoystickFourUp, Event::QTJoystickFourDown,
        Event::QTJoystickFourLeft, Event::QTJoystickFourRight,
        Event::QTJoystickFourFire} }}
  }};

  const auto& map = eventMaps[altmap ? 1 : 0][myJack == Jack::Right ? 1 : 0];
  myUpEvent    = map.up;
  myDownEvent  = map.down;
  myLeftEvent  = map.left;
  myRightEvent = map.right;
  myFireEvent  = map.fire;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Joystick::update()
{
  updateButtons();

  updateDigitalAxes();
  updateMouseAxes();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Joystick::updateButtons()
{
  bool firePressed = myEvent.get(myFireEvent) != 0;

  // The joystick uses both mouse buttons for the single joystick button
  updateMouseButtons(firePressed, firePressed);

  setPin(DigitalPin::Six, !getAutoFireState(firePressed));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Joystick::updateMouseButtons(bool& pressedLeft, bool& pressedRight)
{
  if(myControlID > -1)
  {
    pressedLeft  |= (myEvent.get(Event::MouseButtonLeftValue) != 0);
    pressedRight |= (myEvent.get(Event::MouseButtonRightValue) != 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Joystick::updateDigitalAxes()
{
  // Digital events (from keyboard or joystick hats & buttons)
  setPin(DigitalPin::One,   myEvent.get(myUpEvent) == 0);
  setPin(DigitalPin::Two,   myEvent.get(myDownEvent) == 0);
  setPin(DigitalPin::Three, myEvent.get(myLeftEvent) == 0);
  setPin(DigitalPin::Four,  myEvent.get(myRightEvent) == 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Joystick::updateMouseAxes()
{
  // Mouse motion and button events
  if(myControlID > -1)
  {
    // The following code was taken from z26
    static constexpr int MJ_Threshold = 2;
    const int mousex = myEvent.get(Event::MouseAxisXMove),
              mousey = myEvent.get(Event::MouseAxisYMove);

    if(mousex || mousey)
    {
      if((!(abs(mousey) > abs(mousex) << 1)) && (abs(mousex) >= MJ_Threshold))
      {
        if(mousex < 0)      setPin(DigitalPin::Three, false);
        else if(mousex > 0) setPin(DigitalPin::Four, false);
      }

      if((!(abs(mousex) > abs(mousey) << 1)) && (abs(mousey) >= MJ_Threshold))
      {
        if(mousey < 0)      setPin(DigitalPin::One, false);
        else if(mousey > 0) setPin(DigitalPin::Two, false);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Joystick::setMouseControl(
    Controller::Type xtype, int xid, Controller::Type ytype, int yid)
{
  // Currently, the joystick takes full control of the mouse, using both
  // axes for its two degrees of movement
  if(xtype == myType && ytype == myType && xid == yid)
  {
    myControlID = ((myJack == Jack::Left && xid == 0) ||
                   (myJack == Jack::Right && xid == 1)
                  ) ? xid : -1;
  }
  else
    myControlID = -1;

  return true;
}

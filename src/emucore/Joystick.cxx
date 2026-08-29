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
  // The plain joystick's single button is also triggered by both mouse buttons
  updateFire(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Joystick::updateFire(bool bothButtonsFire)
{
  // Bind the fire button to its event and, when the mouse emulates this
  // controller, the mouse button(s) that also trigger it, so each can change
  // the button within the input window instead of latching a static aggregate.
  std::array<Event::Type, MAX_PIN_EVENTS> fire{myFireEvent};
  auto n = 1UZ;
  if(myControlID > -1)
  {
    fire[n++] = Event::MouseButtonLeftValue;
    if(bothButtonsFire)
      fire[n++] = Event::MouseButtonRightValue;
  }

  updateFireButton(DigitalPin::Six, myFireDelay, {fire.data(), n});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Joystick::updateDigitalAxes()
{
  // Digital events (from keyboard or joystick hats & buttons), bound so they
  // reflect their value at the current scanline.  Mouse-driven axes
  // (updateMouseAxes) override these with static values when active.
  bindPin(DigitalPin::One,   myUpEvent);
  bindPin(DigitalPin::Two,   myDownEvent);
  bindPin(DigitalPin::Three, myLeftEvent);
  bindPin(DigitalPin::Four,  myRightEvent);
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

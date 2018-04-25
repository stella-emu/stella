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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "OSystem.hxx"
#include "Console.hxx"
#include "Joystick.hxx"
#include "Settings.hxx"
#include "EventHandler.hxx"
#include "DialogContainer.hxx"
#include "PJoystickHandler.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::PhysicalJoystickHandler(
      OSystem& system, EventHandler& handler, Event& event)
  : myOSystem(system),
    myHandler(handler),
    myEvent(event)
{
  // Load previously saved joystick mapping (if any) from settings
  istringstream buf(myOSystem.settings().getString("joymap"));
  string joymap, joyname;

  // First check the event type, and disregard the entire mapping if it's invalid
  getline(buf, joymap, '^');
  if(atoi(joymap.c_str()) == Event::LastType)
  {
    // Otherwise, put each joystick mapping entry into the database
    while(getline(buf, joymap, '^'))
    {
      istringstream namebuf(joymap);
      getline(namebuf, joyname, '|');
      if(joyname.length() != 0)
        myDatabase.emplace(joyname, StickInfo(joymap));
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int PhysicalJoystickHandler::add(PhysicalJoystickPtr stick)
{
  // Skip if we couldn't open it for any reason
  if(stick->ID < 0)
    return -1;

  // Figure out what type of joystick this is
  bool specialAdaptor = false;

  if(BSPF::containsIgnoreCase(stick->name, "2600-daptor"))
  {
    specialAdaptor = true;
    if(stick->numAxes == 4)
    {
      // TODO - detect controller type based on z-axis
      stick->name = "2600-daptor D9";
    }
    else if(stick->numAxes == 3)
    {
      stick->name = "2600-daptor II";
    }
    else
      stick->name = "2600-daptor";
  }
  else if(BSPF::containsIgnoreCase(stick->name, "Stelladaptor"))
  {
    stick->name = "Stelladaptor";
    specialAdaptor = true;
  }
  else
  {
    // We need unique names for mappable devices
    // For non-unique names that already have a database entry,
    // we append ' #x', where 'x' increases consecutively
    int count = 0;
    for(const auto& i: myDatabase)
      if(BSPF::startsWithIgnoreCase(i.first, stick->name) && i.second.joy)
        ++count;

    if(count > 0)
    {
      ostringstream name;
      name << stick->name << " #" << count+1;
      stick->name = name.str();
    }
    stick->type = PhysicalJoystick::JT_REGULAR;
  }
  // The stick *must* be inserted here, since it may be used below
  mySticks[stick->ID] = stick;

  // Map the stelladaptors we've found according to the specified ports
  if(specialAdaptor)
    mapStelladaptors(myOSystem.settings().getString("saport"));

  // Add stick to database
  auto it = myDatabase.find(stick->name);
  if(it != myDatabase.end())    // already present
  {
    it->second.joy = stick;
    stick->setMap(it->second.mapping);
  }
  else    // adding for the first time
  {
    StickInfo info("", stick);
    myDatabase.emplace(stick->name, info);
    setStickDefaultMapping(stick->ID, Event::NoType, kEmulationMode);
    setStickDefaultMapping(stick->ID, Event::NoType, kMenuMode);
  }

  // We're potentially swapping out an input device behind the back of
  // the Event system, so we make sure all Stelladaptor-generated events
  // are reset
  for(int i = 0; i < 2; ++i)
  {
    for(int j = 0; j < 2; ++j)
      myEvent.set(SA_Axis[i][j], 0);
    for(int j = 0; j < 4; ++j)
      myEvent.set(SA_Button[i][j], 0);
    for(int j = 0; j < 12; ++j)
      myEvent.set(SA_Key[i][j], 0);
  }

  return stick->ID;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::remove(int id)
{
  // When a joystick is removed, we delete the actual joystick object but
  // remember its mapping, since it will eventually be saved to settings

  // Sticks that are removed must have initially been added
  // So we use the 'active' joystick list to access them
  try
  {
    PhysicalJoystickPtr stick = mySticks.at(id);

    auto it = myDatabase.find(stick->name);
    if(it != myDatabase.end() && it->second.joy == stick)
    {
      ostringstream buf;
      buf << "Removed joystick " << mySticks[id]->ID << ":" << endl
          << "  " << mySticks[id]->about() << endl;
      myOSystem.logMessage(buf.str(), 1);

      // Remove joystick, but remember mapping
      it->second.mapping = stick->getMap();
      it->second.joy = nullptr;
      mySticks.erase(id);

      return true;
    }
  }
  catch(const std::out_of_range&)
  {
    // fall through to indicate remove failed
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::remove(const string& name)
{
  auto it = myDatabase.find(name);
  if(it != myDatabase.end() && it->second.joy == nullptr)
  {
    myDatabase.erase(it);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::mapStelladaptors(const string& saport)
{
  // saport will have two values:
  //   'lr' means treat first valid adaptor as left port, second as right port
  //   'rl' means treat first valid adaptor as right port, second as left port
  // We know there will be only two such devices (at most), since the logic
  // in setupJoysticks take care of that
  int saCount = 0;
  int saOrder[2] = { 1, 2 };
  if(BSPF::equalsIgnoreCase(saport, "rl"))
  {
    saOrder[0] = 2; saOrder[1] = 1;
  }

  for(auto& stick: mySticks)
  {
    if(BSPF::startsWithIgnoreCase(stick.second->name, "Stelladaptor"))
    {
      if(saOrder[saCount] == 1)
      {
        stick.second->name += " (emulates left joystick port)";
        stick.second->type = PhysicalJoystick::JT_STELLADAPTOR_LEFT;
      }
      else if(saOrder[saCount] == 2)
      {
        stick.second->name += " (emulates right joystick port)";
        stick.second->type = PhysicalJoystick::JT_STELLADAPTOR_RIGHT;
      }
      saCount++;
    }
    else if(BSPF::startsWithIgnoreCase(stick.second->name, "2600-daptor"))
    {
      if(saOrder[saCount] == 1)
      {
        stick.second->name += " (emulates left joystick port)";
        stick.second->type = PhysicalJoystick::JT_2600DAPTOR_LEFT;
      }
      else if(saOrder[saCount] == 2)
      {
        stick.second->name += " (emulates right joystick port)";
        stick.second->type = PhysicalJoystick::JT_2600DAPTOR_RIGHT;
      }
      saCount++;
    }
  }
  myOSystem.settings().setValue("saport", saport);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::setDefaultMapping(Event::Type event, EventMode mode)
{
  eraseMapping(event, mode);
  for(auto& i: mySticks)
    setStickDefaultMapping(i.first, event, mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::setStickDefaultMapping(int stick,
        Event::Type event, EventMode mode)
{
  bool eraseAll = (event == Event::NoType);

  auto setDefaultAxis = [&](int a_stick, int a_axis, int a_value, Event::Type a_event)
  {
    if(eraseAll || a_event == event)
      myHandler.addJoyAxisMapping(a_event, mode, a_stick, a_axis, a_value, false);
  };
  auto setDefaultBtn = [&](int b_stick, int b_button, Event::Type b_event)
  {
    if(eraseAll || b_event == event)
      myHandler.addJoyButtonMapping(b_event, mode, b_stick, b_button, false);
  };
  auto setDefaultHat = [&](int h_stick, int h_hat, JoyHat h_dir, Event::Type h_event)
  {
    if(eraseAll || h_event == event)
      myHandler.addJoyHatMapping(h_event, mode, h_stick, h_hat, h_dir, false);
  };

  switch(mode)
  {
    case kEmulationMode:  // Default emulation events
      if(stick == 0)
      {
        // Left joystick left/right directions (assume joystick zero)
        setDefaultAxis( 0, 0, 0, Event::JoystickZeroLeft  );
        setDefaultAxis( 0, 0, 1, Event::JoystickZeroRight );
        // Left joystick up/down directions (assume joystick zero)
        setDefaultAxis( 0, 1, 0, Event::JoystickZeroUp    );
        setDefaultAxis( 0, 1, 1, Event::JoystickZeroDown  );
        // Left joystick (assume joystick zero, button zero)
        setDefaultBtn( 0, 0, Event::JoystickZeroFire );
        // Left joystick left/right directions (assume joystick zero and hat 0)
        setDefaultHat( 0, 0, JoyHat::LEFT,  Event::JoystickZeroLeft  );
        setDefaultHat( 0, 0, JoyHat::RIGHT, Event::JoystickZeroRight );
        // Left joystick up/down directions (assume joystick zero and hat 0)
        setDefaultHat( 0, 0, JoyHat::UP,    Event::JoystickZeroUp    );
        setDefaultHat( 0, 0, JoyHat::DOWN,  Event::JoystickZeroDown  );
      }
      else if(stick == 1)
      {
        // Right joystick left/right directions (assume joystick one)
        setDefaultAxis( 1, 0, 0, Event::JoystickOneLeft  );
        setDefaultAxis( 1, 0, 1, Event::JoystickOneRight );
        // Right joystick left/right directions (assume joystick one)
        setDefaultAxis( 1, 1, 0, Event::JoystickOneUp    );
        setDefaultAxis( 1, 1, 1, Event::JoystickOneDown  );
        // Right joystick (assume joystick one, button zero)
        setDefaultBtn( 1, 0, Event::JoystickOneFire );
        // Right joystick left/right directions (assume joystick one and hat 0)
        setDefaultHat( 1, 0, JoyHat::LEFT,  Event::JoystickOneLeft  );
        setDefaultHat( 1, 0, JoyHat::RIGHT, Event::JoystickOneRight );
        // Right joystick up/down directions (assume joystick one and hat 0)
        setDefaultHat( 1, 0, JoyHat::UP,    Event::JoystickOneUp    );
        setDefaultHat( 1, 0, JoyHat::DOWN,  Event::JoystickOneDown  );
      }
      break;

    case kMenuMode:  // Default menu/UI events
      if(stick == 0)
      {
        setDefaultAxis( 0, 0, 0, Event::UILeft  );
        setDefaultAxis( 0, 0, 1, Event::UIRight );
        setDefaultAxis( 0, 1, 0, Event::UIUp    );
        setDefaultAxis( 0, 1, 1, Event::UIDown  );

        // Left joystick (assume joystick zero, button zero)
        setDefaultBtn( 0, 0, Event::UISelect );

        setDefaultHat( 0, 0, JoyHat::LEFT,  Event::UILeft  );
        setDefaultHat( 0, 0, JoyHat::RIGHT, Event::UIRight );
        setDefaultHat( 0, 0, JoyHat::UP,    Event::UIUp    );
        setDefaultHat( 0, 0, JoyHat::DOWN,  Event::UIDown  );
      }
      break;

    default:
      break;
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::eraseMapping(Event::Type event, EventMode mode)
{
  // If event is 'NoType', erase and reset all mappings
  // Otherwise, only reset the given event
  if(event == Event::NoType)
  {
    for(auto& stick: mySticks)
      stick.second->eraseMap(mode);          // erase all events
  }
  else
  {
    for(auto& stick: mySticks)
      stick.second->eraseEvent(event, mode); // only reset the specific event
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::saveMapping()
{
  // Save the joystick mapping hash table, making sure to update it with
  // any changes that have been made during the program run
  ostringstream joybuf;
  joybuf << Event::LastType;

  for(const auto& i: myDatabase)
  {
    const string& map = i.second.joy ? i.second.joy->getMap() : i.second.mapping;
    if(map != "")
      joybuf << "^" << map;
  }
  myOSystem.settings().setValue("joymap", joybuf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string PhysicalJoystickHandler::getMappingDesc(Event::Type event, EventMode mode) const
{
  ostringstream buf;

  for(const auto& s: mySticks)
  {
    uInt32 stick = s.first;
    const PhysicalJoystickPtr j = s.second;
    if(!j)  continue;

    // Joystick button mapping/labeling
    for(int button = 0; button < j->numButtons; ++button)
    {
      if(j->btnTable[button][mode] == event)
      {
        if(buf.str() != "")
          buf << ", ";
        buf << "J" << stick << "/B" << button;
      }
    }

    // Joystick axis mapping/labeling
    for(int axis = 0; axis < j->numAxes; ++axis)
    {
      for(int dir = 0; dir < 2; ++dir)
      {
        if(j->axisTable[axis][dir][mode] == event)
        {
          if(buf.str() != "")
            buf << ", ";
          buf << "J" << stick << "/A" << axis;
          if(Event::isAnalog(event))
          {
            dir = 2;  // Immediately exit the inner loop after this iteration
            buf << "/+|-";
          }
          else if(dir == 0)
            buf << "/-";
          else
            buf << "/+";
        }
      }
    }

    // Joystick hat mapping/labeling
    for(int hat = 0; hat < j->numHats; ++hat)
    {
      for(int dir = 0; dir < 4; ++dir)
      {
        if(j->hatTable[hat][dir][mode] == event)
        {
          if(buf.str() != "")
            buf << ", ";
          buf << "J" << stick << "/H" << hat;
          switch(JoyHat(dir))
          {
            case JoyHat::UP:    buf << "/up";    break;
            case JoyHat::DOWN:  buf << "/down";  break;
            case JoyHat::LEFT:  buf << "/left";  break;
            case JoyHat::RIGHT: buf << "/right"; break;
            default:                             break;
          }
        }
      }
    }
  }
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::addAxisMapping(Event::Type event, EventMode mode,
    int stick, int axis, int value)
{
  const PhysicalJoystickPtr j = joy(stick);
  if(j)
  {
    if(axis >= 0 && axis < j->numAxes && event < Event::LastType)
    {
      // This confusing code is because each axis has two associated values,
      // but analog events only affect one of the axis.
      if(Event::isAnalog(event))
        j->axisTable[axis][0][mode] = j->axisTable[axis][1][mode] = event;
      else
      {
        // Otherwise, turn off the analog event(s) for this axis
        if(Event::isAnalog(j->axisTable[axis][0][mode]))
          j->axisTable[axis][0][mode] = Event::NoType;
        if(Event::isAnalog(j->axisTable[axis][1][mode]))
          j->axisTable[axis][1][mode] = Event::NoType;

        j->axisTable[axis][(value > 0)][mode] = event;
      }
      return true;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::addBtnMapping(Event::Type event, EventMode mode,
    int stick, int button)
{
  const PhysicalJoystickPtr j = joy(stick);
  if(j)
  {
    if(button >= 0 && button < j->numButtons && event < Event::LastType)
    {
      j->btnTable[button][mode] = event;
      return true;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::addHatMapping(Event::Type event, EventMode mode,
    int stick, int hat, JoyHat value)
{
  const PhysicalJoystickPtr j = joy(stick);
  if(j)
  {
    if(hat >= 0 && hat < j->numHats && event < Event::LastType &&
       value != JoyHat::CENTER)
    {
      j->hatTable[hat][int(value)][mode] = event;
      return true;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::handleAxisEvent(int stick, int axis, int value)
{
  const PhysicalJoystickPtr j = joy(stick);
  if(!j)  return;

  // Stelladaptors handle axis differently than regular joysticks
  switch(j->type)
  {
    case PhysicalJoystick::JT_REGULAR:
      if(myHandler.state() == EventHandlerState::EMULATION)
      {
        // Every axis event has two associated values, negative and positive
        Event::Type eventAxisNeg = j->axisTable[axis][0][kEmulationMode];
        Event::Type eventAxisPos = j->axisTable[axis][1][kEmulationMode];

        // Check for analog events, which are handled differently
        // We'll pass them off as Stelladaptor events, and let the controllers
        // handle it
        switch(int(eventAxisNeg))
        {
          case Event::PaddleZeroAnalog:
            myEvent.set(Event::SALeftAxis0Value, value);
            break;
          case Event::PaddleOneAnalog:
            myEvent.set(Event::SALeftAxis1Value, value);
            break;
          case Event::PaddleTwoAnalog:
            myEvent.set(Event::SARightAxis0Value, value);
            break;
          case Event::PaddleThreeAnalog:
            myEvent.set(Event::SARightAxis1Value, value);
            break;
          default:
          {
            // Otherwise, we know the event is digital
            if(value > Joystick::deadzone())
              myHandler.handleEvent(eventAxisPos, 1);
            else if(value < -Joystick::deadzone())
              myHandler.handleEvent(eventAxisNeg, 1);
            else
            {
              // Treat any deadzone value as zero
              value = 0;

              // Now filter out consecutive, similar values
              // (only pass on the event if the state has changed)
              if(j->axisLastValue[axis] != value)
              {
                // Turn off both events, since we don't know exactly which one
                // was previously activated.
                myHandler.handleEvent(eventAxisNeg, 0);
                myHandler.handleEvent(eventAxisPos, 0);
              }
            }
            j->axisLastValue[axis] = value;
            break;
          }
        }
      }
      else if(myHandler.hasOverlay())
      {
        // First, clamp the values to simulate digital input
        // (the only thing that the underlying code understands)
        if(value > Joystick::deadzone())
          value = 32000;
        else if(value < -Joystick::deadzone())
          value = -32000;
        else
          value = 0;

        // Now filter out consecutive, similar values
        // (only pass on the event if the state has changed)
        if(value != j->axisLastValue[axis])
        {
          myHandler.overlay().handleJoyAxisEvent(stick, axis, value);
          j->axisLastValue[axis] = value;
        }
      }
      break;  // Regular joystick axis

    // Since the various controller classes deal with Stelladaptor
    // devices differently, we send the raw X and Y axis data directly,
    // and let the controller handle it
    // These events don't have to pass through handleEvent, since
    // they can never be remapped
    case PhysicalJoystick::JT_STELLADAPTOR_LEFT:
    case PhysicalJoystick::JT_2600DAPTOR_LEFT:
      if(axis < 2)
        myEvent.set(SA_Axis[0][axis], value);
      break;  // axis on left controller (0)
    case PhysicalJoystick::JT_STELLADAPTOR_RIGHT:
    case PhysicalJoystick::JT_2600DAPTOR_RIGHT:
      if(axis < 2)
        myEvent.set(SA_Axis[1][axis], value);
      break;  // axis on right controller (1)
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::handleBtnEvent(int stick, int button, uInt8 state)
{
  const PhysicalJoystickPtr j = joy(stick);
  if(!j)  return;

  // Stelladaptors handle buttons differently than regular joysticks
  switch(j->type)
  {
    case PhysicalJoystick::JT_REGULAR:
      // Handle buttons which switch eventhandler state
      if(state && myHandler.changeStateByEvent(j->btnTable[button][kEmulationMode]))
        return;

      // Determine which mode we're in, then send the event to the appropriate place
      if(myHandler.state() == EventHandlerState::EMULATION)
        myHandler.handleEvent(j->btnTable[button][kEmulationMode], state);
      else if(myHandler.hasOverlay())
        myHandler.overlay().handleJoyBtnEvent(stick, button, state);
      break;  // Regular button

    // These events don't have to pass through handleEvent, since
    // they can never be remapped
    case PhysicalJoystick::JT_STELLADAPTOR_LEFT:
    case PhysicalJoystick::JT_STELLADAPTOR_RIGHT:
      // The 'type-2' here refers to the fact that 'PhysicalJoystick::JT_STELLADAPTOR_LEFT'
      // and 'PhysicalJoystick::JT_STELLADAPTOR_RIGHT' are at index 2 and 3 in the JoyType
      // enum; subtracting two gives us Controller 0 and 1
      if(button < 2) myEvent.set(SA_Button[j->type-2][button], state);
      break;  // Stelladaptor button
    case PhysicalJoystick::JT_2600DAPTOR_LEFT:
    case PhysicalJoystick::JT_2600DAPTOR_RIGHT:
      // The 'type-4' here refers to the fact that 'PhysicalJoystick::JT_2600DAPTOR_LEFT'
      // and 'PhysicalJoystick::JT_2600DAPTOR_RIGHT' are at index 4 and 5 in the JoyType
      // enum; subtracting four gives us Controller 0 and 1
      if(myHandler.state() == EventHandlerState::EMULATION)
      {
        switch(myOSystem.console().leftController().type())
        {
          case Controller::Keyboard:
            if(button < 12) myEvent.set(SA_Key[j->type-4][button], state);
            break;
          default:
            if(button < 4) myEvent.set(SA_Button[j->type-4][button], state);
        }
        switch(myOSystem.console().rightController().type())
        {
          case Controller::Keyboard:
            if(button < 12) myEvent.set(SA_Key[j->type-4][button], state);
            break;
          default:
            if(button < 4) myEvent.set(SA_Button[j->type-4][button], state);
        }
      }
      break;  // 2600DAPTOR button
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::handleHatEvent(int stick, int hat, int value)
{
  // Preprocess all hat events, converting to Stella JoyHat type
  // Generate multiple equivalent hat events representing combined direction
  // when we get a diagonal hat event
  if(myHandler.state() == EventHandlerState::EMULATION)
  {
    const PhysicalJoystickPtr j = joy(stick);
    if(!j)  return;

    myHandler.handleEvent(j->hatTable[hat][int(JoyHat::UP)][kEmulationMode],
        value & EVENT_HATUP_M);
    myHandler.handleEvent(j->hatTable[hat][int(JoyHat::RIGHT)][kEmulationMode],
        value & EVENT_HATRIGHT_M);
    myHandler.handleEvent(j->hatTable[hat][int(JoyHat::DOWN)][kEmulationMode],
        value & EVENT_HATDOWN_M);
    myHandler.handleEvent(j->hatTable[hat][int(JoyHat::LEFT)][kEmulationMode],
        value & EVENT_HATLEFT_M);
  }
  else if(myHandler.hasOverlay())
  {
    if(value == EVENT_HATCENTER_M)
      myHandler.overlay().handleJoyHatEvent(stick, hat, JoyHat::CENTER);
    else
    {
      if(value & EVENT_HATUP_M)
        myHandler.overlay().handleJoyHatEvent(stick, hat, JoyHat::UP);
      if(value & EVENT_HATRIGHT_M)
        myHandler.overlay().handleJoyHatEvent(stick, hat, JoyHat::RIGHT);
      if(value & EVENT_HATDOWN_M)
        myHandler.overlay().handleJoyHatEvent(stick, hat, JoyHat::DOWN);
      if(value & EVENT_HATLEFT_M)
        myHandler.overlay().handleJoyHatEvent(stick, hat, JoyHat::LEFT);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VariantList PhysicalJoystickHandler::database() const
{
  VariantList db;
  for(const auto& i: myDatabase)
    VarList::push_back(db, i.first, i.second.joy ? i.second.joy->ID : -1);

  return db;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream& operator<<(ostream& os, const PhysicalJoystickHandler& jh)
{
  os << "---------------------------------------------------------" << endl
     << "joy database:"  << endl;
  for(const auto& i: jh.myDatabase)
    os << i.first << endl << i.second << endl << endl;

  os << "---------------------" << endl
     << "joy active:"  << endl;
  for(const auto& i: jh.mySticks)
    os << i.first << ": " << *i.second << endl;
  os << "---------------------------------------------------------"
     << endl << endl << endl;

  return os;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used by the Stelladaptor to send absolute axis values
const Event::Type PhysicalJoystickHandler::SA_Axis[2][2] = {
  { Event::SALeftAxis0Value,  Event::SALeftAxis1Value  },
  { Event::SARightAxis0Value, Event::SARightAxis1Value }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used by the Stelladaptor to map button presses to joystick or paddles
//  (driving controllers and boostergrip are considered the same as joysticks)
const Event::Type PhysicalJoystickHandler::SA_Button[2][4] = {
  { Event::JoystickZeroFire,  Event::JoystickZeroFire9,
    Event::JoystickZeroFire5, Event::JoystickZeroFire9 },
  { Event::JoystickOneFire,   Event::JoystickOneFire9,
    Event::JoystickOneFire5,  Event::JoystickOneFire9  }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used by the 2600-daptor to map button presses to keypad keys
const Event::Type PhysicalJoystickHandler::SA_Key[2][12] = {
  { Event::KeyboardZero1,    Event::KeyboardZero2,  Event::KeyboardZero3,
    Event::KeyboardZero4,    Event::KeyboardZero5,  Event::KeyboardZero6,
    Event::KeyboardZero7,    Event::KeyboardZero8,  Event::KeyboardZero9,
    Event::KeyboardZeroStar, Event::KeyboardZero0,  Event::KeyboardZeroPound },
  { Event::KeyboardOne1,     Event::KeyboardOne2,   Event::KeyboardOne3,
    Event::KeyboardOne4,     Event::KeyboardOne5,   Event::KeyboardOne6,
    Event::KeyboardOne7,     Event::KeyboardOne8,   Event::KeyboardOne9,
    Event::KeyboardOneStar,  Event::KeyboardOne0,   Event::KeyboardOnePound  }
};

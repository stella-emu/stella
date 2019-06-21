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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Logger.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "Joystick.hxx"
#include "Settings.hxx"
#include "EventHandler.hxx"
#include "PJoystickHandler.hxx"

#ifdef GUI_SUPPORT
  #include "DialogContainer.hxx"
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::PhysicalJoystickHandler(
      OSystem& system, EventHandler& handler, Event& event)
  : myOSystem(system),
    myHandler(handler),
    myEvent(event)
{
  Int32 version = myOSystem.settings().getInt("event_ver");
  // Load previously saved joystick mapping (if any) from settings
  istringstream buf(myOSystem.settings().getString("joymap"));
  string joymap, joyname;

  // First compare if event list version has changed, and disregard the entire mapping if true
  getline(buf, joymap, '^'); // event list size, ignore
  if(version != Event::VERSION)
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
  for(int i = 0; i < NUM_PORTS; ++i)
  {
    for(int j = 0; j < NUM_JOY_AXIS; ++j)
      myEvent.set(SA_Axis[i][j], 0);
    for(int j = 0; j < NUM_JOY_BTN; ++j)
      myEvent.set(SA_Button[i][j], 0);
    for(int j = 0; j < NUM_KEY_BTN; ++j)
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
      Logger::log(buf.str(), 1);

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
  int saOrder[NUM_PORTS] = { 1, 2 };
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

  auto setDefaultAxis = [&](Event::Type a_event, int stick, JoyAxis axis, JoyDir value, int button = JOY_CTRL_NONE)
  {
    if (eraseAll || a_event == event)
    {
      //myHandler.addJoyAxisMapping(a_event, mode, stick, int(axis), int(value), false);
      myHandler.addJoyMapping(a_event, mode, stick, button, axis, int(value), false);
    }
  };
  auto setDefaultBtn = [&](Event::Type b_event, int stick, int button)
  {
    if (eraseAll || b_event == event)
    {
      //myHandler.addJoyButtonMapping(b_event, mode, stick, button, false);
      myHandler.addJoyMapping(b_event, mode, stick, button, JoyAxis::NONE, int(JoyDir::NONE), false);
    }
  };
  auto setDefaultHat = [&](Event::Type h_event, int stick, int hat, JoyHat dir, int button = JOY_CTRL_NONE)
  {
    if (eraseAll || h_event == event)
    {
      //myHandler.addJoyHatMapping(h_event, mode, stick, hat, dir, false);
      myHandler.addJoyHatMapping(h_event, mode, stick, button, hat, dir, false);
    }
  };

  switch(mode)
  {
    case kEmulationMode:  // Default emulation events
      switch (stick)
      {
        case 0:
        case 2:
          // Left joystick left/right directions (assume joystick zero or two)
          setDefaultAxis(Event::JoystickZeroLeft,   stick, JoyAxis::X, JoyDir::NEG);
          setDefaultAxis(Event::JoystickZeroRight,  stick, JoyAxis::X, JoyDir::POS);
          // Left joystick up/down directions (assume joystick zero or two)
          setDefaultAxis(Event::JoystickZeroUp,     stick, JoyAxis::Y, JoyDir::NEG);
          setDefaultAxis(Event::JoystickZeroDown,   stick, JoyAxis::Y, JoyDir::POS);
          // Left joystick (assume joystick zero or two, buttons zero..two)
          setDefaultBtn(Event::JoystickZeroFire,    stick, 0);
          setDefaultBtn(Event::JoystickZeroFire5,   stick, 1);
          setDefaultBtn(Event::JoystickZeroFire9,   stick, 2);
          // Left joystick left/right directions (assume joystick zero or two and hat 0)
          setDefaultHat(Event::JoystickZeroLeft,    stick, 0, JoyHat::LEFT);
          setDefaultHat(Event::JoystickZeroRight,   stick, 0, JoyHat::RIGHT);
          // Left joystick up/down directions (assume joystick zero or two and hat 0)
          setDefaultHat(Event::JoystickZeroUp,      stick, 0, JoyHat::UP);
          setDefaultHat(Event::JoystickZeroDown,    stick, 0, JoyHat::DOWN);
        #if defined(RETRON77)
          // Left joystick (assume joystick zero or two, buttons two..four)
          setDefaultBtn(Event::CmdMenuMode,         stick, 2);
          setDefaultBtn(Event::OptionsMenuMode,     stick, 3);
          setDefaultBtn(Event::ExitMode,            stick, 4);
        #endif
          break;
        case 1:
        case 3:
          // Right joystick left/right directions (assume joystick one or three)
          setDefaultAxis(Event::JoystickOneLeft,    stick, JoyAxis::X, JoyDir::NEG);
          setDefaultAxis(Event::JoystickOneRight,   stick, JoyAxis::X, JoyDir::POS);
          // Right joystick left/right directions (assume joystick one or three)
          setDefaultAxis(Event::JoystickOneUp,      stick, JoyAxis::Y, JoyDir::NEG);
          setDefaultAxis(Event::JoystickOneDown,    stick, JoyAxis::Y, JoyDir::POS);
          // Right joystick (assume joystick one or three, buttons zero..two)
          setDefaultBtn(Event::JoystickOneFire,     stick, 0);
          setDefaultBtn(Event::JoystickOneFire5,    stick, 1);
          setDefaultBtn(Event::JoystickOneFire9,    stick, 2);
          // Right joystick left/right directions (assume joystick one or three and hat 0)
          setDefaultHat(Event::JoystickOneLeft,     stick, 0, JoyHat::LEFT);
          setDefaultHat(Event::JoystickOneRight,    stick, 0, JoyHat::RIGHT);
          // Right joystick up/down directions (assume joystick one or three and hat 0)
          setDefaultHat(Event::JoystickOneUp,       stick, 0, JoyHat::UP);
          setDefaultHat(Event::JoystickOneDown,     stick, 0, JoyHat::DOWN);
        #if defined(RETRON77)
          // Right joystick (assume joystick one or three, buttons two..four)
          setDefaultBtn(Event::CmdMenuMode,         stick, 2);
          setDefaultBtn(Event::OptionsMenuMode,     stick, 3);
          setDefaultBtn(Event::ExitMode,            stick, 4);
          setDefaultBtn(Event::RewindPause,         stick, 5);
          setDefaultBtn(Event::ConsoleSelect,       stick, 7);
          setDefaultBtn(Event::ConsoleReset,        stick, 8);
        #endif
          break;
        default:
          break;
      }
      break;

    case kMenuMode:  // Default menu/UI events
      setDefaultAxis(Event::UINavPrev,              stick, JoyAxis::X, JoyDir::NEG);
      setDefaultAxis(Event::UINavNext,              stick, JoyAxis::X, JoyDir::POS);
      setDefaultAxis(Event::UIUp,                   stick, JoyAxis::Y, JoyDir::NEG);
      setDefaultAxis(Event::UIDown,                 stick, JoyAxis::Y, JoyDir::POS);

      // joystick (assume buttons zero..three)
      setDefaultBtn(Event::UISelect,                stick, 0);
      setDefaultBtn(Event::UIOK,                    stick, 1);
      setDefaultBtn(Event::UITabNext,               stick, 2);
      setDefaultBtn(Event::UITabPrev,               stick, 3);
      setDefaultBtn(Event::UICancel,                stick, 5);

      setDefaultHat(Event::UINavPrev,               stick, 0, JoyHat::LEFT);
      setDefaultHat(Event::UINavNext,               stick, 0, JoyHat::RIGHT);
      setDefaultHat(Event::UIUp,                    stick, 0, JoyHat::UP);
      setDefaultHat(Event::UIDown,                  stick, 0, JoyHat::DOWN);
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
  //joybuf << Event::LastType;

  for(const auto& i: myDatabase)
  {
    const string& map = i.second.joy ? i.second.joy->getMap() : i.second.mapping;
    if(map != "")
      //joybuf << "^" << map;
      joybuf << map;
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

    /*// Joystick button mapping/labeling
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
      for(int dir = 0; dir < NUM_JOY_DIRS; ++dir)
      {
        if(j->axisTable[axis][dir][mode] == event)
        {
          if(buf.str() != "")
            buf << ", ";
          buf << "J" << stick << "/A" << axis;
          if(Event::isAnalog(event))
          {
            dir = NUM_JOY_DIRS;  // Immediately exit the inner loop after this iteration
            buf << "/+|-";
          }
          else if(dir == int(JoyDir::NEG))
            buf << "/-";
          else
            buf << "/+";
        }
      }
    }

    // Joystick hat mapping/labeling
    for(int hat = 0; hat < j->numHats; ++hat)
    {
      for(int dir = 0; dir < NUM_JOY_HAT_DIRS; ++dir)
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
    }*/

    // new:
    //Joystick button + axis mapping / labeling
    if (j->joyMap.getEventMapping(event, mode).size())
      buf << j->joyMap.getEventMappingDesc(stick, event, mode);
    // Joystick hat mapping/labeling
    if (j->joyHatMap.getEventMapping(event, mode).size())
      buf << j->joyHatMap.getEventMappingDesc(stick, event, mode);
  }
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*bool PhysicalJoystickHandler::addAxisMapping(Event::Type event, EventMode mode,
    int stick, int axis, int value)
{
  const PhysicalJoystickPtr j = joy(stick);
  if(j)
  {
    if(int(axis) >= 0 && int(axis) < j->numAxes && event < Event::LastType)
    {
      // This confusing code is because each axis has two associated values,
      // but analog events only affect one of the axis.
      if(Event::isAnalog(event))
        j->axisTable[axis][int(JoyDir::NEG)][mode] = j->axisTable[axis][int(JoyDir::POS)][mode] = event;
      else
      {
        // Otherwise, turn off the analog event(s) for this axis
        if(Event::isAnalog(j->axisTable[axis][int(JoyDir::NEG)][mode]))
          j->axisTable[axis][int(JoyDir::NEG)][mode] = Event::NoType;
        if(Event::isAnalog(j->axisTable[axis][int(JoyDir::POS)][mode]))
          j->axisTable[axis][int(JoyDir::POS)][mode] = Event::NoType;

        j->axisTable[axis][int(value > 0 ? JoyDir::POS : JoyDir::NEG)][mode] = event;
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
}*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::addJoyMapping(Event::Type event, EventMode mode, int stick,
    int button, JoyAxis axis, int value)
{
  const PhysicalJoystickPtr j = joy(stick);

  if (j && event < Event::LastType &&
      button >= JOY_CTRL_NONE && button < j->numButtons &&
      axis >= JoyAxis::NONE && int(axis) < j->numAxes)
  {
    // This confusing code is because each axis has two associated values,
    // but analog events only affect one of the axis.
    if (Event::isAnalog(event))
    {
      j->joyMap.add(event, mode, button, axis, JoyDir::NEG);
      j->joyMap.add(event, mode, button, axis, JoyDir::POS);
    }
    else
    {
      // Otherwise, turn off the analog event(s) for this axis
      if (Event::isAnalog(j->joyMap.get(mode, button, axis, JoyDir::NEG)))
        j->joyMap.erase(mode, button, axis, JoyDir::NEG);
      if (Event::isAnalog(j->joyMap.get(mode, button, axis, JoyDir::POS)))
        j->joyMap.erase(mode, button, axis, JoyDir::POS);

      j->joyMap.add(event, mode, button, axis, value == int(JoyDir::NONE) ? JoyDir::NONE : value > 0 ? JoyDir::POS : JoyDir::NEG);
    }
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::addJoyHatMapping(Event::Type event, EventMode mode, int stick,
                                               int button, int hat, JoyHat dir)
{
  const PhysicalJoystickPtr j = joy(stick);

  if (j && event < Event::LastType &&
      button >= JOY_CTRL_NONE && button < j->numButtons &&
      hat >= 0 && hat < j->numHats && dir != JoyHat::CENTER)
  {
    j->joyHatMap.add(event, mode, button, hat, dir);
    return true;
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
        //Event::Type eventAxisNeg = j->axisTable[axis][int(JoyDir::NEG)][kEmulationMode];
        //Event::Type eventAxisPos = j->axisTable[axis][int(JoyDir::POS)][kEmulationMode];
        Event::Type eventAxisNeg = j->joyMap.get(kEmulationMode, j->buttonLast[stick], JoyAxis(axis), JoyDir::NEG);
        Event::Type eventAxisPos = j->joyMap.get(kEmulationMode, j->buttonLast[stick], JoyAxis(axis), JoyDir::POS);

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
              myHandler.handleEvent(eventAxisPos);
            else if(value < -Joystick::deadzone())
              myHandler.handleEvent(eventAxisNeg);
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
                myHandler.handleEvent(eventAxisNeg, false);
                myHandler.handleEvent(eventAxisPos, false);
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
        #ifdef GUI_SUPPORT
          myHandler.overlay().handleJoyAxisEvent(stick, axis, value);
        #endif
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
      if(axis < NUM_JOY_AXIS)
        myEvent.set(SA_Axis[int(Controller::Jack::Left)][axis], value);
      break;  // axis on left controller (0)
    case PhysicalJoystick::JT_STELLADAPTOR_RIGHT:
    case PhysicalJoystick::JT_2600DAPTOR_RIGHT:
      if(axis < NUM_JOY_AXIS)
        myEvent.set(SA_Axis[int(Controller::Jack::Right)][axis], value);
      break;  // axis on right controller (1)
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::handleBtnEvent(int stick, int button, bool pressed)
{
  const PhysicalJoystickPtr j = joy(stick);
  if(!j) return;
  j->buttonLast[stick] = pressed ? button : JOY_CTRL_NONE;

  // Stelladaptors handle buttons differently than regular joysticks
  switch(j->type)
  {
    case PhysicalJoystick::JT_REGULAR:
      // Handle buttons which switch eventhandler state
      //if(pressed && myHandler.changeStateByEvent(j->btnTable[button][kEmulationMode]))
      if (pressed && myHandler.changeStateByEvent(j->joyMap.get(kEmulationMode, button)))
        return;

      // Determine which mode we're in, then send the event to the appropriate place
      if(myHandler.state() == EventHandlerState::EMULATION)
        //myHandler.handleEvent(j->btnTable[button][kEmulationMode], pressed);
        myHandler.handleEvent(j->joyMap.get(kEmulationMode, button), pressed);
    #ifdef GUI_SUPPORT
      else if(myHandler.hasOverlay())
        myHandler.overlay().handleJoyBtnEvent(stick, button, pressed);
    #endif
      break;  // Regular button

    // These events don't have to pass through handleEvent, since
    // they can never be remapped
    case PhysicalJoystick::JT_STELLADAPTOR_LEFT:
    case PhysicalJoystick::JT_STELLADAPTOR_RIGHT:
      // The 'type-2' here refers to the fact that 'PhysicalJoystick::JT_STELLADAPTOR_LEFT'
      // and 'PhysicalJoystick::JT_STELLADAPTOR_RIGHT' are at index 2 and 3 in the JoyType
      // enum; subtracting two gives us Controller 0 and 1
      if(button < 2)
        myEvent.set(SA_Button[j->type-PhysicalJoystick::JT_STELLADAPTOR_LEFT][button], pressed ? 1 : 0);
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
          case Controller::Type::Keyboard:
            if(button < NUM_KEY_BTN)
              myEvent.set(SA_Key[j->type-PhysicalJoystick::JT_2600DAPTOR_LEFT][button], pressed ? 1 : 0);
            break;
          default:
            if(button < NUM_JOY_BTN) myEvent.set(SA_Button[j->type-PhysicalJoystick::JT_2600DAPTOR_LEFT][button], pressed ? 1 : 0);
        }
        switch(myOSystem.console().rightController().type())
        {
          case Controller::Type::Keyboard:
            if(button < NUM_KEY_BTN)
              myEvent.set(SA_Key[j->type-PhysicalJoystick::JT_2600DAPTOR_LEFT][button], pressed ? 1 : 0);
            break;
          default:
            if(button < NUM_JOY_BTN) myEvent.set(SA_Button[j->type-PhysicalJoystick::JT_2600DAPTOR_LEFT][button], pressed ? 1 : 0);
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

    /*myHandler.handleEvent(j->hatTable[hat][int(JoyHat::UP)][kEmulationMode],
        value & EVENT_HATUP_M);
    myHandler.handleEvent(j->hatTable[hat][int(JoyHat::RIGHT)][kEmulationMode],
        value & EVENT_HATRIGHT_M);
    myHandler.handleEvent(j->hatTable[hat][int(JoyHat::DOWN)][kEmulationMode],
        value & EVENT_HATDOWN_M);
    myHandler.handleEvent(j->hatTable[hat][int(JoyHat::LEFT)][kEmulationMode],
        value & EVENT_HATLEFT_M);*/

    // TODO: 4 different events
    myHandler.handleEvent(j->joyHatMap.get(kEmulationMode, JOY_CTRL_NONE, hat, JoyHat(value)));
  }
#ifdef GUI_SUPPORT
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
#endif
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
const Event::Type PhysicalJoystickHandler::SA_Axis[NUM_PORTS][NUM_JOY_AXIS] = {
  { Event::SALeftAxis0Value,  Event::SALeftAxis1Value  },
  { Event::SARightAxis0Value, Event::SARightAxis1Value }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used by the Stelladaptor to map button presses to joystick or paddles
//  (driving controllers and boostergrip are considered the same as joysticks)
const Event::Type PhysicalJoystickHandler::SA_Button[NUM_PORTS][NUM_JOY_BTN] = {
  { Event::JoystickZeroFire,  Event::JoystickZeroFire9,
    Event::JoystickZeroFire5, Event::JoystickZeroFire9 },
  { Event::JoystickOneFire,   Event::JoystickOneFire9,
    Event::JoystickOneFire5,  Event::JoystickOneFire9  }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used by the 2600-daptor to map button presses to keypad keys
const Event::Type PhysicalJoystickHandler::SA_Key[NUM_PORTS][NUM_KEY_BTN] = {
  { Event::KeyboardZero1,    Event::KeyboardZero2,  Event::KeyboardZero3,
    Event::KeyboardZero4,    Event::KeyboardZero5,  Event::KeyboardZero6,
    Event::KeyboardZero7,    Event::KeyboardZero8,  Event::KeyboardZero9,
    Event::KeyboardZeroStar, Event::KeyboardZero0,  Event::KeyboardZeroPound },
  { Event::KeyboardOne1,     Event::KeyboardOne2,   Event::KeyboardOne3,
    Event::KeyboardOne4,     Event::KeyboardOne5,   Event::KeyboardOne6,
    Event::KeyboardOne7,     Event::KeyboardOne8,   Event::KeyboardOne9,
    Event::KeyboardOneStar,  Event::KeyboardOne0,   Event::KeyboardOnePound  }
};

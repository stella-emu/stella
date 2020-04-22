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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
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

static constexpr char CTRL_DELIM = '^';

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::PhysicalJoystickHandler(
      OSystem& system, EventHandler& handler)
  : myOSystem(system),
    myHandler(handler)
{
  Int32 version = myOSystem.settings().getInt("event_ver");
  // Load previously saved joystick mapping (if any) from settings
  istringstream buf(myOSystem.settings().getString("joymap"));
  string joymap, joyname;

  // First compare if event list version has changed, and disregard the entire
  // mapping if true
  getline(buf, joymap, CTRL_DELIM); // event list size, ignore
  if(version == Event::VERSION)
  {
    // Otherwise, put each joystick mapping entry into the database
    while(getline(buf, joymap, CTRL_DELIM))
    {
      istringstream namebuf(joymap);
      getline(namebuf, joyname, PhysicalJoystick::MODE_DELIM);
      if(joyname.length() != 0)
        myDatabase.emplace(joyname, StickInfo(joymap));
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int PhysicalJoystickHandler::add(const PhysicalJoystickPtr& stick)
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
  if(it != myDatabase.end()) // already present
  {
    it->second.joy = stick;
    stick->setMap(it->second.mapping);
    enableEmulationMappings();
  }
  else // adding for the first time
  {
    StickInfo info("", stick);
    myDatabase.emplace(stick->name, info);
    setStickDefaultMapping(stick->ID, Event::NoType, EventMode::kEmulationMode, true);
    setStickDefaultMapping(stick->ID, Event::NoType, EventMode::kMenuMode, true);
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
      Logger::info(buf.str());

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
    // remove previously added emulated ports
    size_t pos = stick.second->name.find(" (emulates ");

    if(pos != std::string::npos)
      stick.second->name.erase(pos);

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
// Depending on parameters, this method does the following:
// 1. update all events with default (event == Event::NoType, updateDefault == true)
// 2. reset all events to default    (event == Event::NoType, updateDefault == false)
// 3. reset one event to default     (event != Event::NoType)
void PhysicalJoystickHandler::setDefaultAction(int stick,
                                               EventMapping map, Event::Type event,
                                               EventMode mode, bool updateDefaults)
{
  const PhysicalJoystickPtr j = joy(stick);

  // If event is 'NoType', erase and reset all mappings
  // Otherwise, only reset the given event
  bool eraseAll = !updateDefaults && (event == Event::NoType);

  if(updateDefaults)
  {
    // if there is no existing mapping for the event and
    //  the default mapping for the event is unused, set default key for event
    if(j->joyMap.getEventMapping(map.event, mode).size() == 0 &&
       !j->joyMap.check(mode, map.button, map.axis, map.adir, map.hat, map.hdir))
    {
      if (map.hat == JOY_CTRL_NONE)
        addJoyMapping(map.event, mode, stick, map.button, map.axis, map.adir);
      else
        addJoyHatMapping(map.event, mode, stick, map.button, map.hat, map.hdir);
    }
  }
  else if(eraseAll || map.event == event)
  {
    // TODO: allow for multiple defaults
    //j->joyMap.eraseEvent(map.event, mode);
    if (map.hat == JOY_CTRL_NONE)
      addJoyMapping(map.event, mode, stick, map.button, map.axis, map.adir);
    else
      addJoyHatMapping(map.event, mode, stick, map.button, map.hat, map.hdir);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::setStickDefaultMapping(int stick, Event::Type event,
                                                     EventMode mode, bool updateDefaults)
{
  const PhysicalJoystickPtr j = joy(stick);

  if(j)
  {
    switch (mode)
    {
      case EventMode::kEmulationMode:
        if((stick % 2) == 0) // even sticks
        {
          // put all controller events into their own mode's mappings
          for (const auto& item : DefaultLeftJoystickMapping)
            setDefaultAction(stick, item, event, EventMode::kJoystickMode, updateDefaults);
          for (const auto& item : DefaultLeftPaddlesMapping)
            setDefaultAction(stick, item, event, EventMode::kPaddlesMode, updateDefaults);
          for (const auto& item : DefaultLeftKeypadMapping)
            setDefaultAction(stick, item, event, EventMode::kKeypadMode, updateDefaults);
        }
        else // odd sticks
        {
          // put all controller events into their own mode's mappings
          for (const auto& item : DefaultRightJoystickMapping)
            setDefaultAction(stick, item, event, EventMode::kJoystickMode, updateDefaults);
          for (const auto& item : DefaultRightPaddlesMapping)
            setDefaultAction(stick, item, event, EventMode::kPaddlesMode, updateDefaults);
          for (const auto& item : DefaultRightKeypadMapping)
            setDefaultAction(stick, item, event, EventMode::kKeypadMode, updateDefaults);
        }
        for(const auto& item : DefaultCommonMapping)
          setDefaultAction(stick, item, event, EventMode::kCommonMode, updateDefaults);
        // update running emulation mapping too
        enableEmulationMappings();
        break;

      case EventMode::kMenuMode:
        for (const auto& item : DefaultMenuMapping)
          setDefaultAction(stick, item, event, EventMode::kMenuMode, updateDefaults);
        break;

      default:
        break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::setDefaultMapping(Event::Type event, EventMode mode)
{
  eraseMapping(event, mode);
  for (auto& i : mySticks)
    setStickDefaultMapping(i.first, event, mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::defineControllerMappings(const Controller::Type type, Controller::Jack port)
{
  // determine controller events to use
  switch(type)
  {
    case Controller::Type::Keyboard:
    case Controller::Type::KidVid:
      if(port == Controller::Jack::Left)
        myLeftMode = EventMode::kKeypadMode;
      else
        myRightMode = EventMode::kKeypadMode;
      break;

    case Controller::Type::Paddles:
    case Controller::Type::PaddlesIAxDr:
    case Controller::Type::PaddlesIAxis:
      if(port == Controller::Jack::Left)
        myLeftMode = EventMode::kPaddlesMode;
      else
        myRightMode = EventMode::kPaddlesMode;
      break;

    case Controller::Type::CompuMate:
      myLeftMode = myRightMode = EventMode::kCompuMateMode;
      break;

    default:
      // let's use joystick then
      if(port == Controller::Jack::Left)
        myLeftMode = EventMode::kJoystickMode;
      else
        myRightMode = EventMode::kJoystickMode;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::enableEmulationMappings()
{
  for (auto& stick : mySticks)
  {
    const PhysicalJoystickPtr j = stick.second;

    // start from scratch and enable common mappings
    j->joyMap.eraseMode(EventMode::kEmulationMode);
  }

  enableCommonMappings();

  // enable right mode first, so that in case of mapping clashes the left controller has preference
  switch (myRightMode)
  {
    case EventMode::kPaddlesMode:
      enableMappings(RightPaddlesEvents, EventMode::kPaddlesMode);
      break;

    case EventMode::kKeypadMode:
      enableMappings(RightKeypadEvents, EventMode::kKeypadMode);
      break;

    default:
      enableMappings(RightJoystickEvents, EventMode::kJoystickMode);
      break;
  }

  switch (myLeftMode)
  {
    case EventMode::kPaddlesMode:
      enableMappings(LeftPaddlesEvents, EventMode::kPaddlesMode);
      break;

    case EventMode::kKeypadMode:
      enableMappings(LeftKeypadEvents, EventMode::kKeypadMode);
      break;

    default:
      enableMappings(LeftJoystickEvents, EventMode::kJoystickMode);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::enableCommonMappings()
{
  for (int i = Event::NoType + 1; i < Event::LastType; i++)
  {
    Event::Type event = static_cast<Event::Type>(i);

    if(isCommonEvent(event))
      enableMapping(event, EventMode::kCommonMode);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::enableMappings(const Event::EventSet& events, EventMode mode)
{
  for (const auto& event : events)
    enableMapping(event, mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::enableMapping(const Event::Type event, EventMode mode)
{
  // copy from controller mode into emulation mode
  for (auto& stick : mySticks)
  {
    const PhysicalJoystickPtr j = stick.second;

    JoyMap::JoyMappingArray joyMappings = j->joyMap.getEventMapping(event, mode);

    for (const auto& mapping : joyMappings)
      j->joyMap.add(event, EventMode::kEmulationMode, mapping.button,
                    mapping.axis, mapping.adir, mapping.hat, mapping.hdir);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventMode PhysicalJoystickHandler::getEventMode(const Event::Type event, const EventMode mode) const
{
  if(mode == EventMode::kEmulationMode)
  {
    if(isJoystickEvent(event))
      return EventMode::kJoystickMode;

    if(isPaddleEvent(event))
      return EventMode::kPaddlesMode;

    if(isKeypadEvent(event))
      return EventMode::kKeypadMode;

    if(isCommonEvent(event))
      return EventMode::kCommonMode;
  }

  return mode;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::isJoystickEvent(const Event::Type event) const
{
  return LeftJoystickEvents.find(event) != LeftJoystickEvents.end()
    || RightJoystickEvents.find(event) != RightJoystickEvents.end();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::isPaddleEvent(const Event::Type event) const
{
  return LeftPaddlesEvents.find(event) != LeftPaddlesEvents.end()
    || RightPaddlesEvents.find(event) != RightPaddlesEvents.end();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::isKeypadEvent(const Event::Type event) const
{
  return LeftKeypadEvents.find(event) != LeftKeypadEvents.end()
    || RightKeypadEvents.find(event) != RightKeypadEvents.end();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::isCommonEvent(const Event::Type event) const
{
  return !(isJoystickEvent(event) || isPaddleEvent(event) || isKeypadEvent(event));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::eraseMapping(Event::Type event, EventMode mode)
{
  // If event is 'NoType', erase and reset all mappings
  // Otherwise, only reset the given event
  if(event == Event::NoType)
  {
    for (auto& stick : mySticks)
    {
      stick.second->eraseMap(mode);          // erase all events
      if(mode == EventMode::kEmulationMode)
      {
        stick.second->eraseMap(EventMode::kCommonMode);
        stick.second->eraseMap(EventMode::kJoystickMode);
        stick.second->eraseMap(EventMode::kPaddlesMode);
        stick.second->eraseMap(EventMode::kKeypadMode);
      }
    }
  }
  else
  {
    for (auto& stick : mySticks)
    {
      stick.second->eraseEvent(event, mode); // only reset the specific event
      stick.second->eraseEvent(event, getEventMode(event, mode));
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::saveMapping()
{
  // Save the joystick mapping hash table, making sure to update it with
  // any changes that have been made during the program run
  ostringstream joybuf;

  for(const auto& i: myDatabase)
  {
    const string& map = i.second.joy ? i.second.joy->getMap() : i.second.mapping;
    if(map != "")
      joybuf << CTRL_DELIM << map;
  }
  myOSystem.settings().setValue("joymap", joybuf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string PhysicalJoystickHandler::getMappingDesc(Event::Type event, EventMode mode) const
{
  ostringstream buf;
  EventMode evMode = getEventMode(event, mode);

  for(const auto& s: mySticks)
  {
    uInt32 stick = s.first;
    const PhysicalJoystickPtr j = s.second;

    if(j)
    {
      //Joystick mapping / labeling
      if(j->joyMap.getEventMapping(event, evMode).size())
      {
        if(buf.str() != "")
          buf << ", ";
        buf << j->joyMap.getEventMappingDesc(stick, event, evMode);
      }
    }
  }
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::addJoyMapping(Event::Type event, EventMode mode, int stick,
                                            int button, JoyAxis axis, JoyDir adir)
{
  const PhysicalJoystickPtr j = joy(stick);

  if(j && event < Event::LastType &&
      button >= JOY_CTRL_NONE && button < j->numButtons &&
      axis >= JoyAxis::NONE && int(axis) < j->numAxes)
  {
    EventMode evMode = getEventMode(event, mode);


    // This confusing code is because each axis has two associated values,
    // but analog events only affect one of the axis.
    if (Event::isAnalog(event))
      adir = JoyDir::ANALOG;

    // avoid double mapping in common and controller modes
    if (evMode == EventMode::kCommonMode)
    {
      // erase identical mappings for all controller modes
      j->joyMap.erase(EventMode::kJoystickMode, button, axis, adir);
      j->joyMap.erase(EventMode::kPaddlesMode, button, axis, adir);
      j->joyMap.erase(EventMode::kKeypadMode, button, axis, adir);
      j->joyMap.erase(EventMode::kCompuMateMode, button, axis, adir);
    }
    else if (evMode != EventMode::kMenuMode)
    {
      // erase identical mapping for kCommonMode
      j->joyMap.erase(EventMode::kCommonMode, button, axis, adir);
    }

    j->joyMap.add(event, evMode, button, axis, adir);
    // update running emulation mapping too
    j->joyMap.add(event, EventMode::kEmulationMode, button, axis, adir);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::addJoyHatMapping(Event::Type event, EventMode mode, int stick,
                                               int button, int hat, JoyHatDir hdir)
{
  const PhysicalJoystickPtr j = joy(stick);

  if(j && event < Event::LastType &&
     button >= JOY_CTRL_NONE && button < j->numButtons &&
     hat >= 0 && hat < j->numHats && hdir != JoyHatDir::CENTER)
  {
    EventMode evMode = getEventMode(event, mode);

    // avoid double mapping in common and controller modes
    if (evMode == EventMode::kCommonMode)
    {
      // erase identical mappings for all controller modes
      j->joyMap.erase(EventMode::kJoystickMode, button, hat, hdir);
      j->joyMap.erase(EventMode::kPaddlesMode, button, hat, hdir);
      j->joyMap.erase(EventMode::kKeypadMode, button, hat, hdir);
      j->joyMap.erase(EventMode::kCompuMateMode, button, hat, hdir);
    }
    else if (evMode != EventMode::kMenuMode)
    {
      // erase identical mapping for kCommonMode
      j->joyMap.erase(EventMode::kCommonMode, button, hat, hdir);
    }

    j->joyMap.add(event, evMode, button, hat, hdir);
    // update running emulation mapping too
    j->joyMap.add(event, EventMode::kEmulationMode, button, hat, hdir);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::handleAxisEvent(int stick, int axis, int value)
{
  const PhysicalJoystickPtr j = joy(stick);

  if(j)
  {
    int button = j->buttonLast[stick];

    if(myHandler.state() == EventHandlerState::EMULATION)
    {
      Event::Type eventAxisAnalog;

      // Check for analog events, which are handled differently
      // A value change lower than ~90% indicates analog input
      if((abs(j->axisLastValue[axis] - value) < 30000)
         && (eventAxisAnalog = j->joyMap.get(EventMode::kEmulationMode, button, JoyAxis(axis), JoyDir::ANALOG)) != Event::Type::NoType)
      {
        myHandler.handleEvent(eventAxisAnalog, value);
      }
      else
      {
        // Otherwise, we assume the event is digital
        // Every axis event has two associated values, negative and positive
        Event::Type eventAxisNeg = j->joyMap.get(EventMode::kEmulationMode, button, JoyAxis(axis), JoyDir::NEG);
        Event::Type eventAxisPos = j->joyMap.get(EventMode::kEmulationMode, button, JoyAxis(axis), JoyDir::POS);

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
      }
      j->axisLastValue[axis] = value;
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
        myHandler.overlay().handleJoyAxisEvent(stick, JoyAxis(axis), convertAxisValue(value), button);
#endif
        j->axisLastValue[axis] = value;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::handleBtnEvent(int stick, int button, bool pressed)
{
  const PhysicalJoystickPtr j = joy(stick);

  if(j)
  {
    j->buttonLast[stick] = pressed ? button : JOY_CTRL_NONE;

    // Handle buttons which switch eventhandler state
    if(!pressed && myHandler.changeStateByEvent(j->joyMap.get(EventMode::kEmulationMode, button)))
      return;

    // Determine which mode we're in, then send the event to the appropriate place
    if(myHandler.state() == EventHandlerState::EMULATION)
      myHandler.handleEvent(j->joyMap.get(EventMode::kEmulationMode, button), pressed);
#ifdef GUI_SUPPORT
    else if(myHandler.hasOverlay())
      myHandler.overlay().handleJoyBtnEvent(stick, button, pressed);
#endif
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::handleHatEvent(int stick, int hat, int value)
{
  // Preprocess all hat events, converting to Stella JoyHatDir type
  // Generate multiple equivalent hat events representing combined direction
  // when we get a diagonal hat event

  const PhysicalJoystickPtr j = joy(stick);

  if(j)
  {
    const int button = j->buttonLast[stick];

    if(myHandler.state() == EventHandlerState::EMULATION)
    {
      myHandler.handleEvent(j->joyMap.get(EventMode::kEmulationMode, button, hat, JoyHatDir::UP),
                            value & EVENT_HATUP_M);
      myHandler.handleEvent(j->joyMap.get(EventMode::kEmulationMode, button, hat, JoyHatDir::RIGHT),
                            value & EVENT_HATRIGHT_M);
      myHandler.handleEvent(j->joyMap.get(EventMode::kEmulationMode, button, hat, JoyHatDir::DOWN),
                            value & EVENT_HATDOWN_M);
      myHandler.handleEvent(j->joyMap.get(EventMode::kEmulationMode, button, hat, JoyHatDir::LEFT),
                            value & EVENT_HATLEFT_M);
    }
#ifdef GUI_SUPPORT
    else if(myHandler.hasOverlay())
    {
      if(value == EVENT_HATCENTER_M)
        myHandler.overlay().handleJoyHatEvent(stick, hat, JoyHatDir::CENTER, button);
      else
      {
        if(value & EVENT_HATUP_M)
          myHandler.overlay().handleJoyHatEvent(stick, hat, JoyHatDir::UP, button);
        if(value & EVENT_HATRIGHT_M)
          myHandler.overlay().handleJoyHatEvent(stick, hat, JoyHatDir::RIGHT, button);
        if(value & EVENT_HATDOWN_M)
          myHandler.overlay().handleJoyHatEvent(stick, hat, JoyHatDir::DOWN, button);
        if(value & EVENT_HATLEFT_M)
          myHandler.overlay().handleJoyHatEvent(stick, hat, JoyHatDir::LEFT, button);
      }
    }
#endif
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
PhysicalJoystickHandler::EventMappingArray PhysicalJoystickHandler::DefaultLeftJoystickMapping = {
  // Left joystick (assume buttons zero..two)
  {Event::JoystickZeroFire,   0},
  {Event::JoystickZeroFire5,  1},
  {Event::JoystickZeroFire9,  2},
  // Left joystick left/right directions
  {Event::JoystickZeroLeft,   JOY_CTRL_NONE, JoyAxis::X, JoyDir::NEG},
  {Event::JoystickZeroRight,  JOY_CTRL_NONE, JoyAxis::X, JoyDir::POS},
  // Left joystick up/down directions
  {Event::JoystickZeroUp,     JOY_CTRL_NONE, JoyAxis::Y, JoyDir::NEG},
  {Event::JoystickZeroDown,   JOY_CTRL_NONE, JoyAxis::Y, JoyDir::POS},
  // Left joystick left/right directions (assume hat 0)
  {Event::JoystickZeroLeft,   JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::LEFT},
  {Event::JoystickZeroRight,  JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::RIGHT},
  // Left joystick up/down directions (assume hat 0)
  {Event::JoystickZeroUp,     JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::UP},
  {Event::JoystickZeroDown,   JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::DOWN},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray PhysicalJoystickHandler::DefaultRightJoystickMapping = {
  // Right joystick (assume buttons zero..two)
  {Event::JoystickOneFire,    0},
  {Event::JoystickOneFire5,   1},
  {Event::JoystickOneFire9,   2},
  // Right joystick left/right directions
  {Event::JoystickOneLeft,    JOY_CTRL_NONE, JoyAxis::X, JoyDir::NEG},
  {Event::JoystickOneRight,   JOY_CTRL_NONE, JoyAxis::X, JoyDir::POS},
  // Right joystick up/down directions
  {Event::JoystickOneUp,      JOY_CTRL_NONE, JoyAxis::Y, JoyDir::NEG},
  {Event::JoystickOneDown,    JOY_CTRL_NONE, JoyAxis::Y, JoyDir::POS},
  // Right joystick left/right directions (assume hat 0)
  {Event::JoystickOneLeft,    JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::LEFT},
  {Event::JoystickOneRight,   JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::RIGHT},
  // Right joystick up/down directions (assume hat 0)
  {Event::JoystickOneUp,      JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::UP},
  {Event::JoystickOneDown,    JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::DOWN},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray PhysicalJoystickHandler::DefaultLeftPaddlesMapping = {
  {Event::PaddleZeroAnalog,   JOY_CTRL_NONE, JoyAxis::X, JoyDir::ANALOG},
#if defined(RETRON77)
  {Event::PaddleZeroAnalog,   JOY_CTRL_NONE, JoyAxis::Z, JoyDir::ANALOG},
#endif
  // Current code does NOT allow digital and anlog events on the same axis at the same time
  //{Event::PaddleZeroDecrease, JOY_CTRL_NONE, JoyAxis::X, JoyDir::POS},
  //{Event::PaddleZeroIncrease, JOY_CTRL_NONE, JoyAxis::X, JoyDir::NEG},
  {Event::PaddleZeroFire,     0},
  {Event::PaddleOneAnalog,    JOY_CTRL_NONE, JoyAxis::Y, JoyDir::ANALOG},
#if defined(RETRON77)
  {Event::PaddleOneAnalog,    JOY_CTRL_NONE, JoyAxis::A3, JoyDir::ANALOG},
#endif
  // Current code does NOT allow digital and anlog events on the same axis at the same
  //{Event::PaddleOneDecrease,  JOY_CTRL_NONE, JoyAxis::Y, JoyDir::POS},
  //{Event::PaddleOneIncrease,  JOY_CTRL_NONE, JoyAxis::Y, JoyDir::NEG},
  {Event::PaddleOneFire,      1},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray PhysicalJoystickHandler::DefaultRightPaddlesMapping = {
  {Event::PaddleTwoAnalog,    JOY_CTRL_NONE, JoyAxis::X, JoyDir::ANALOG},
#if defined(RETRON77)
  {Event::PaddleTwoAnalog,    JOY_CTRL_NONE, JoyAxis::Z, JoyDir::ANALOG},
#endif
  // Current code does NOT allow digital and anlog events on the same axis at the same
  //{Event::PaddleTwoDecrease,  JOY_CTRL_NONE, JoyAxis::X, JoyDir::POS},
  //{Event::PaddleTwoIncrease,  JOY_CTRL_NONE, JoyAxis::X, JoyDir::NEG},
  {Event::PaddleTwoFire,      0},
  {Event::PaddleThreeAnalog,  JOY_CTRL_NONE, JoyAxis::Y, JoyDir::ANALOG},
#if defined(RETRON77)
  {Event::PaddleThreeAnalog,  JOY_CTRL_NONE, JoyAxis::A3, JoyDir::ANALOG},
#endif
  // Current code does NOT allow digital and anlog events on the same axis at the same
  //{Event::PaddleThreeDecrease,JOY_CTRL_NONE, JoyAxis::Y, JoyDir::POS},
  //{Event::PaddleThreeIncrease,JOY_CTRL_NONE, JoyAxis::Y, JoyDir::NEG},
  {Event::PaddleThreeFire,    1},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray PhysicalJoystickHandler::DefaultLeftKeypadMapping = {
  {Event::KeyboardZero1,      0},
  {Event::KeyboardZero2,      1},
  {Event::KeyboardZero3,      2},
  {Event::KeyboardZero4,      3},
  {Event::KeyboardZero5,      4},
  {Event::KeyboardZero6,      5},
  {Event::KeyboardZero7,      6},
  {Event::KeyboardZero8,      7},
  {Event::KeyboardZero9,      8},
  {Event::KeyboardZeroStar,   9},
  {Event::KeyboardZero0,      10},
  {Event::KeyboardZeroPound,  11},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray PhysicalJoystickHandler::DefaultRightKeypadMapping = {
  {Event::KeyboardOne1,       0},
  {Event::KeyboardOne2,       1},
  {Event::KeyboardOne3,       2},
  {Event::KeyboardOne4,       3},
  {Event::KeyboardOne5,       4},
  {Event::KeyboardOne6,       5},
  {Event::KeyboardOne7,       6},
  {Event::KeyboardOne8,       7},
  {Event::KeyboardOne9,       8},
  {Event::KeyboardOneStar,    9},
  {Event::KeyboardOne0,       10},
  {Event::KeyboardOnePound,   11},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray PhysicalJoystickHandler::DefaultCommonMapping = {
  // valid for all joysticks
//#if defined(RETRON77)
  {Event::CmdMenuMode,        3}, // Note: buttons 0..2 are used by controllers!
  {Event::ExitMode,           4},
  {Event::OptionsMenuMode,    5},
  {Event::RewindPause,        6},
  {Event::ConsoleSelect,      7},
  {Event::ConsoleReset,       8},
//#endif
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray PhysicalJoystickHandler::DefaultMenuMapping = {
  // valid for all joysticks
  {Event::UISelect,           0},
  {Event::UIOK,               1},
  {Event::UITabNext,          2},
  {Event::UITabPrev,          3},
  {Event::UICancel,           5},

  {Event::UINavPrev,          JOY_CTRL_NONE, JoyAxis::X, JoyDir::NEG},
  {Event::UITabPrev,          0,             JoyAxis::X, JoyDir::NEG},
  {Event::UINavNext,          JOY_CTRL_NONE, JoyAxis::X, JoyDir::POS},
  {Event::UITabNext,          0,             JoyAxis::X, JoyDir::POS},
  {Event::UIUp,               JOY_CTRL_NONE, JoyAxis::Y, JoyDir::NEG},
  {Event::UIDown,             JOY_CTRL_NONE, JoyAxis::Y, JoyDir::POS},

  {Event::UINavPrev,          JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::LEFT},
  {Event::UINavNext,          JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::RIGHT},
  {Event::UIUp,               JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::UP},
  {Event::UIDown,             JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::DOWN},
};

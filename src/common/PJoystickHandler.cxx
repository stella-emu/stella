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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Logger.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "Joystick.hxx"
#include "Paddles.hxx"
#include "PointingDevice.hxx"
#include "Driving.hxx"
#include "Settings.hxx"
#include "EventHandler.hxx"
#include "PJoystickHandler.hxx"
#include "Logger.hxx"

#ifdef GUI_SUPPORT
  #include "DialogContainer.hxx"
#endif

using json = nlohmann::json;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::PhysicalJoystickHandler(
      OSystem& system, EventHandler& handler, Event& event)
  : myOSystem{system},
    myHandler{handler},
    myEvent(event)
{
  if(myOSystem.settings().getInt("event_ver") != Event::VERSION) {
    Logger::info("event version mismatch; dropping previous joystick mappings");

    return;
  }

  json mappings;
  const string& serializedMapping = myOSystem.settings().getString("joymap");

  try {
    mappings = json::parse(serializedMapping);
  } catch (const json::exception&) {
    Logger::info("converting legacy joystick mappings");

    mappings = convertLegacyMapping(serializedMapping);
  }

  for (const json& mapping: mappings) {
    if (!mapping.contains("name")) {
      Logger::error("ignoring bad joystick mapping");
      continue;
    }

    myDatabase.emplace(mapping.at("name").get<string>(), StickInfo(mapping));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
json PhysicalJoystickHandler::convertLegacyMapping(const string& mapping)
{
  constexpr char CTRL_DELIM = '^';

  istringstream buf(mapping);
  string joymap, joyname;

  getline(buf, joymap, CTRL_DELIM); // event list size, ignore

  json convertedMapping = json::array();

  while(getline(buf, joymap, CTRL_DELIM))
  {
    istringstream namebuf(joymap);
    getline(namebuf, joyname, PhysicalJoystick::MODE_DELIM);

    convertedMapping.push_back(PhysicalJoystick::convertLegacyMapping(joymap, joyname));
  }

  return convertedMapping;
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
    for(const auto& [_name, _info]: myDatabase)
      if(BSPF::startsWithIgnoreCase(_name, stick->name) && _info.joy)
        ++count;

    if(count > 0)
    {
      ostringstream name;
      name << stick->name << " #" << count+1;
      stick->name = name.str();
    }
    stick->type = PhysicalJoystick::Type::REGULAR;
  }
  // The stick *must* be inserted here, since it may be used below
  mySticks[stick->ID] = stick;

  // Map the stelladaptors we've found according to the specified ports
  // The 'type' is also set there
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
    setStickDefaultMapping(stick->ID, Event::NoType, EventMode::kEmulationMode);
    setStickDefaultMapping(stick->ID, Event::NoType, EventMode::kMenuMode);
  }

  // We're potentially swapping out an input device behind the back of
  // the Event system, so we make sure all Stelladaptor-generated events
  // are reset
  for(int port = 0; port < NUM_PORTS; ++port)
  {
    for(int axis = 0; axis < NUM_SA_AXIS; ++axis)
      myEvent.set(SA_Axis[port][axis], 0);
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
  int saOrder[] = { 1, 2 };

  if(BSPF::equalsIgnoreCase(saport, "rl"))
  {
    saOrder[0] = 2; saOrder[1] = 1;
  }

  for(auto& [_id, _joyptr]: mySticks)
  {
    bool found = false;
    // remove previously added emulated ports
    size_t pos = _joyptr->name.find(" (emulates ");

    if(pos != std::string::npos)
      _joyptr->name.erase(pos);

    if(BSPF::startsWithIgnoreCase(_joyptr->name, "Stelladaptor"))
    {
      if(saOrder[saCount] == 1)
        _joyptr->type = PhysicalJoystick::Type::LEFT_STELLADAPTOR;
      else if(saOrder[saCount] == 2)
        _joyptr->type = PhysicalJoystick::Type::RIGHT_STELLADAPTOR;
      found = true;
    }
    else if(BSPF::startsWithIgnoreCase(_joyptr->name, "2600-daptor"))
    {
      if(saOrder[saCount] == 1)
        _joyptr->type = PhysicalJoystick::Type::LEFT_2600DAPTOR;
      else if(saOrder[saCount] == 2)
        _joyptr->type = PhysicalJoystick::Type::RIGHT_2600DAPTOR;
      found = true;
    }
    if(found)
    {
      if(saOrder[saCount] == 1)
        _joyptr->name += " (emulates left joystick port)";
      else if(saOrder[saCount] == 2)
        _joyptr->name += " (emulates right joystick port)";

      saCount++;
      // always map Stelladaptor/2600-daptor to emulation mode defaults
      setStickDefaultMapping(_joyptr->ID, Event::NoType, EventMode::kEmulationMode);
    }
  }
  myOSystem.settings().setValue("saport", saport);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::hasStelladaptors() const
{
  for(auto& [_id, _joyptr] : mySticks)
  {
    // remove previously added emulated ports
    size_t pos = _joyptr->name.find(" (emulates ");

    if(pos != std::string::npos)
      _joyptr->name.erase(pos);

    if(BSPF::startsWithIgnoreCase(_joyptr->name, "Stelladaptor")
       || BSPF::startsWithIgnoreCase(_joyptr->name, "2600-daptor"))
      return true;
  }
  return false;
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
      {
        // A regular joystick defaults to left or right based on the
        // stick number being even or odd; 'daptor joysticks request a
        // specific port
        const bool useLeftMappings =
            j->type == PhysicalJoystick::Type::REGULAR ? ((stick % 2) == 0) :
            (j->type == PhysicalJoystick::Type::LEFT_STELLADAPTOR ||
             j->type == PhysicalJoystick::Type::LEFT_2600DAPTOR);
        if(useLeftMappings)
        {
          // put all controller events into their own mode's mappings
          for (const auto& item : DefaultLeftJoystickMapping)
            setDefaultAction(stick, item, event, EventMode::kJoystickMode, updateDefaults);
          for (const auto& item : DefaultLeftPaddlesMapping)
            setDefaultAction(stick, item, event, EventMode::kPaddlesMode, updateDefaults);
          for (const auto& item : DefaultLeftKeypadMapping)
            setDefaultAction(stick, item, event, EventMode::kKeypadMode, updateDefaults);
        }
        else
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
      }

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
  for (const auto& [_id, _joyptr]: mySticks)
    setStickDefaultMapping(_id, event, mode);
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
    for (auto& [_id, _joyptr]: mySticks)
    {
      _joyptr->eraseMap(mode);          // erase all events
      if(mode == EventMode::kEmulationMode)
      {
        _joyptr->eraseMap(EventMode::kCommonMode);
        _joyptr->eraseMap(EventMode::kJoystickMode);
        _joyptr->eraseMap(EventMode::kPaddlesMode);
        _joyptr->eraseMap(EventMode::kKeypadMode);
      }
    }
  }
  else
  {
    for (auto& [_id, _joyptr]: mySticks)
    {
      _joyptr->eraseEvent(event, mode); // only reset the specific event
      _joyptr->eraseEvent(event, getEventMode(event, mode));
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::saveMapping()
{
  // Save the joystick mapping hash table, making sure to update it with
  // any changes that have been made during the program run
  json mapping = json::array();

  for(const auto& [_name, _info]: myDatabase)
  {
    json map = _info.joy ? _info.joy->getMap() : _info.mapping;

    if (!map.is_null()) mapping.emplace_back(map);
  }

  myOSystem.settings().setValue("joymap", mapping.dump(2));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string PhysicalJoystickHandler::getMappingDesc(Event::Type event, EventMode mode) const
{
  ostringstream buf;
  EventMode evMode = getEventMode(event, mode);

  for(const auto& [_id, _joyptr]: mySticks)
  {
    if(_joyptr)
    {
      //Joystick mapping / labeling
      if(_joyptr->joyMap.getEventMapping(event, evMode).size())
      {
        if(buf.str() != "")
          buf << ", ";
        buf << _joyptr->joyMap.getEventMappingDesc(_id, event, evMode);
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
    //int button = j->buttonLast[stick];

    switch(j->type)
    {
      // Since the various controller classes deal with Stelladaptor
      // devices differently, we send the raw X and Y axis data directly,
      // and let the controller handle it
      // These events don't have to pass through handleEvent, since
      // they can never be remapped
      case PhysicalJoystick::Type::LEFT_STELLADAPTOR:
      case PhysicalJoystick::Type::LEFT_2600DAPTOR:
        if(myOSystem.hasConsole()
           && myOSystem.console().leftController().type() == Controller::Type::Driving)
        {
          if(axis < NUM_SA_AXIS)
            myEvent.set(SA_Axis[0][axis], value);
        }
        else
          handleRegularAxisEvent(j, stick, axis, value);
        break;  // axis on left controller (0)

      case PhysicalJoystick::Type::RIGHT_STELLADAPTOR:
      case PhysicalJoystick::Type::RIGHT_2600DAPTOR:
        if(myOSystem.hasConsole()
           && myOSystem.console().rightController().type() == Controller::Type::Driving)
        {
          if(axis < NUM_SA_AXIS)
            myEvent.set(SA_Axis[1][axis], value);
        }
        else
          handleRegularAxisEvent(j, stick, axis, value);
        break;  // axis on right controller (1)

      default: // PhysicalJoystick::Type::REGULAR
        handleRegularAxisEvent(j, stick, axis, value);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::handleRegularAxisEvent(const PhysicalJoystickPtr j,
                                                     int stick, int axis, int value)
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
#ifdef GUI_SUPPORT
  else if(myHandler.hasOverlay())
  {
    // A value change lower than Joystick::deadzone indicates analog input which is ignored
    if((abs(j->axisLastValue[axis] - value) > Joystick::deadzone()))
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
        myHandler.overlay().handleJoyAxisEvent(stick, JoyAxis(axis), convertAxisValue(value), button);

      }
    }
    j->axisLastValue[axis] = value;
  }
#endif
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
  for(const auto& [_name, _info]: myDatabase)
    VarList::push_back(db, _name, _info.joy ? _info.joy->ID : -1);

  return db;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream& operator<<(ostream& os, const PhysicalJoystickHandler& jh)
{
  os << "---------------------------------------------------------" << endl
     << "joy database:"  << endl;
  for(const auto& [_name, _info]: jh.myDatabase)
    os << _name << endl << _info << endl << endl;

  os << "---------------------" << endl
     << "joy active:"  << endl;
  for(const auto& [_id, _joyptr]: jh.mySticks)
    os << _id << ": " << *_joyptr << endl;
  os << "---------------------------------------------------------"
     << endl << endl << endl;

  return os;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::changeDeadzone(int direction)
{
  int deadzone = BSPF::clamp(myOSystem.settings().getInt("joydeadzone") + direction,
                             Joystick::DEAD_ZONE_MIN, Joystick::DEAD_ZONE_MAX);
  myOSystem.settings().setValue("joydeadzone", deadzone);

  Joystick::setDeadZone(deadzone);

  int value = Joystick::deadZoneValue(deadzone);

  myOSystem.frameBuffer().showGaugeMessage("Joystick deadzone", std::to_string(value),
                                           value, 3200, 32200);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::changeAnalogPaddleSensitivity(int direction)
{
  int sense = BSPF::clamp(myOSystem.settings().getInt("psense") + direction,
                          Paddles::MIN_ANALOG_SENSE, Paddles::MAX_ANALOG_SENSE);
  myOSystem.settings().setValue("psense", sense);

  Paddles::setAnalogSensitivity(sense);

  ostringstream ss;
  ss << std::round(Paddles::analogSensitivityValue(sense) * 100.F) << "%";
  myOSystem.frameBuffer().showGaugeMessage("Analog paddle sensitivity", ss.str(), sense,
                                           Paddles::MIN_ANALOG_SENSE, Paddles::MAX_ANALOG_SENSE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::changePaddleDejitterAveraging(int direction)
{
  int dejitter = BSPF::clamp(myOSystem.settings().getInt("dejitter.base") + direction,
                             Paddles::MIN_DEJITTER, Paddles::MAX_DEJITTER);
  myOSystem.settings().setValue("dejitter.base", dejitter);

  Paddles::setDejitterBase(dejitter);

  ostringstream ss;
  if(dejitter)
    ss << dejitter;
  else
    ss << "Off";

  myOSystem.frameBuffer().showGaugeMessage("Analog paddle dejitter averaging",
                                           ss.str(), dejitter,
                                           Paddles::MIN_DEJITTER, Paddles::MAX_DEJITTER);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::changePaddleDejitterReaction(int direction)
{
  int dejitter = BSPF::clamp(myOSystem.settings().getInt("dejitter.diff") + direction,
                             Paddles::MIN_DEJITTER, Paddles::MAX_DEJITTER);
  myOSystem.settings().setValue("dejitter.diff", dejitter);

  Paddles::setDejitterDiff(dejitter);

  ostringstream ss;
  if(dejitter)
    ss << dejitter;
  else
    ss << "Off";

  myOSystem.frameBuffer().showGaugeMessage("Analog paddle dejitter reaction",
                                           ss.str(), dejitter,
                                           Paddles::MIN_DEJITTER, Paddles::MAX_DEJITTER);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::changeDigitalPaddleSensitivity(int direction)
{
  int sense = BSPF::clamp(myOSystem.settings().getInt("dsense") + direction,
                          Paddles::MIN_DIGITAL_SENSE, Paddles::MAX_DIGITAL_SENSE);
  myOSystem.settings().setValue("dsense", sense);

  Paddles::setDigitalSensitivity(sense);

  ostringstream ss;
  if(sense)
    ss << sense * 10 << "%";
  else
    ss << "Off";

  myOSystem.frameBuffer().showGaugeMessage("Digital sensitivity",
                                           ss.str(), sense,
                                           Paddles::MIN_DIGITAL_SENSE, Paddles::MAX_DIGITAL_SENSE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::changeMousePaddleSensitivity(int direction)
{
  int sense = BSPF::clamp(myOSystem.settings().getInt("msense") + direction,
                          Paddles::MIN_MOUSE_SENSE, Paddles::MAX_MOUSE_SENSE);
  myOSystem.settings().setValue("msense", sense);

  Paddles::setMouseSensitivity(sense);

  ostringstream ss;
  ss << sense * 10 << "%";

  myOSystem.frameBuffer().showGaugeMessage("Mouse paddle sensitivity",
                                           ss.str(), sense,
                                           Paddles::MIN_MOUSE_SENSE, Paddles::MAX_MOUSE_SENSE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::changeMouseTrackballSensitivity(int direction)
{
  int sense = BSPF::clamp(myOSystem.settings().getInt("tsense") + direction,
                          PointingDevice::MIN_SENSE, PointingDevice::MAX_SENSE);
  myOSystem.settings().setValue("tsense", sense);

  PointingDevice::setSensitivity(sense);

  ostringstream ss;
  ss << sense * 10 << "%";

  myOSystem.frameBuffer().showGaugeMessage("Mouse trackball sensitivity",
                                           ss.str(), sense,
                                           PointingDevice::MIN_SENSE, PointingDevice::MAX_SENSE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::changeDrivingSensitivity(int direction)
{
  int sense = BSPF::clamp(myOSystem.settings().getInt("dcsense") + direction,
                          Driving::MIN_SENSE, Driving::MAX_SENSE);
  myOSystem.settings().setValue("dcsense", sense);

  Driving::setSensitivity(sense);

  ostringstream ss;
  ss << sense * 10 << "%";

  myOSystem.frameBuffer().showGaugeMessage("Driving controller sensitivity",
                                           ss.str(), sense,
                                           Driving::MIN_SENSE, Driving::MAX_SENSE);
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
PhysicalJoystickHandler::EventMappingArray
PhysicalJoystickHandler::DefaultCommonMapping = {
  // valid for all joysticks
#if defined(RETRON77)
  {Event::CmdMenuMode,        3}, // Note: buttons 0..2 are used by controllers!
  {Event::ExitMode,           4},
  {Event::OptionsMenuMode,    5},
  {Event::RewindPause,        6},
  {Event::ConsoleSelect,      7},
  {Event::ConsoleReset,       8},
#endif
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray
PhysicalJoystickHandler::DefaultMenuMapping = {
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used by the Stelladaptor to send absolute axis values
const Event::Type PhysicalJoystickHandler::SA_Axis[NUM_PORTS][NUM_SA_AXIS] = {
  { Event::SALeftAxis0Value,  Event::SALeftAxis1Value  },
  { Event::SARightAxis0Value, Event::SARightAxis1Value }
};

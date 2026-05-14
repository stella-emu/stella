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

#include "OSystem.hxx"
#include "Console.hxx"
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
    myEvent{event}
{
  if(myOSystem.settings().getInt("event_ver") != Event::VERSION) {
    Logger::info("event version mismatch; dropping previous joystick mappings");

    return;
  }

  json mappings;
  const string_view serializedMapping = myOSystem.settings().getString("joymap");

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
json PhysicalJoystickHandler::convertLegacyMapping(string_view mapping)
{
  constexpr char CTRL_DELIM = '^';

  json convertedMapping = json::array();

  // Skip the first segment (event list size)
  auto pos = mapping.find(CTRL_DELIM);
  if(pos == string_view::npos)
    return convertedMapping;

  mapping = mapping.substr(pos + 1);

  while(!mapping.empty())
  {
    pos = mapping.find(CTRL_DELIM);
    const string_view joymap = mapping.substr(0, pos);

    const auto nameEnd = joymap.find(PhysicalJoystick::MODE_DELIM);
    const string_view joyname = joymap.substr(0, nameEnd);

    convertedMapping.push_back(
      PhysicalJoystick::convertLegacyMapping(joymap, joyname));

    if(pos == string_view::npos) break;
    mapping = mapping.substr(pos + 1);
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
  bool isAdaptor = false;

  if(BSPF::containsIgnoreCase(stick->name, "Stelladaptor")
     || BSPF::containsIgnoreCase(stick->name, "2600-daptor"))
  {
    isAdaptor = true;
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
      stick->name = std::format("{} #{}", stick->name, count + 1);

    stick->type = PhysicalJoystick::Type::REGULAR;
  }
  // The stick *must* be inserted here, since it may be used below
  mySticks[stick->ID] = stick;

  bool erased = false;
  if(isAdaptor)
  {
    // Map the Stelladaptors we've found according to the specified ports
    // The 'type' is also set there
    erased = mapStelladaptors(myOSystem.settings().getString("saport"), stick->ID);

  }
  if(erased)
    // We have to add all Stelladaptors again, because they have changed
    // name due to being reordered when mapping them
    for(const auto& [_id, _stick] : mySticks)
    {
      if(_stick->name.find(" (emulates ") != std::string::npos)
        addToDatabase(_stick);
    }
  else
    addToDatabase(stick);

  // We're potentially swapping out an input device behind the back of
  // the Event system, so we make sure all Stelladaptor-generated events
  // are reset
  for(const auto& axisList : SA_Axis)
    for(const auto axis : axisList)
      myEvent.set(axis, 0);

  return stick->ID;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::addToDatabase(const PhysicalJoystickPtr& stick)
{
  // Add stick to database
  const auto it = myDatabase.find(stick->name);
  if(it != myDatabase.end()) // already present
  {
    it->second.joy = stick;
    stick->setMap(it->second.mapping);
    enableEmulationMappings();
  }
  else // adding for the first time
  {
    const StickInfo info("", stick);
    myDatabase.emplace(stick->name, info);
    setStickDefaultMapping(stick->ID, Event::NoType, EventMode::kMenuMode);
    setStickDefaultMapping(stick->ID, Event::NoType, EventMode::kEmulationMode);
  }

  Logger::info(std::format("Added joystick {}:\n  {}\n",
                           stick->ID, stick->about()));
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
    const PhysicalJoystickPtr stick = mySticks.at(id);

    const auto it = myDatabase.find(stick->name);
    if(it != myDatabase.end() && it->second.joy == stick)
    {
      Logger::info(std::format("Removed joystick {}:\n  {}\n",
        mySticks[id]->ID, mySticks[id]->about()));

      // Remove joystick, but remember mapping
      it->second.mapping = stick->getMap();
      it->second.joy = nullptr;
      mySticks.erase(id);

      return true;
    }
  }
  catch(const std::out_of_range&)
  {
    return false;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::remove(string_view name)
{
  const auto it = myDatabase.find(name);
  if(it != myDatabase.end() && it->second.joy == nullptr)
  {
    myDatabase.erase(it);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::setPort(string_view name, PhysicalJoystick::Port port)
{
  const auto it = myDatabase.find(name);
  if(it != myDatabase.end() && it->second.joy != nullptr)
  {
    it->second.joy->setPort(port);
    // TODO: update mappings
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::mapStelladaptors(string_view saport, int ID)
{
  bool erased = false;
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

  for(const auto& [_id, _stick]: mySticks)
  {
    bool found = false;
    const size_t pos = _stick->name.find(" (emulates ");

    if(pos != std::string::npos && ID != -1 && ID < _stick->ID)
    {
      // Erase a previously added Stelladapter with a higher ID
      Logger::info(std::format("Erased joystick {}:\n  {}\n",
                               _stick->ID, _stick->about()));

      _stick->name.erase(pos);
      erased = true;
    }

    if(BSPF::containsIgnoreCase(_stick->name, "Stelladaptor"))
    {
      if(saOrder[saCount] == 1)
        _stick->type = PhysicalJoystick::Type::LEFT_STELLADAPTOR;
      else if(saOrder[saCount] == 2)
        _stick->type = PhysicalJoystick::Type::RIGHT_STELLADAPTOR;
      found = true;
    }
    else if(BSPF::containsIgnoreCase(_stick->name, "2600-daptor"))
    {
      if(saOrder[saCount] == 1)
        _stick->type = PhysicalJoystick::Type::LEFT_2600DAPTOR;
      else if(saOrder[saCount] == 2)
        _stick->type = PhysicalJoystick::Type::RIGHT_2600DAPTOR;
      found = true;
    }
    if(found)
    {
      if(saOrder[saCount] == 1)
        _stick->name += " (emulates left joystick port)";
      else if(saOrder[saCount] == 2)
        _stick->name += " (emulates right joystick port)";

      saCount++;
      // always map Stelladaptor/2600-daptor to emulation mode defaults
      setStickDefaultMapping(_stick->ID, Event::NoType, EventMode::kEmulationMode);
    }
  }
  myOSystem.settings().setValue("saport", saport);
  return erased;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::hasStelladaptors() const
{
  // Note: this has side effect of changing joystick name; removes part of
  //       name that starts with " (emulates " to end of string
  return std::ranges::any_of(mySticks, [](const auto& stick) {
    const auto& [_id, _joyptr] = stick;

    // remove previously added emulated ports
    const size_t pos = _joyptr->name.find(" (emulates ");

    if(pos != std::string::npos)
      _joyptr->name.erase(pos);

    return BSPF::containsIgnoreCase(_joyptr->name, "Stelladaptor") ||
           BSPF::containsIgnoreCase(_joyptr->name, "2600-daptor");
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Depending on parameters, this method does the following:
// 1. update all events with default (event == Event::NoType, updateDefault == true)
// 2. reset all events to default    (event == Event::NoType, updateDefault == false)
// 3. reset one event to default     (event != Event::NoType)
void PhysicalJoystickHandler::setDefaultAction(int stick,
    EventMapping map, Event::Type event, EventMode mode, bool updateDefaults)
{
  const PhysicalJoystickPtr j = joy(stick);

  // If event is 'NoType', erase and reset all mappings
  // Otherwise, only reset the given event
  const bool eraseAll = !updateDefaults && (event == Event::NoType);

  if(updateDefaults)
  {
    // if there is no existing mapping for the event and
    //  the default mapping for the event is unused, set default key for event
    if(j->joyMap.getEventMapping(map.event, mode).empty() &&
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
void PhysicalJoystickHandler::applyDefaultActions(int stick, EventMappingSpan mappings,
  Event::Type event, EventMode mode, bool updateDefaults)
{
  for(const auto& item : mappings)
    setDefaultAction(stick, item, event, mode, updateDefaults);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::setStickDefaultMapping(
    int stick, Event::Type event, EventMode mode, bool updateDefaults)
{
  const PhysicalJoystickPtr j = joy(stick);

  if(j)
  {
    switch(mode)
    {
      case EventMode::kEmulationMode:
      {
        // A regular joystick defaults to left or right based on
        // the defined port or stick number being even or odd;
        // 'daptor' joysticks request a specific port
        bool useLeftMappings = true;
        if(j->type == PhysicalJoystick::Type::REGULAR)
        {
          useLeftMappings = j->port == PhysicalJoystick::Port::LEFT
            || (j->port == PhysicalJoystick::Port::AUTO && (stick % 2) == 0);
        }
        else
          useLeftMappings =
            j->type == PhysicalJoystick::Type::LEFT_STELLADAPTOR ||
            j->type == PhysicalJoystick::Type::LEFT_2600DAPTOR;

        if(useLeftMappings)
        {
          // put all controller events into their own mode's mappings
          applyDefaultActions(stick, DefaultLeftJoystickMapping, event,
                              EventMode::kJoystickMode, updateDefaults);
          applyDefaultActions(stick, DefaultLeftKeyboardMapping, event,
                              EventMode::kKeyboardMode, updateDefaults);
          applyDefaultActions(stick, DefaultLeftDrivingMapping, event,
                              EventMode::kDrivingMode,  updateDefaults);
        }
        else
        {
          // put all controller events into their own mode's mappings
          applyDefaultActions(stick, DefaultRightJoystickMapping, event,
                              EventMode::kJoystickMode, updateDefaults);
          applyDefaultActions(stick, DefaultRightKeyboardMapping, event,
                              EventMode::kKeyboardMode, updateDefaults);
          applyDefaultActions(stick, DefaultRightDrivingMapping, event,
                              EventMode::kDrivingMode,  updateDefaults);
        }

        // Regular joysticks can only be used by one player at a time,
        // so we need to separate the paddles onto different
        // devices. The Stelladaptor and 2600-daptor controllers support
        // two players each when used as paddles, so are different.
        const int paddlesPerJoystick = (j->type == PhysicalJoystick::Type::REGULAR) ? 1 : 2;

        if(paddlesPerJoystick == 2)
        {
          const EventMappingSpan paddleMappings = useLeftMappings
            ? EventMappingSpan{DefaultLeftPaddlesMapping}
            : EventMappingSpan{DefaultRightPaddlesMapping};
          applyDefaultActions(stick, paddleMappings, event, EventMode::kPaddlesMode, updateDefaults);
        }
        else
        {
          // One paddle per joystick means we need different defaults,
          // such that each player gets the same mapping.

          // This mapping is:
          // - stick 0: left paddle A
          // - stick 1: left paddle B
          // - stick 2: right paddle A
          // - stick 3: right paddle B
          const bool useLeftPaddleMappings = (stick % 4) < 2;
          const bool useAPaddleMappings = (stick % 2) == 0;

          const EventMappingSpan paddleMappings =
            useLeftPaddleMappings
              ? (useAPaddleMappings
                  ? EventMappingSpan{DefaultLeftAPaddlesMapping}
                  : EventMappingSpan{DefaultLeftBPaddlesMapping})
              : (useAPaddleMappings
                  ? EventMappingSpan{DefaultRightAPaddlesMapping}
                  : EventMappingSpan{DefaultRightBPaddlesMapping});

          applyDefaultActions(stick, paddleMappings, event,
                              EventMode::kPaddlesMode, updateDefaults);
        }

        applyDefaultActions(stick, DefaultCommonMapping, event,
                            EventMode::kCommonMode, updateDefaults);
        // update running emulation mapping too
        enableEmulationMappings();
        break;
      }

      case EventMode::kMenuMode:
        applyDefaultActions(stick, DefaultMenuMapping, event,
                            EventMode::kMenuMode, updateDefaults);
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
void PhysicalJoystickHandler::defineControllerMappings(
  Controller::Type type, Controller::Jack port, const Properties& properties,
  Controller::Type qtType1, Controller::Type qtType2)
{
  // Determine controller events to use
  if(type == Controller::Type::QuadTari)
  {
    if(port == Controller::Jack::Left)
    {
      myLeftMode = getMode(qtType1);
      myLeft2ndMode = getMode(qtType2);
    }
    else
    {
      myRightMode = getMode(qtType1);
      myRight2ndMode = getMode(qtType2);
    }
  }
  else
  {
    const EventMode mode = getMode(type);

    if(port == Controller::Jack::Left)
      myLeftMode = mode;
    else
      myRightMode = mode;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventMode PhysicalJoystickHandler::getMode(const Properties& properties,
                                           PropType propType)
{
  const string& propName = properties.get(propType);

  if(!propName.empty())
    return getMode(Controller::getType(propName));

  return EventMode::kJoystickMode;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventMode PhysicalJoystickHandler::getMode(Controller::Type type)
{
  switch(type)
  {
    using enum Controller::Type;
    case Keyboard:
    case KidVid:
      return EventMode::kKeyboardMode;

    case Paddles:
    case PaddlesIAxDr:
    case PaddlesIAxis:
      return EventMode::kPaddlesMode;

    case CompuMate:
      return EventMode::kCompuMateMode;

    case Driving:
      return EventMode::kDrivingMode;

    default:
      // let's use joystick then
      return EventMode::kJoystickMode;
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

  // Process in increasing priority order, so that in case of mapping clashes
  //  the higher priority controller has preference
  switch(myRight2ndMode)
  {
    case EventMode::kPaddlesMode:
      enableMappings(QTPaddles4Events, EventMode::kPaddlesMode);
      break;

    case EventMode::kEmulationMode: // no QuadTari
      break;

    default:
      enableMappings(QTJoystick4Events, EventMode::kJoystickMode);
      break;
  }

  switch(myLeft2ndMode)
  {
    case EventMode::kPaddlesMode:
      enableMappings(QTPaddles3Events, EventMode::kPaddlesMode);
      break;

    case EventMode::kEmulationMode: // no QuadTari
      break;

    default:
      enableMappings(QTJoystick3Events, EventMode::kJoystickMode);
      break;
  }

  // enable right mode first, so that in case of mapping clashes the left controller has preference
  switch(myRightMode)
  {
    case EventMode::kPaddlesMode:
      enableMappings(RightPaddlesEvents, EventMode::kPaddlesMode);
      break;

    case EventMode::kKeyboardMode:
      enableMappings(RightKeyboardEvents, EventMode::kKeyboardMode);
      break;

    case EventMode::kDrivingMode:
      enableMappings(RightDrivingEvents, EventMode::kDrivingMode);
      break;

    default:
      enableMappings(RightJoystickEvents, EventMode::kJoystickMode);
      break;
  }

  switch(myLeftMode)
  {
    case EventMode::kPaddlesMode:
      enableMappings(LeftPaddlesEvents, EventMode::kPaddlesMode);
      break;

    case EventMode::kKeyboardMode:
      enableMappings(LeftKeyboardEvents, EventMode::kKeyboardMode);
      break;

    case EventMode::kDrivingMode:
      enableMappings(LeftDrivingEvents, EventMode::kDrivingMode);
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
    const auto event = static_cast<Event::Type>(i);

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
void PhysicalJoystickHandler::enableMapping(Event::Type event, EventMode mode)
{
  // copy from controller mode into emulation mode
  for (auto& stick : mySticks)
  {
    const PhysicalJoystickPtr j = stick.second;

    const JoyMap::JoyMappingArray joyMappings = j->joyMap.getEventMapping(event, mode);

    for (const auto& mapping : joyMappings)
      j->joyMap.add(event, EventMode::kEmulationMode, mapping.button,
                    mapping.axis, mapping.adir, mapping.hat, mapping.hdir);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventMode PhysicalJoystickHandler::getEventMode(Event::Type event, EventMode mode)
{
  if(mode == EventMode::kEmulationMode)
  {
    if(isJoystickEvent(event))
      return EventMode::kJoystickMode;

    if(isPaddleEvent(event))
      return EventMode::kPaddlesMode;

    if(isKeyboardEvent(event))
      return EventMode::kKeyboardMode;

    if(isDrivingEvent(event))
      return EventMode::kDrivingMode;

    if(isCommonEvent(event))
      return EventMode::kCommonMode;
  }

  return mode;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::isJoystickEvent(Event::Type event)
{
  return LeftJoystickEvents.contains(event)
    || QTJoystick3Events.contains(event)
    || RightJoystickEvents.contains(event)
    || QTJoystick4Events.contains(event);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::isPaddleEvent(Event::Type event)
{
  return LeftPaddlesEvents.contains(event)
    || QTPaddles3Events.contains(event)
    || RightPaddlesEvents.contains(event)
    || QTPaddles4Events.contains(event);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::isKeyboardEvent(Event::Type event)
{
  return LeftKeyboardEvents.contains(event)
    || RightKeyboardEvents.contains(event);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::isDrivingEvent(Event::Type event)
{
  return LeftDrivingEvents.contains(event)
    || RightDrivingEvents.contains(event);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::isCommonEvent(Event::Type event)
{
  return !(isJoystickEvent(event) || isPaddleEvent(event)
    || isKeyboardEvent(event) || isDrivingEvent(event));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::eraseMapping(Event::Type event, EventMode mode)
{
  // If event is 'NoType', erase and reset all mappings
  // Otherwise, only reset the given event
  if(event == Event::NoType)
  {
    for (const auto& [_id, _joyptr]: mySticks)
    {
      _joyptr->eraseMap(mode);          // erase all events
      if(mode == EventMode::kEmulationMode)
      {
        _joyptr->eraseMap(EventMode::kCommonMode);
        _joyptr->eraseMap(EventMode::kJoystickMode);
        _joyptr->eraseMap(EventMode::kPaddlesMode);
        _joyptr->eraseMap(EventMode::kDrivingMode);
        _joyptr->eraseMap(EventMode::kKeyboardMode);
      }
    }
  }
  else
  {
    for (const auto& [_id, _joyptr]: mySticks)
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
    const json map = _info.joy ? _info.joy->getMap() : _info.mapping;

    if (!map.is_null()) mapping.emplace_back(map);
  }

  myOSystem.settings().setValue("joymap", mapping.dump(2));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string PhysicalJoystickHandler::getMappingDesc(Event::Type event, EventMode mode) const
{
  const EventMode evMode = getEventMode(event, mode);
  string desc;
  desc.reserve(100);

  for(const auto& [_id, _joyptr]: mySticks)
  {
    if(_joyptr && !_joyptr->joyMap.getEventMapping(event, evMode).empty())
    {
      if(!desc.empty())
        desc += ", ";
      desc += _joyptr->joyMap.getEventMappingDesc(_id, event, evMode);
    }
  }
  return desc;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystickHandler::addJoyMapping(Event::Type event, EventMode mode,
    int stick, int button, JoyAxis axis, JoyDir adir)
{
  const PhysicalJoystickPtr j = joy(stick);

  if(j && event < Event::LastType &&
      (button == JOY_CTRL_NONE || (button >= 0 && button < j->numButtons)) &&
      (axis == JoyAxis::NONE || static_cast<int>(axis) < j->numAxes))
  {
    const EventMode evMode = getEventMode(event, mode);

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
      //j->joyMap.erase(EventMode::kKeyboardMode, button, axis, adir); // no common buttons in keyboard mode!
      j->joyMap.erase(EventMode::kCompuMateMode, button, axis, adir);
      j->joyMap.erase(EventMode::kDrivingMode, button, axis, adir);
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
bool PhysicalJoystickHandler::addJoyHatMapping(Event::Type event, EventMode mode,
    int stick, int button, int hat, JoyHatDir hdir)
{
  const PhysicalJoystickPtr j = joy(stick);

  if(j && event < Event::LastType &&
     (button == JOY_CTRL_NONE || (button >= 0 && button < j->numButtons)) &&
     hat >= 0 && hat < j->numHats && hdir != JoyHatDir::CENTER)
  {
    const EventMode evMode = getEventMode(event, mode);

    // avoid double mapping in common and controller modes
    if (evMode == EventMode::kCommonMode)
    {
      // erase identical mappings for all controller modes
      j->joyMap.erase(EventMode::kJoystickMode, button, hat, hdir);
      j->joyMap.erase(EventMode::kPaddlesMode, button, hat, hdir);
      j->joyMap.erase(EventMode::kKeyboardMode, button, hat, hdir);
      j->joyMap.erase(EventMode::kDrivingMode, button, hat, hdir);
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
  const PhysicalJoystickPtr& j = joy(stick);

  if(j)
  {
    switch(j->type)
    {
      using enum PhysicalJoystick::Type;
      // Since the various controller classes deal with Stelladaptor
      // devices differently, we send the raw X and Y axis data directly,
      // and let the controller handle it
      // These events don't have to pass through handleEvent, since
      // they can never be remapped
      case LEFT_STELLADAPTOR:
      case LEFT_2600DAPTOR:
      case RIGHT_STELLADAPTOR:
      case RIGHT_2600DAPTOR:
      {
        const bool isLeft = (j->type == LEFT_STELLADAPTOR ||
                             j->type == LEFT_2600DAPTOR);
        const size_t port = isLeft ? 0 : 1;
        const auto& ctrl = isLeft ? myOSystem.console().leftController()
                                  : myOSystem.console().rightController();
        if(myOSystem.hasConsole() && ctrl.type() == Controller::Type::Driving)
        {
          if(std::cmp_less(axis, SA_Axis[port].size()))
            myEvent.set(SA_Axis[port][axis], value);
        }
        else
          handleRegularAxisEvent(j, stick, axis, value);
        break;
      }
      default:  // PhysicalJoystick::Type::REGULAR
        handleRegularAxisEvent(j, stick, axis, value);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::handleRegularAxisEvent(const PhysicalJoystickPtr& j,
                                                     int stick, int axis, int value)
{
  const int button = j->buttonLast;

  if(myHandler.state() == EventHandlerState::EMULATION)
  {
    // Check for analog events, which are handled differently
    // A value change lower than ~90% indicates analog input
    const Event::Type eventAxisAnalog =
      (abs(j->axisLastValue[axis] - value) < 30000)
        ? j->joyMap.get(EventMode::kEmulationMode, button,
            static_cast<JoyAxis>(axis), JoyDir::ANALOG)
        : Event::Type::NoType;

    if(eventAxisAnalog != Event::Type::NoType)
      myHandler.handleEvent(eventAxisAnalog, value);
    else
    {
      // Otherwise, we assume the event is digital
      // Every axis event has two associated values, negative and positive
      const Event::Type eventAxisNeg = j->joyMap.get(EventMode::kEmulationMode, button,
                                                     static_cast<JoyAxis>(axis), JoyDir::NEG);
      const Event::Type eventAxisPos = j->joyMap.get(EventMode::kEmulationMode, button,
                                                     static_cast<JoyAxis>(axis), JoyDir::POS);

      if(value > Controller::digitalDeadZone())
        myHandler.handleEvent(eventAxisPos);
      else if(value < -Controller::digitalDeadZone())
        myHandler.handleEvent(eventAxisNeg);
      else
      {
        // Treat any dead zone value as zero
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
    }
    j->axisLastValue[axis] = value;
  }
#ifdef GUI_SUPPORT
  else if(myHandler.hasOverlay())
  {
    // A value change lower than Controller::digitalDeadzone indicates analog input which is ignored
    if((abs(j->axisLastValue[axis] - value) > Controller::digitalDeadZone()))
    {
      // First, clamp the values to simulate digital input
      // (the only thing that the underlying code understands)
      if(value > Controller::digitalDeadZone())
        value = 32000;
      else if(value < -Controller::digitalDeadZone())
        value = -32000;
      else
        value = 0;

      // Now filter out consecutive, similar values
      // (only pass on the event if the state has changed)
      if(value != j->axisLastValue[axis])
        myHandler.overlay().handleJoyAxisEvent(stick, static_cast<JoyAxis>(axis),
                                               convertAxisValue(value), button);
    }
    else
      myHandler.overlay().handleJoyAxisEvent(stick, static_cast<JoyAxis>(axis), JoyDir::NONE, button);
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
    j->buttonLast = pressed ? button : JOY_CTRL_NONE;

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
void PhysicalJoystickHandler::handleHatEvent(int stick, int hat, JoyHatMask value)
{
  // Preprocess all hat events, converting to Stella JoyHatDir type
  // Generate multiple equivalent hat events representing combined direction
  // when we get a diagonal hat event

  const PhysicalJoystickPtr j = joy(stick);

  if(j)
  {
    const int button = j->buttonLast;
    const Bitmask::Enum hat_value{value};

    if(myHandler.state() == EventHandlerState::EMULATION)
    {
      myHandler.handleEvent(j->joyMap.get(EventMode::kEmulationMode,
                                          button, hat, JoyHatDir::UP),
                                          hat_value.any_of(JoyHatMask::UP));
      myHandler.handleEvent(j->joyMap.get(EventMode::kEmulationMode,
                                          button, hat, JoyHatDir::RIGHT),
                                          hat_value.any_of(JoyHatMask::RIGHT));
      myHandler.handleEvent(j->joyMap.get(EventMode::kEmulationMode,
                                          button, hat, JoyHatDir::DOWN),
                                          hat_value.any_of(JoyHatMask::DOWN));
      myHandler.handleEvent(j->joyMap.get(EventMode::kEmulationMode,
                                          button, hat, JoyHatDir::LEFT),
                                          hat_value.any_of(JoyHatMask::LEFT));
    }
#ifdef GUI_SUPPORT
    else if(myHandler.hasOverlay())
    {
      if(value == JoyHatMask::CENTER)
        myHandler.overlay().handleJoyHatEvent(stick, hat, JoyHatDir::CENTER, button);
      else
      {
        if(hat_value.any_of(JoyHatMask::UP))
          myHandler.overlay().handleJoyHatEvent(stick, hat, JoyHatDir::UP, button);
        if(hat_value.any_of(JoyHatMask::RIGHT))
          myHandler.overlay().handleJoyHatEvent(stick, hat, JoyHatDir::RIGHT, button);
        if(hat_value.any_of(JoyHatMask::DOWN))
          myHandler.overlay().handleJoyHatEvent(stick, hat, JoyHatDir::DOWN, button);
        if(hat_value.any_of(JoyHatMask::LEFT))
          myHandler.overlay().handleJoyHatEvent(stick, hat, JoyHatDir::LEFT, button);
      }
    }
#endif
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::MinStickInfoList PhysicalJoystickHandler::minStickList() const
{
  MinStickInfoList list;
  list.reserve(myDatabase.size());

  std::ranges::transform(myDatabase, std::back_inserter(list),
    [](const auto& entry) {
      const auto& [_name, _info] = entry;
      return MinStickInfo(_name,
        _info.joy ? _info.joy->ID : -1,
        _info.joy ? _info.joy->port : PhysicalJoystick::Port::AUTO);
    });
  return list;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::ostream& operator<<(std::ostream& os, const PhysicalJoystickHandler& jh)
{
  os << "---------------------------------------------------------\n"
     << "joy database:\n";
  for(const auto& [_name, _info]: jh.myDatabase)
    os << _name << '\n' << _info << "\n\n";

  os << "---------------------\n"
     << "joy active:\n";
  for(const auto& [_id, _joyptr]: jh.mySticks)
    os << _id << ": " << *_joyptr << '\n';
  os << "---------------------------------------------------------"
     << "\n\n\n";

  return os;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::changeDigitalDeadZone(int direction)
{
  const int deadZone =
    BSPF::clamp(myOSystem.settings().getInt("joydeadzone") + direction,
                Controller::MIN_DIGITAL_DEADZONE, Controller::MAX_DIGITAL_DEADZONE);
  myOSystem.settings().setValue("joydeadzone", deadZone);

  Controller::setDigitalDeadZone(deadZone);

  myOSystem.frameBuffer().showGaugeMessage(
    "Digital controller dead zone",
    std::format("{}%", static_cast<int>(
      std::round(Controller::digitalDeadZoneValue(deadZone) * 100.F / 32768))),
    deadZone,
    Controller::MIN_DIGITAL_DEADZONE, Controller::MAX_DIGITAL_DEADZONE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::changeAnalogPaddleDeadZone(int direction)
{
  const int deadZone =
    BSPF::clamp(myOSystem.settings().getInt("adeadzone") + direction,
                Controller::MIN_ANALOG_DEADZONE, Controller::MAX_ANALOG_DEADZONE);
  myOSystem.settings().setValue("adeadzone", deadZone);

  Controller::setAnalogDeadZone(deadZone);

  myOSystem.frameBuffer().showGaugeMessage(
    "Analog controller dead zone",
    std::format("{}%", static_cast<int>(
      std::round(Controller::analogDeadZoneValue(deadZone) * 100.F / 32768))),
    deadZone,
    Controller::MIN_ANALOG_DEADZONE, Controller::MAX_ANALOG_DEADZONE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::changeAnalogPaddleSensitivity(int direction)
{
  const int sense =
    BSPF::clamp(myOSystem.settings().getInt("psense") + direction,
                Paddles::MIN_ANALOG_SENSE, Paddles::MAX_ANALOG_SENSE);
  myOSystem.settings().setValue("psense", sense);

  Paddles::setAnalogSensitivity(sense);

  myOSystem.frameBuffer().showGaugeMessage(
    "Analog paddle sensitivity",
    std::format("{}%", static_cast<int>(
      std::round(Paddles::analogSensitivityValue(sense) * 100.F))),
    sense,
    Paddles::MIN_ANALOG_SENSE, Paddles::MAX_ANALOG_SENSE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::changeAnalogPaddleLinearity(int direction)
{
  const int linear =
    BSPF::clamp(myOSystem.settings().getInt("plinear") + direction * 5,
                Paddles::MIN_ANALOG_LINEARITY, Paddles::MAX_ANALOG_LINEARITY);
  myOSystem.settings().setValue("plinear", linear);

  Paddles::setAnalogLinearity(linear);

  myOSystem.frameBuffer().showGaugeMessage(
    "Analog paddle linearity",
    linear ? std::format("{}%", linear) : "Off",
    linear,
    Paddles::MIN_ANALOG_LINEARITY, Paddles::MAX_ANALOG_LINEARITY);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::changePaddleDejitterAveraging(int direction)
{
  const int dejitter =
    BSPF::clamp(myOSystem.settings().getInt("dejitter.base") + direction,
                Paddles::MIN_DEJITTER, Paddles::MAX_DEJITTER);
  myOSystem.settings().setValue("dejitter.base", dejitter);

  Paddles::setDejitterBase(dejitter);

  myOSystem.frameBuffer().showGaugeMessage(
    "Analog paddle dejitter averaging",
    dejitter ? std::to_string(dejitter) : "Off",
    dejitter,
    Paddles::MIN_DEJITTER, Paddles::MAX_DEJITTER);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::changePaddleDejitterReaction(int direction)
{
  const int dejitter =
    BSPF::clamp(myOSystem.settings().getInt("dejitter.diff") + direction,
                Paddles::MIN_DEJITTER, Paddles::MAX_DEJITTER);
  myOSystem.settings().setValue("dejitter.diff", dejitter);

  Paddles::setDejitterDiff(dejitter);

  myOSystem.frameBuffer().showGaugeMessage(
    "Analog paddle dejitter reaction",
    dejitter ? std::to_string(dejitter) : "Off",
    dejitter,
    Paddles::MIN_DEJITTER, Paddles::MAX_DEJITTER);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::changeDigitalPaddleSensitivity(int direction)
{
  const int sense =
    BSPF::clamp(myOSystem.settings().getInt("dsense") + direction,
                Paddles::MIN_DIGITAL_SENSE, Paddles::MAX_DIGITAL_SENSE);
  myOSystem.settings().setValue("dsense", sense);

  Paddles::setDigitalSensitivity(sense);

  myOSystem.frameBuffer().showGaugeMessage(
    "Digital sensitivity",
    sense ? std::format("{}%", sense * 10) : "Off",
    sense,
    Paddles::MIN_DIGITAL_SENSE, Paddles::MAX_DIGITAL_SENSE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::changeMousePaddleSensitivity(int direction)
{
  const int sense =
    BSPF::clamp(myOSystem.settings().getInt("msense") + direction,
                Controller::MIN_MOUSE_SENSE, Controller::MAX_MOUSE_SENSE);
  myOSystem.settings().setValue("msense", sense);

  Controller::setMouseSensitivity(sense);

  myOSystem.frameBuffer().showGaugeMessage(
    "Mouse paddle sensitivity",
    std::format("{}%", sense * 10),
    sense,
    Controller::MIN_MOUSE_SENSE, Controller::MAX_MOUSE_SENSE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::changeMouseTrackballSensitivity(int direction)
{
  const int sense =
    BSPF::clamp(myOSystem.settings().getInt("tsense") + direction,
                PointingDevice::MIN_SENSE, PointingDevice::MAX_SENSE);
  myOSystem.settings().setValue("tsense", sense);

  PointingDevice::setSensitivity(sense);

  myOSystem.frameBuffer().showGaugeMessage(
    "Mouse trackball sensitivity",
    std::format("{}%", sense * 10),
    sense,
    PointingDevice::MIN_SENSE, PointingDevice::MAX_SENSE);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystickHandler::changeDrivingSensitivity(int direction)
{
  const int sense =
    BSPF::clamp(myOSystem.settings().getInt("dcsense") + direction,
                Driving::MIN_SENSE, Driving::MAX_SENSE);
  myOSystem.settings().setValue("dcsense", sense);

  Driving::setSensitivity(sense);

  myOSystem.frameBuffer().showGaugeMessage(
    "Driving controller sensitivity",
    std::format("{}%", sense * 10),
    sense,
    Driving::MIN_SENSE, Driving::MAX_SENSE);
}

// NOLINTBEGIN(bugprone-throwing-static-initialization)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray
PhysicalJoystickHandler::DefaultLeftJoystickMapping = {
  // Left joystick (assume buttons zero..two)
  {Event::LeftJoystickFire,   0},
  {Event::LeftJoystickFire5,  1},
  {Event::LeftJoystickFire9,  2},
  // Left joystick left/right directions
  {Event::LeftJoystickLeft,   JOY_CTRL_NONE, JoyAxis::X, JoyDir::NEG},
  {Event::LeftJoystickRight,  JOY_CTRL_NONE, JoyAxis::X, JoyDir::POS},
  // Left joystick up/down directions
  {Event::LeftJoystickUp,     JOY_CTRL_NONE, JoyAxis::Y, JoyDir::NEG},
  {Event::LeftJoystickDown,   JOY_CTRL_NONE, JoyAxis::Y, JoyDir::POS},
  // Left joystick left/right directions (assume hat 0)
  {Event::LeftJoystickLeft,   JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::LEFT},
  {Event::LeftJoystickRight,  JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::RIGHT},
  // Left joystick up/down directions (assume hat 0)
  {Event::LeftJoystickUp,     JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::UP},
  {Event::LeftJoystickDown,   JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::DOWN},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray
PhysicalJoystickHandler::DefaultRightJoystickMapping = {
  // Right joystick (assume buttons zero..two)
  {Event::RightJoystickFire,    0},
  {Event::RightJoystickFire5,   1},
  {Event::RightJoystickFire9,   2},
  // Right joystick left/right directions
  {Event::RightJoystickLeft,    JOY_CTRL_NONE, JoyAxis::X, JoyDir::NEG},
  {Event::RightJoystickRight,   JOY_CTRL_NONE, JoyAxis::X, JoyDir::POS},
  // Right joystick up/down directions
  {Event::RightJoystickUp,      JOY_CTRL_NONE, JoyAxis::Y, JoyDir::NEG},
  {Event::RightJoystickDown,    JOY_CTRL_NONE, JoyAxis::Y, JoyDir::POS},
  // Right joystick left/right directions (assume hat 0)
  {Event::RightJoystickLeft,    JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::LEFT},
  {Event::RightJoystickRight,   JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::RIGHT},
  // Right joystick up/down directions (assume hat 0)
  {Event::RightJoystickUp,      JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::UP},
  {Event::RightJoystickDown,    JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::DOWN},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray
PhysicalJoystickHandler::DefaultLeftPaddlesMapping = {
  {Event::LeftPaddleAAnalog,   JOY_CTRL_NONE, JoyAxis::X, JoyDir::ANALOG},
  // Current code does NOT allow digital and anlog events on the same axis at the same time
  //{Event::LeftPaddleADecrease, JOY_CTRL_NONE, JoyAxis::X, JoyDir::POS},
  //{Event::LeftPaddleAIncrease, JOY_CTRL_NONE, JoyAxis::X, JoyDir::NEG},
  {Event::LeftPaddleAFire,     0},
  {Event::LeftPaddleBAnalog,    JOY_CTRL_NONE, JoyAxis::Y, JoyDir::ANALOG},
  // Current code does NOT allow digital and anlog events on the same axis at the same
  //{Event::LeftPaddleBDecrease,  JOY_CTRL_NONE, JoyAxis::Y, JoyDir::POS},
  //{Event::LeftPaddleBIncrease,  JOY_CTRL_NONE, JoyAxis::Y, JoyDir::NEG},
  {Event::LeftPaddleBFire,      1},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray
PhysicalJoystickHandler::DefaultRightPaddlesMapping = {
  {Event::RightPaddleAAnalog,    JOY_CTRL_NONE, JoyAxis::X, JoyDir::ANALOG},
  // Current code does NOT allow digital and anlog events on the same axis at the same
  //{Event::RightPaddleADecrease,  JOY_CTRL_NONE, JoyAxis::X, JoyDir::POS},
  //{Event::RightPaddleAIncrease,  JOY_CTRL_NONE, JoyAxis::X, JoyDir::NEG},
  {Event::RightPaddleAFire,      0},
  {Event::RightPaddleBAnalog,  JOY_CTRL_NONE, JoyAxis::Y, JoyDir::ANALOG},
  // Current code does NOT allow digital and anlog events on the same axis at the same
  //{Event::RightPaddleBDecrease,JOY_CTRL_NONE, JoyAxis::Y, JoyDir::POS},
  //{Event::RightPaddleBIncrease,JOY_CTRL_NONE, JoyAxis::Y, JoyDir::NEG},
  {Event::RightPaddleBFire,    1},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray
PhysicalJoystickHandler::DefaultLeftAPaddlesMapping = {
  {Event::LeftPaddleAAnalog, JOY_CTRL_NONE, JoyAxis::X, JoyDir::ANALOG},
  // Current code does NOT allow digital and anlog events on the same axis at the same time
  //{Event::LeftPaddleADecrease, JOY_CTRL_NONE, JoyAxis::X, JoyDir::POS},
  //{Event::LeftPaddleAIncrease, JOY_CTRL_NONE, JoyAxis::X, JoyDir::NEG},
  {Event::LeftPaddleAFire,   0},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray
PhysicalJoystickHandler::DefaultLeftBPaddlesMapping = {
  {Event::LeftPaddleBAnalog, JOY_CTRL_NONE, JoyAxis::X, JoyDir::ANALOG},
  // Current code does NOT allow digital and anlog events on the same axis at the same
  //{Event::LeftPaddleBDecrease,  JOY_CTRL_NONE, JoyAxis::X, JoyDir::POS},
  //{Event::LeftPaddleBIncrease,  JOY_CTRL_NONE, JoyAxis::X, JoyDir::NEG},
  {Event::LeftPaddleBFire,   0},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray
PhysicalJoystickHandler::DefaultRightAPaddlesMapping = {
  {Event::RightPaddleAAnalog, JOY_CTRL_NONE, JoyAxis::X, JoyDir::ANALOG},
  // Current code does NOT allow digital and anlog events on the same axis at the same
  //{Event::RightPaddleADecrease,  JOY_CTRL_NONE, JoyAxis::X, JoyDir::POS},
  //{Event::RightPaddleAIncrease,  JOY_CTRL_NONE, JoyAxis::X, JoyDir::NEG},
  {Event::RightPaddleAFire,   0},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray
PhysicalJoystickHandler::DefaultRightBPaddlesMapping = {
  {Event::RightPaddleBAnalog, JOY_CTRL_NONE, JoyAxis::X, JoyDir::ANALOG},
  // Current code does NOT allow digital and anlog events on the same axis at the same
  //{Event::RightPaddleBDecrease,JOY_CTRL_NONE, JoyAxis::X, JoyDir::POS},
  //{Event::RightPaddleBIncrease,JOY_CTRL_NONE, JoyAxis::X, JoyDir::NEG},
  {Event::RightPaddleBFire,   0},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray
PhysicalJoystickHandler::DefaultLeftKeyboardMapping = {
  {Event::LeftKeyboard1,      0},
  {Event::LeftKeyboard2,      1},
  {Event::LeftKeyboard3,      2},
  {Event::LeftKeyboard4,      3},
  {Event::LeftKeyboard5,      4},
  {Event::LeftKeyboard6,      5},
  {Event::LeftKeyboard7,      6},
  {Event::LeftKeyboard8,      7},
  {Event::LeftKeyboard9,      8},
  {Event::LeftKeyboardStar,   9},
  {Event::LeftKeyboard0,      10},
  {Event::LeftKeyboardPound,  11},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray
PhysicalJoystickHandler::DefaultRightKeyboardMapping = {
  {Event::RightKeyboard1,       0},
  {Event::RightKeyboard2,       1},
  {Event::RightKeyboard3,       2},
  {Event::RightKeyboard4,       3},
  {Event::RightKeyboard5,       4},
  {Event::RightKeyboard6,       5},
  {Event::RightKeyboard7,       6},
  {Event::RightKeyboard8,       7},
  {Event::RightKeyboard9,       8},
  {Event::RightKeyboardStar,    9},
  {Event::RightKeyboard0,       10},
  {Event::RightKeyboardPound,   11},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray
PhysicalJoystickHandler::DefaultLeftDrivingMapping = {
  // Left joystick (assume buttons zero..two)
  {Event::LeftDrivingFire,   0},
  // Left joystick left/right directions
  {Event::LeftDrivingAnalog, JOY_CTRL_NONE, JoyAxis::X, JoyDir::ANALOG},
  // Left joystick left/right directions (assume hat 0)
  {Event::LeftDrivingCCW,    JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::LEFT},
  {Event::LeftDrivingCW,     JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::RIGHT},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray
PhysicalJoystickHandler::DefaultRightDrivingMapping = {
  // Right joystick (assume buttons zero..two)
  {Event::RightDrivingFire,   0},
  // Right joystick left/right directions
  {Event::RightDrivingAnalog, JOY_CTRL_NONE, JoyAxis::X, JoyDir::ANALOG},
  // Right joystick left/right directions (assume hat 0)
  {Event::RightDrivingCCW,    JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::LEFT},
  {Event::RightDrivingCW,     JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::RIGHT},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystickHandler::EventMappingArray
PhysicalJoystickHandler::DefaultCommonMapping = {
  // valid for all joysticks
  // Note: buttons 0..2 are used by controllers!
  {Event::ConsoleSelect,          8}, // Button "Select"
  {Event::ConsoleReset,           9}, // Button "Start"
  {Event::ConsoleColorToggle,     3}, // Button "Y" / "4"
  {Event::ConsoleLeftDiffToggle,  4}, // Left Shoulder Button
  {Event::ConsoleRightDiffToggle, 5}, // Right Shoulder Button
  {Event::CmdMenuMode,            6}, // Left Trigger Button
  {Event::OptionsMenuMode,        7}, // Right Trigger Button
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
  {Event::UIOK,               0            , JoyAxis::Y, JoyDir::NEG},
  {Event::UIDown,             JOY_CTRL_NONE, JoyAxis::Y, JoyDir::POS},
  {Event::UICancel,           0            , JoyAxis::Y, JoyDir::POS},

  {Event::UINavPrev,          JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::LEFT},
  {Event::UINavNext,          JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::RIGHT},
  {Event::UIUp,               JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::UP},
  {Event::UIDown,             JOY_CTRL_NONE, JoyAxis::NONE, JoyDir::NONE, 0, JoyHatDir::DOWN},
};
// NOLINTEND(bugprone-throwing-static-initialization)

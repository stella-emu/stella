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
#include "PropsSet.hxx"
#include "EventHandler.hxx"
#include "PKeyboardHandler.hxx"
#include "json_lib.hxx"

using json = nlohmann::json;

#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif
#ifdef GUI_SUPPORT
  #include "DialogContainer.hxx"
#endif

#if defined(BSPF_MACOS) || defined(MACOS_KEYS)
static constexpr auto MOD3   = StellaMod::GUI;
static constexpr auto CMD    = StellaMod::GUI;
static constexpr auto OPTION = StellaMod::ALT;
#else
static constexpr auto MOD3   = StellaMod::ALT;
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalKeyboardHandler::PhysicalKeyboardHandler(OSystem& system, EventHandler& handler)
  : myOSystem{system},
    myHandler{handler}
{
  const Int32 version = myOSystem.settings().getInt("event_ver");
  bool updateDefaults = false;

  // Compare if event list version has changed so that key maps became invalid
  if (version == Event::VERSION)
  {
    loadSerializedMappings(myOSystem.settings().getString("keymap_emu"), EventMode::kCommonMode);
    loadSerializedMappings(myOSystem.settings().getString("keymap_joy"), EventMode::kJoystickMode);
    loadSerializedMappings(myOSystem.settings().getString("keymap_pad"), EventMode::kPaddlesMode);
    loadSerializedMappings(myOSystem.settings().getString("keymap_drv"), EventMode::kDrivingMode);
    loadSerializedMappings(myOSystem.settings().getString("keymap_key"), EventMode::kKeyboardMode);
    loadSerializedMappings(myOSystem.settings().getString("keymap_ui"), EventMode::kMenuMode);

    updateDefaults = true;
  }

  myKeyMap.enableMod() = myOSystem.settings().getBool("modcombo");

  setDefaultMapping(Event::NoType, EventMode::kEmulationMode, updateDefaults);
  setDefaultMapping(Event::NoType, EventMode::kMenuMode, updateDefaults);
#ifdef GUI_SUPPORT
  setDefaultMapping(Event::NoType, EventMode::kEditMode, updateDefaults);
#endif
#ifdef DEBUGGER_SUPPORT
  setDefaultMapping(Event::NoType, EventMode::kPromptMode, updateDefaults);
#endif
#ifdef DEBUG_BUILD
  verifyDefaultMapping(DefaultCommonMapping, EventMode::kEmulationMode, "EmulationMode");
  verifyDefaultMapping(DefaultMenuMapping, EventMode::kMenuMode, "MenuMode");
#endif
}

#ifdef DEBUG_BUILD
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::verifyDefaultMapping(
  EventMappingSpan mapping, EventMode mode, string_view name)
{
  for(const auto& item1 : mapping)
    for(const auto& item2 : mapping)
      if(item1.event != item2.event && item1.key == item2.key && item1.mod == item2.mod)
        cerr << "ERROR! Duplicate hotkey mapping found: " << name << ", "
          << myKeyMap.getDesc(KeyMap::Mapping(mode, item1.key, item1.mod)) << "\n";
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::loadSerializedMappings(
    string_view serializedMapping, EventMode mode)
{
  json mapping;

  try {
    mapping = json::parse(serializedMapping);
  } catch (const json::exception&) {
    Logger::info("converting legacy keyboard mappings");

    mapping = KeyMap::convertLegacyMapping(serializedMapping);
  }

  try {
    myKeyMap.loadMapping(mapping, mode);
  } catch (const json::exception&) {
    Logger::error("ignoring bad keyboard mappings");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalKeyboardHandler::isMappingUsed(EventMode mode, const EventMapping& map) const
{
  // Menu events can only interfere with
  //   - other menu events
  if(mode == EventMode::kMenuMode)
    return myKeyMap.check(EventMode::kMenuMode, map.key, map.mod);

  // Controller events can interfere with
  //   - other events of the same controller
  //   - and common emulation events
  if(mode != EventMode::kCommonMode)
    return myKeyMap.check(mode, map.key, map.mod)
      || myKeyMap.check(EventMode::kCommonMode, map.key, map.mod);

  // Common emulation events can interfere with
  //   - other common emulation events
  //   - and all controller events
  return myKeyMap.check(EventMode::kCommonMode, map.key, map.mod)
    || myKeyMap.check(EventMode::kJoystickMode, map.key, map.mod)
    || myKeyMap.check(EventMode::kPaddlesMode, map.key, map.mod)
    || myKeyMap.check(EventMode::kKeyboardMode, map.key, map.mod)
    || myKeyMap.check(EventMode::kDrivingMode, map.key, map.mod)
    || myKeyMap.check(EventMode::kCompuMateMode, map.key, map.mod);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Depending on parameters, this method does the following:
// 1. update all events with default (event == Event::NoType, updateDefault == true)
// 2. reset all events to default    (event == Event::NoType, updateDefault == false)
// 3. reset one event to default     (event != Event::NoType)
void PhysicalKeyboardHandler::setDefaultKey(EventMapping map, Event::Type event,
                                            EventMode mode, bool updateDefaults)
{
  // If event is 'NoType', erase and reset all mappings
  // Otherwise, only reset the given event
  const bool eraseAll = !updateDefaults && (event == Event::NoType);

#ifdef GUI_SUPPORT
  // Swap Y and Z for QWERTZ keyboards
  if(mode == EventMode::kEditMode && myHandler.isQwertz())
  {
    if(map.key == StellaKey::Z)
      map.key = StellaKey::Y;
    else if(map.key == StellaKey::Y)
      map.key = StellaKey::Z;
  }
#endif

  if (updateDefaults)
  {
    // if there is no existing mapping for the event and
    //  the default mapping for the event is unused, set default key for event
    if (myKeyMap.getEventMapping(map.event, mode).empty() &&
        !isMappingUsed(mode, map))
    {
      addMapping(map.event, mode, map.key, map.mod);
    }
  }
  else if (eraseAll || map.event == event)
  {
    //myKeyMap.eraseEvent(map.event, mode);
    addMapping(map.event, mode, map.key, map.mod);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Depending on parameters, this method does the following:
// 1. update all events with default (event == Event::NoType, updateDefault == true)
// 2. reset all events to default    (event == Event::NoType, updateDefault == false)
// 3. reset one event to default     (event != Event::NoType)
void PhysicalKeyboardHandler::setDefaultMapping(Event::Type event, EventMode mode,
                                                bool updateDefaults)
{
  if (!updateDefaults)
  {
    myKeyMap.eraseEvent(event, mode);
    myKeyMap.eraseEvent(event, getEventMode(event, mode));
  }

  switch(mode)
  {
    case EventMode::kEmulationMode:
      applyDefaultMappings(DefaultCommonMapping, event,
                           EventMode::kCommonMode, updateDefaults);
      applyDefaultMappings(DefaultJoystickMapping, event,
                           EventMode::kJoystickMode, updateDefaults);
      applyDefaultMappings(DefaultPaddleMapping, event,
                           EventMode::kPaddlesMode, updateDefaults);
      applyDefaultMappings(DefaultKeyboardMapping, event,
                           EventMode::kKeyboardMode, updateDefaults);
      applyDefaultMappings(DefaultDrivingMapping, event,
                           EventMode::kDrivingMode, updateDefaults);
      applyDefaultMappings(CompuMateMapping, event,
                           EventMode::kCompuMateMode, updateDefaults);
      break;

    case EventMode::kMenuMode:
      applyDefaultMappings(DefaultMenuMapping, event,
                           EventMode::kMenuMode, updateDefaults);
      break;

  #ifdef GUI_SUPPORT
    case EventMode::kEditMode:
      // Edit mode events are always set because they are not saved
      applyDefaultMappings(FixedEditMapping, event,
                           EventMode::kEditMode, false);
      break;
  #endif
  #ifdef DEBUGGER_SUPPORT
    case EventMode::kPromptMode:
      // Edit mode events are always set because they are not saved
      applyDefaultMappings(FixedPromptMapping, event,
                           EventMode::kPromptMode, false);
      break;
  #endif

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::defineControllerMappings(
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
EventMode PhysicalKeyboardHandler::getMode(const Properties& properties,
                                           PropType propType)
{
  const string& propName = properties.get(propType);

  if(!propName.empty())
    return getMode(Controller::getType(propName));

  return EventMode::kJoystickMode;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventMode PhysicalKeyboardHandler::getMode(Controller::Type type)
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
void PhysicalKeyboardHandler::enableEmulationMappings()
{
  // start from scratch and enable common mappings
  myKeyMap.eraseMode(EventMode::kEmulationMode);
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

  switch(myRightMode)
  {
    case EventMode::kPaddlesMode:
      enableMappings(RightPaddlesEvents, EventMode::kPaddlesMode);
      break;

    case EventMode::kKeyboardMode:
      enableMappings(RightKeyboardEvents, EventMode::kKeyboardMode);
      break;

    case EventMode::kCompuMateMode:
      // see below
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

    case EventMode::kCompuMateMode:
      for(const auto& item : CompuMateMapping)
        enableMapping(item.event, EventMode::kCompuMateMode);
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
void PhysicalKeyboardHandler::enableCommonMappings()
{
  for (int i = Event::NoType + 1; i < Event::LastType; i++)
  {
    const auto event = static_cast<Event::Type>(i);

    if (isCommonEvent(event))
      enableMapping(event, EventMode::kCommonMode);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::enableMappings(const Event::EventSet& events,
                                             EventMode mode)
{
  for (const auto& event : events)
    enableMapping(event, mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::enableMapping(Event::Type event, EventMode mode)
{
  // copy from controller mode into emulation mode
  const KeyMap::MappingArray mappings = myKeyMap.getEventMapping(event, mode);

  for (const auto& mapping : mappings)
    myKeyMap.add(event, EventMode::kEmulationMode, mapping.key, mapping.mod);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::applyDefaultMappings(EventMappingSpan mappings,
  Event::Type event, EventMode mode, bool updateDefaults)
{
  for (const auto& item : mappings)
    setDefaultKey(item, event, mode, updateDefaults);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventMode PhysicalKeyboardHandler::getEventMode(Event::Type event, EventMode mode)
{
  if (mode == EventMode::kEmulationMode)
  {
    if (isJoystickEvent(event))
      return EventMode::kJoystickMode;

    if (isPaddleEvent(event))
      return EventMode::kPaddlesMode;

    if (isKeyboardEvent(event))
      return EventMode::kKeyboardMode;

    if (isDrivingEvent(event))
      return EventMode::kDrivingMode;

    if (isCommonEvent(event))
      return EventMode::kCommonMode;
  }

  return mode;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalKeyboardHandler::isJoystickEvent(Event::Type event)
{
  return LeftJoystickEvents.contains(event)
    || QTJoystick3Events.contains(event)
    || RightJoystickEvents.contains(event)
    || QTJoystick4Events.contains(event);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalKeyboardHandler::isPaddleEvent(Event::Type event)
{
  return LeftPaddlesEvents.contains(event)
    || QTPaddles3Events.contains(event)
    || RightPaddlesEvents.contains(event)
    || QTPaddles4Events.contains(event);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalKeyboardHandler::isKeyboardEvent(Event::Type event)
{
  return LeftKeyboardEvents.contains(event)
    || RightKeyboardEvents.contains(event);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalKeyboardHandler::isDrivingEvent(Event::Type event)
{
  return LeftDrivingEvents.contains(event)
    || RightDrivingEvents.contains(event);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalKeyboardHandler::isCommonEvent(Event::Type event)
{
  return !(isJoystickEvent(event) || isPaddleEvent(event)
    || isKeyboardEvent(event) || isDrivingEvent(event));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::eraseMapping(Event::Type event, EventMode mode)
{
  myKeyMap.eraseEvent(event, mode);
  myKeyMap.eraseEvent(event, getEventMode(event, mode));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::saveMapping()
{
  myOSystem.settings().setValue("event_ver", Event::VERSION);
  myOSystem.settings().setValue("keymap_emu", myKeyMap.saveMapping(EventMode::kCommonMode).dump(2));
  myOSystem.settings().setValue("keymap_joy", myKeyMap.saveMapping(EventMode::kJoystickMode).dump(2));
  myOSystem.settings().setValue("keymap_pad", myKeyMap.saveMapping(EventMode::kPaddlesMode).dump(2));
  myOSystem.settings().setValue("keymap_drv", myKeyMap.saveMapping(EventMode::kDrivingMode).dump(2));
  myOSystem.settings().setValue("keymap_key", myKeyMap.saveMapping(EventMode::kKeyboardMode).dump(2));
  myOSystem.settings().setValue("keymap_ui", myKeyMap.saveMapping(EventMode::kMenuMode).dump(2));
  enableEmulationMappings();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalKeyboardHandler::addMapping(Event::Type event, EventMode mode,
                                         StellaKey key, StellaMod mod)
{
  // These events cannot be remapped to keys
  if(Event::isAnalog(event))
    return false;
  else
  {
    const EventMode evMode = getEventMode(event, mode);

    // avoid double mapping in common and controller modes
    if (evMode == EventMode::kCommonMode)
    {
      // erase identical mappings for all controller modes
      myKeyMap.erase(EventMode::kJoystickMode, key, mod);
      myKeyMap.erase(EventMode::kPaddlesMode, key, mod);
      myKeyMap.erase(EventMode::kKeyboardMode, key, mod);
      myKeyMap.erase(EventMode::kCompuMateMode, key, mod);
    }
    else if(evMode != EventMode::kMenuMode
            && evMode != EventMode::kEditMode
            && evMode != EventMode::kPromptMode)
    {
      // erase identical mapping for kCommonMode
      myKeyMap.erase(EventMode::kCommonMode, key, mod);
    }

    myKeyMap.add(event, evMode, key, mod);
    if (evMode == myLeftMode || evMode == myRightMode ||
        evMode == myLeft2ndMode || evMode == myRight2ndMode)
      myKeyMap.add(event, mode, key, mod);
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::handleEvent(StellaKey key, StellaMod mod,
                                          bool pressed, bool repeated)
{
#ifdef BSPF_UNIX
  // Swallow StellaKey::TAB under certain conditions
  // See comments on 'myAltKeyCounter' for more information
  if(myAltKeyCounter > 1 && key == StellaKey::TAB)
  {
    myAltKeyCounter = 0;
    return;
  }
  if (key == StellaKey::TAB && pressed && StellaModTest::isAlt(mod))
  {
    // Swallow Alt-Tab, but remember that it happened
    myAltKeyCounter = 1;
    return;
  }
#endif

  const EventHandlerState estate = myHandler.state();

  // special handling for CompuMate in emulation modes
  if ((estate == EventHandlerState::EMULATION || estate == EventHandlerState::PAUSE) &&
      myOSystem.console().leftController().type() == Controller::Type::CompuMate)
  {
    const Event::Type event = myKeyMap.get(EventMode::kCompuMateMode, key, mod);

    // (potential) CompuMate events are handled directly.
    if (myKeyMap.get(EventMode::kEmulationMode, key, mod) != Event::ExitMode &&
      !StellaModTest::isAlt(mod) && event != Event::NoType)
    {
      myHandler.handleEvent(event, pressed, repeated);
      return;
    }
  }

  // Arrange the logic to take advantage of short-circuit evaluation
  // Handle keys which switch eventhandler state
  if (!pressed && myHandler.changeStateByEvent(myKeyMap.get(EventMode::kEmulationMode, key, mod)))
    return;

  // Otherwise, let the event handler deal with it
  switch(estate)
  {
    case EventHandlerState::EMULATION:
    case EventHandlerState::PAUSE:
    case EventHandlerState::PLAYBACK:
      myHandler.handleEvent(myKeyMap.get(EventMode::kEmulationMode, key, mod), pressed, repeated);
      break;

    default:
    #ifdef GUI_SUPPORT
      if (myHandler.hasOverlay())
        myHandler.overlay().handleKeyEvent(key, mod, pressed, repeated);
    #endif
      myHandler.handleEvent(myKeyMap.get(EventMode::kMenuMode, key, mod), pressed, repeated);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::toggleModKeys(bool toggle)
{
  bool modCombo = myOSystem.settings().getBool("modcombo");

  if(toggle)
  {
    modCombo = !modCombo;
    myKeyMap.enableMod() = modCombo;
    myOSystem.settings().setValue("modcombo", modCombo);
  }

  myOSystem.frameBuffer().showTextMessage(
    std::format("Modifier key combos {}", modCombo ? "enabled" : "disabled")
  );
}

// NOLINTBEGIN(bugprone-throwing-static-initialization)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalKeyboardHandler::EventMappingArray
PhysicalKeyboardHandler::DefaultCommonMapping = {
  { Event::ConsoleSelect,            StellaKey::F1 },
  { Event::ConsoleReset,             StellaKey::F2 },
  { Event::ConsoleColor,             StellaKey::F3 },
  { Event::Console7800Pause,         StellaKey::F3, MOD3 },
  { Event::ConsoleLeftDiffA,         StellaKey::F5 },
  { Event::ConsoleRightDiffA,        StellaKey::F7 },
  { Event::SaveState,                StellaKey::F9 },
  { Event::SaveAllStates,            StellaKey::F9, MOD3 },
  { Event::PreviousState,            StellaKey::F10, StellaMod::SHIFT },
  { Event::NextState,                StellaKey::F10 },
  { Event::ToggleAutoSlot,           StellaKey::F10, MOD3 },
  { Event::LoadState,                StellaKey::F11 },
  { Event::LoadAllStates,            StellaKey::F11, MOD3 },
  { Event::TakeSnapshot,             StellaKey::F12 },
  #ifdef BSPF_MACOS
  { Event::TogglePauseMode,          StellaKey::P, StellaMod::SHIFT | MOD3 },
  #else
  { Event::TogglePauseMode,          StellaKey::PAUSE },
  #endif
  { Event::OptionsMenuMode,          StellaKey::TAB },
  { Event::CmdMenuMode,              StellaKey::BACKSLASH },
  { Event::ToggleBezel,              StellaKey::B, StellaMod::CTRL },
  { Event::TimeMachineMode,          StellaKey::T, StellaMod::SHIFT },
  { Event::DebuggerMode,             StellaKey::GRAVE },
  { Event::PlusRomsSetupMode,        StellaKey::P, StellaMod::SHIFT | StellaMod::CTRL | MOD3 },
  { Event::ExitMode,                 StellaKey::ESCAPE },
  #ifdef BSPF_MACOS
  { Event::Quit,                     StellaKey::Q, MOD3 },
  #else
  { Event::Quit,                     StellaKey::Q, StellaMod::CTRL },
  #endif
  { Event::ReloadConsole,            StellaKey::R, StellaMod::CTRL },
  { Event::PreviousMultiCartRom,     StellaKey::R, StellaMod::SHIFT | StellaMod::CTRL },

  { Event::VidmodeDecrease,          StellaKey::MINUS, MOD3 },
  { Event::VidmodeIncrease,          StellaKey::EQUALS, MOD3 },
  { Event::VCenterDecrease,          StellaKey::PAGEUP, MOD3 },
  { Event::VCenterIncrease,          StellaKey::PAGEDOWN, MOD3 },
  { Event::VSizeAdjustDecrease,      StellaKey::PAGEDOWN, StellaMod::SHIFT | MOD3 },
  { Event::VSizeAdjustIncrease,      StellaKey::PAGEUP, StellaMod::SHIFT | MOD3 },
  { Event::ToggleCorrectAspectRatio, StellaKey::C, StellaMod::SHIFT | StellaMod::CTRL },
  { Event::VolumeDecrease,           StellaKey::LEFTBRACKET, MOD3 },
  { Event::VolumeIncrease,           StellaKey::RIGHTBRACKET, MOD3 },
  { Event::SoundToggle,              StellaKey::RIGHTBRACKET, StellaMod::CTRL },

  { Event::ToggleFullScreen,         StellaKey::RETURN, MOD3 },
  { Event::ToggleAdaptRefresh,       StellaKey::R, MOD3 },
  { Event::OverscanDecrease,         StellaKey::PAGEDOWN, StellaMod::SHIFT },
  { Event::OverscanIncrease,         StellaKey::PAGEUP, StellaMod::SHIFT },
  { Event::PreviousVideoMode,        StellaKey::_1, StellaMod::SHIFT | MOD3 },
  { Event::NextVideoMode,            StellaKey::_1, MOD3 },
  { Event::PreviousAttribute,        StellaKey::_2, StellaMod::SHIFT | MOD3 },
  { Event::NextAttribute,            StellaKey::_2, MOD3 },
  { Event::DecreaseAttribute,        StellaKey::_3, StellaMod::SHIFT | MOD3 },
  { Event::IncreaseAttribute,        StellaKey::_3, MOD3 },
  { Event::PhosphorDecrease,         StellaKey::_4, StellaMod::SHIFT | MOD3 },
  { Event::PhosphorIncrease,         StellaKey::_4, MOD3 },
  { Event::TogglePhosphor,           StellaKey::P, MOD3 },
  //{ Event::PhosphorModeDecrease,     StellaKey::P, StellaMod::SHIFT | StellaMod::CTRL | MOD3 },
  { Event::PhosphorModeIncrease,     StellaKey::P, StellaMod::CTRL | MOD3 },
  { Event::ScanlinesDecrease,        StellaKey::_5, StellaMod::SHIFT | MOD3 },
  { Event::ScanlinesIncrease,        StellaKey::_5, MOD3 },
  { Event::PreviousScanlineMask,     StellaKey::_6, StellaMod::SHIFT | MOD3 },
  { Event::NextScanlineMask,         StellaKey::_6, MOD3 },
  { Event::PreviousPaletteAttribute, StellaKey::_9, StellaMod::SHIFT | MOD3 },
  { Event::NextPaletteAttribute,     StellaKey::_9, MOD3 },
  { Event::PaletteAttributeDecrease, StellaKey::_0, StellaMod::SHIFT | MOD3 },
  { Event::PaletteAttributeIncrease, StellaKey::_0, MOD3 },
  { Event::ToggleColorLoss,          StellaKey::L, StellaMod::CTRL },
  { Event::PaletteDecrease,          StellaKey::P, StellaMod::SHIFT | StellaMod::CTRL },
  { Event::PaletteIncrease,          StellaKey::P, StellaMod::CTRL },
  { Event::FormatDecrease,           StellaKey::F, StellaMod::SHIFT | StellaMod::CTRL },
  { Event::FormatIncrease,           StellaKey::F, StellaMod::CTRL },
  #ifndef BSPF_MACOS
  { Event::PreviousSetting,          StellaKey::END },
  { Event::NextSetting,              StellaKey::HOME },
  { Event::PreviousSettingGroup,     StellaKey::END, StellaMod::CTRL },
  { Event::NextSettingGroup,         StellaKey::HOME, StellaMod::CTRL },
  #else
    // HOME & END keys are swapped on Mac keyboards
  { Event::PreviousSetting,          StellaKey::HOME },
  { Event::NextSetting,              StellaKey::END },
  { Event::PreviousSettingGroup,     StellaKey::HOME, StellaMod::CTRL },
  { Event::NextSettingGroup,         StellaKey::END, StellaMod::CTRL },
  #endif
  { Event::PreviousSetting,          StellaKey::KP_1 },
  { Event::NextSetting,              StellaKey::KP_7 },
  { Event::PreviousSettingGroup,     StellaKey::KP_1, StellaMod::CTRL },
  { Event::NextSettingGroup,         StellaKey::KP_7, StellaMod::CTRL },
  { Event::SettingDecrease,          StellaKey::PAGEDOWN },
  { Event::SettingDecrease,          StellaKey::KP_3, StellaMod::CTRL },
  { Event::SettingIncrease,          StellaKey::PAGEUP },
  { Event::SettingIncrease,          StellaKey::KP_9, StellaMod::CTRL },

  { Event::ToggleInter,              StellaKey::I, StellaMod::CTRL },
  { Event::DecreaseSpeed,            StellaKey::S, StellaMod::SHIFT | StellaMod::CTRL },
  { Event::IncreaseSpeed,            StellaKey::S, StellaMod::CTRL },
  { Event::ToggleTurbo,              StellaKey::T, StellaMod::CTRL },
  { Event::JitterSenseDecrease,      StellaKey::J, StellaMod::SHIFT | MOD3 | StellaMod::CTRL },
  { Event::JitterSenseIncrease,      StellaKey::J, MOD3 | StellaMod::CTRL },
  { Event::JitterRecDecrease,        StellaKey::J, StellaMod::SHIFT | StellaMod::CTRL },
  { Event::JitterRecIncrease,        StellaKey::J, StellaMod::CTRL },
  { Event::ToggleDeveloperSet,       StellaKey::D, MOD3 },
  { Event::ToggleJitter,             StellaKey::J, MOD3 },
  { Event::ToggleFrameStats,         StellaKey::L, MOD3 },
  { Event::ToggleTimeMachine,        StellaKey::T, MOD3 },

  #ifdef IMAGE_SUPPORT
  { Event::ToggleContSnapshots,      StellaKey::S, MOD3 | StellaMod::CTRL },
  { Event::ToggleContSnapshotsFrame, StellaKey::S, StellaMod::SHIFT | MOD3 | StellaMod::CTRL },
  #endif

  { Event::DecreaseDeadzone,         StellaKey::F1, StellaMod::CTRL | StellaMod::SHIFT },
  { Event::IncreaseDeadzone,         StellaKey::F1, StellaMod::CTRL },
  { Event::DecAnalogDeadzone,        StellaKey::F1, StellaMod::CTRL | MOD3 | StellaMod::SHIFT},
  { Event::IncAnalogDeadzone,        StellaKey::F1, StellaMod::CTRL | MOD3},
  { Event::DecAnalogSense,           StellaKey::F2, StellaMod::CTRL | StellaMod::SHIFT },
  { Event::IncAnalogSense,           StellaKey::F2, StellaMod::CTRL },
  { Event::DecAnalogLinear,          StellaKey::F2, StellaMod::CTRL | MOD3 | StellaMod::SHIFT},
  { Event::IncAnalogLinear,          StellaKey::F2, StellaMod::CTRL | MOD3},
  { Event::DecDejtterAveraging,      StellaKey::F3, StellaMod::CTRL | StellaMod::SHIFT },
  { Event::IncDejtterAveraging,      StellaKey::F3, StellaMod::CTRL },
  { Event::DecDejtterReaction,       StellaKey::F4, StellaMod::CTRL | StellaMod::SHIFT },
  { Event::IncDejtterReaction,       StellaKey::F4, StellaMod::CTRL },
  { Event::DecDigitalSense,          StellaKey::F5, StellaMod::CTRL | StellaMod::SHIFT },
  { Event::IncDigitalSense,          StellaKey::F5, StellaMod::CTRL },
  { Event::ToggleAutoFire,           StellaKey::A, MOD3 },
  { Event::DecreaseAutoFire,         StellaKey::A, StellaMod::CTRL | StellaMod::SHIFT },
  { Event::IncreaseAutoFire,         StellaKey::A, StellaMod::CTRL },
  { Event::ToggleFourDirections,     StellaKey::F6, StellaMod::CTRL },
  { Event::ToggleKeyCombos,          StellaKey::F7, StellaMod::CTRL },
  { Event::ToggleSAPortOrder,        StellaKey::_1, StellaMod::CTRL },

  { Event::PrevMouseAsController,    StellaKey::F8, StellaMod::CTRL | StellaMod::SHIFT },
  { Event::NextMouseAsController,    StellaKey::F8, StellaMod::CTRL },
  { Event::DecMousePaddleSense,      StellaKey::F9, StellaMod::CTRL | StellaMod::SHIFT },
  { Event::IncMousePaddleSense,      StellaKey::F9, StellaMod::CTRL },
  { Event::DecMouseTrackballSense,   StellaKey::F10, StellaMod::CTRL | StellaMod::SHIFT },
  { Event::IncMouseTrackballSense,   StellaKey::F10, StellaMod::CTRL },
  { Event::DecreaseDrivingSense,     StellaKey::F11, StellaMod::CTRL | StellaMod::SHIFT },
  { Event::IncreaseDrivingSense,     StellaKey::F11, StellaMod::CTRL },
  { Event::PreviousCursorVisbility,  StellaKey::F12, StellaMod::CTRL | StellaMod::SHIFT },
  { Event::NextCursorVisbility,      StellaKey::F12, StellaMod::CTRL },
  { Event::ToggleGrabMouse,          StellaKey::G, StellaMod::CTRL },

  { Event::PreviousLeftPort,         StellaKey::_2, StellaMod::CTRL | StellaMod::SHIFT },
  { Event::NextLeftPort,             StellaKey::_2, StellaMod::CTRL },
  { Event::PreviousRightPort,        StellaKey::_3, StellaMod::CTRL | StellaMod::SHIFT },
  { Event::NextRightPort,            StellaKey::_3, StellaMod::CTRL },
  { Event::ToggleSwapPorts,          StellaKey::_4, StellaMod::CTRL },
  { Event::ToggleSwapPaddles,        StellaKey::_5, StellaMod::CTRL },
  { Event::DecreasePaddleCenterX,    StellaKey::_6, StellaMod::CTRL | StellaMod::SHIFT },
  { Event::IncreasePaddleCenterX,    StellaKey::_6, StellaMod::CTRL },
  { Event::DecreasePaddleCenterY,    StellaKey::_7, StellaMod::CTRL | StellaMod::SHIFT },
  { Event::IncreasePaddleCenterY,    StellaKey::_7, StellaMod::CTRL },
  { Event::PreviousMouseControl,     StellaKey::_0, StellaMod::CTRL | StellaMod::SHIFT },
  { Event::NextMouseControl,         StellaKey::_0, StellaMod::CTRL },
  { Event::DecreaseMouseAxesRange,   StellaKey::_8, StellaMod::CTRL | StellaMod::SHIFT },
  { Event::IncreaseMouseAxesRange,   StellaKey::_8, StellaMod::CTRL },

  { Event::ToggleP0Collision,        StellaKey::Z, StellaMod::SHIFT | MOD3 },
  { Event::ToggleP0Bit,              StellaKey::Z, MOD3 },
  { Event::ToggleP1Collision,        StellaKey::X, StellaMod::SHIFT | MOD3 },
  { Event::ToggleP1Bit,              StellaKey::X, MOD3 },
  { Event::ToggleM0Collision,        StellaKey::C, StellaMod::SHIFT | MOD3 },
  { Event::ToggleM0Bit,              StellaKey::C, MOD3 },
  { Event::ToggleM1Collision,        StellaKey::V, StellaMod::SHIFT | MOD3 },
  { Event::ToggleM1Bit,              StellaKey::V, MOD3 },
  { Event::ToggleBLCollision,        StellaKey::B, StellaMod::SHIFT | MOD3 },
  { Event::ToggleBLBit,              StellaKey::B, MOD3 },
  { Event::TogglePFCollision,        StellaKey::N, StellaMod::SHIFT | MOD3 },
  { Event::TogglePFBit,              StellaKey::N, MOD3 },
  { Event::ToggleCollisions,         StellaKey::COMMA, StellaMod::SHIFT | MOD3 },
  { Event::ToggleBits,               StellaKey::COMMA, MOD3 },
  { Event::ToggleFixedColors,        StellaKey::PERIOD, MOD3 },

  { Event::RewindPause,              StellaKey::LEFT, StellaMod::CTRL | MOD3},
  { Event::Rewind1Menu,              StellaKey::LEFT, MOD3 },
  { Event::Rewind10Menu,             StellaKey::LEFT, StellaMod::SHIFT | MOD3 },
  { Event::RewindAllMenu,            StellaKey::DOWN, MOD3 },
  { Event::UnwindPause,              StellaKey::RIGHT, StellaMod::CTRL | MOD3},
  { Event::Unwind1Menu,              StellaKey::RIGHT, MOD3 },
  { Event::Unwind10Menu,             StellaKey::RIGHT, StellaMod::SHIFT | MOD3 },
  { Event::UnwindAllMenu,            StellaKey::UP, MOD3 },
  { Event::HighScoresMenuMode,       StellaKey::INSERT },
  { Event::TogglePlayBackMode,       StellaKey::SPACE, StellaMod::SHIFT },

  { Event::ConsoleBlackWhite,        StellaKey::F4 },
  { Event::ConsoleLeftDiffB,         StellaKey::F6 },
  { Event::ConsoleRightDiffB,        StellaKey::F8 },
  { Event::Fry,                      StellaKey::BACKSPACE, StellaMod::SHIFT }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalKeyboardHandler::EventMappingArray
PhysicalKeyboardHandler::DefaultMenuMapping = {
  {Event::UIUp,                     StellaKey::UP},
  {Event::UIDown,                   StellaKey::DOWN},
  {Event::UILeft,                   StellaKey::LEFT},
  {Event::UIRight,                  StellaKey::RIGHT},
  {Event::UISelect,                 StellaKey::RETURN},
  {Event::UISelect,                 StellaKey::SPACE},

  {Event::UIHome,                   StellaKey::HOME},
  {Event::UIEnd,                    StellaKey::END},
  {Event::UIPgUp,                   StellaKey::PAGEUP},
  {Event::UIPgDown,                 StellaKey::PAGEDOWN},
  // same with keypad
  {Event::UIUp,                     StellaKey::KP_8},
  {Event::UIDown,                   StellaKey::KP_2},
  {Event::UILeft,                   StellaKey::KP_4},
  {Event::UIRight,                  StellaKey::KP_6},
  {Event::UISelect,                 StellaKey::KP_ENTER},

  {Event::UIHome,                   StellaKey::KP_7},
  {Event::UIEnd,                    StellaKey::KP_1},
  {Event::UIPgUp,                   StellaKey::KP_9},
  {Event::UIPgDown,                 StellaKey::KP_3},

  {Event::UICancel,                 StellaKey::ESCAPE},

  {Event::UINavPrev,                StellaKey::TAB, StellaMod::SHIFT},
  {Event::UINavNext,                StellaKey::TAB},
  {Event::UITabPrev,                StellaKey::TAB, StellaMod::SHIFT | StellaMod::CTRL},
  {Event::UITabNext,                StellaKey::TAB, StellaMod::CTRL},

  {Event::ToggleUIPalette,          StellaKey::T, MOD3},
  {Event::ToggleFullScreen,         StellaKey::RETURN, MOD3},

#ifdef BSPF_MACOS
  {Event::Quit,                     StellaKey::Q, MOD3},
#else
  {Event::Quit,                     StellaKey::Q, StellaMod::CTRL},
#endif

  {Event::UIPrevDir,                StellaKey::BACKSPACE},
#ifdef BSPF_MACOS
  {Event::UIHelp,                   StellaKey::SLASH, StellaMod::SHIFT | CMD},
#else
  {Event::UIHelp,                   StellaKey::F1},
#endif
};

#ifdef GUI_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalKeyboardHandler::EventMappingArray
PhysicalKeyboardHandler::FixedEditMapping = {
  {Event::MoveLeftChar,             StellaKey::LEFT},
  {Event::MoveRightChar,            StellaKey::RIGHT},
  {Event::SelectLeftChar,           StellaKey::LEFT, StellaMod::SHIFT},
  {Event::SelectRightChar,          StellaKey::RIGHT, StellaMod::SHIFT},
#if defined(BSPF_MACOS) || defined(MACOS_KEYS)
  {Event::MoveLeftWord,             StellaKey::LEFT, OPTION},
  {Event::MoveRightWord,            StellaKey::RIGHT, OPTION},
  {Event::MoveHome,                 StellaKey::HOME},
  {Event::MoveHome,                 StellaKey::A, StellaMod::CTRL},
  {Event::MoveHome,                 StellaKey::LEFT, CMD},
  {Event::MoveEnd,                  StellaKey::END},
  {Event::MoveEnd,                  StellaKey::E, StellaMod::CTRL},
  {Event::MoveEnd,                  StellaKey::RIGHT, CMD},
  {Event::SelectLeftWord,           StellaKey::LEFT, StellaMod::SHIFT | OPTION},
  {Event::SelectRightWord,          StellaKey::RIGHT, StellaMod::SHIFT | OPTION},
  {Event::SelectHome,               StellaKey::HOME, StellaMod::SHIFT},
  {Event::SelectHome,               StellaKey::LEFT, StellaMod::SHIFT | CMD},
  {Event::SelectHome,               StellaKey::A, StellaMod::CTRL | StellaMod::SHIFT},
  {Event::SelectEnd,                StellaKey::E, StellaMod::SHIFT | StellaMod::CTRL},
  {Event::SelectEnd,                StellaKey::RIGHT, StellaMod::SHIFT | CMD},
  {Event::SelectEnd,                StellaKey::END, StellaMod::SHIFT},
  {Event::SelectAll,                StellaKey::A, CMD},
  {Event::Delete,                   StellaKey::DELETE},
  {Event::Delete,                   StellaKey::D, StellaMod::CTRL},
  {Event::DeleteLeftWord,           StellaKey::W, StellaMod::CTRL},
  {Event::DeleteLeftWord,           StellaKey::BACKSPACE, OPTION},
  {Event::DeleteRightWord,          StellaKey::DELETE, OPTION},
  {Event::DeleteHome,               StellaKey::U, StellaMod::CTRL},
  {Event::DeleteHome,               StellaKey::BACKSPACE, CMD},
  {Event::DeleteEnd,                StellaKey::K, StellaMod::CTRL},
  {Event::Backspace,                StellaKey::BACKSPACE},
  {Event::Undo,                     StellaKey::Z, CMD},
  {Event::Redo,                     StellaKey::Y, CMD},
  {Event::Redo,                     StellaKey::Z, StellaMod::SHIFT | CMD},
  {Event::Cut,                      StellaKey::X, CMD},
  {Event::Copy,                     StellaKey::C, CMD},
  {Event::Paste,                    StellaKey::V, CMD},
#else
  {Event::MoveLeftWord,             StellaKey::LEFT, StellaMod::CTRL},
  {Event::MoveRightWord,            StellaKey::RIGHT, StellaMod::CTRL},
  {Event::MoveHome,                 StellaKey::HOME},
  {Event::MoveEnd,                  StellaKey::END},
  {Event::SelectLeftWord,           StellaKey::LEFT, StellaMod::SHIFT | StellaMod::CTRL},
  {Event::SelectRightWord,          StellaKey::RIGHT, StellaMod::SHIFT | StellaMod::CTRL},
  {Event::SelectHome,               StellaKey::HOME, StellaMod::SHIFT},
  {Event::SelectEnd,                StellaKey::END, StellaMod::SHIFT},
  {Event::SelectAll,                StellaKey::A, StellaMod::CTRL},
  {Event::Delete,                   StellaKey::DELETE},
  {Event::Delete,                   StellaKey::KP_PERIOD},
  {Event::Delete,                   StellaKey::D, StellaMod::CTRL},
  {Event::DeleteLeftWord,           StellaKey::BACKSPACE, StellaMod::CTRL},
  {Event::DeleteLeftWord,           StellaKey::W, StellaMod::CTRL},
  {Event::DeleteRightWord,          StellaKey::DELETE, StellaMod::CTRL},
  {Event::DeleteRightWord,          StellaKey::D, StellaMod::ALT},
  {Event::DeleteHome,               StellaKey::HOME, StellaMod::CTRL},
  {Event::DeleteHome,               StellaKey::U, StellaMod::CTRL},
  {Event::DeleteEnd,                StellaKey::END, StellaMod::CTRL},
  {Event::DeleteEnd,                StellaKey::K, StellaMod::CTRL},
  {Event::Backspace,                StellaKey::BACKSPACE},
  {Event::Undo,                     StellaKey::Z, StellaMod::CTRL},
  {Event::Undo,                     StellaKey::BACKSPACE, StellaMod::ALT},
  {Event::Redo,                     StellaKey::Y, StellaMod::CTRL},
  {Event::Redo,                     StellaKey::Z, StellaMod::SHIFT | StellaMod::CTRL},
  {Event::Redo,                     StellaKey::BACKSPACE, StellaMod::SHIFT | StellaMod::ALT},
  {Event::Cut,                      StellaKey::X, StellaMod::CTRL},
  {Event::Cut,                      StellaKey::DELETE, StellaMod::SHIFT},
  {Event::Cut,                      StellaKey::KP_PERIOD, StellaMod::SHIFT},
  {Event::Copy,                     StellaKey::C, StellaMod::CTRL},
  {Event::Copy,                     StellaKey::INSERT, StellaMod::CTRL},
  {Event::Paste,                    StellaKey::V, StellaMod::CTRL},
  {Event::Paste,                    StellaKey::INSERT, StellaMod::SHIFT},
#endif
  {Event::EndEdit,                  StellaKey::RETURN},
  {Event::EndEdit,                  StellaKey::KP_ENTER},
  {Event::AbortEdit,                StellaKey::ESCAPE},
};
#endif  // GUI_SUPPORT

#ifdef DEBUGGER_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalKeyboardHandler::EventMappingArray
PhysicalKeyboardHandler::FixedPromptMapping = {
  {Event::UINavNext,                StellaKey::TAB},
  {Event::UINavPrev,                StellaKey::TAB, StellaMod::SHIFT},
  {Event::UIPgUp,                   StellaKey::PAGEUP},
  {Event::UIPgUp,                   StellaKey::PAGEUP, StellaMod::SHIFT},
  {Event::UIPgDown,                 StellaKey::PAGEDOWN},
  {Event::UIPgDown,                 StellaKey::PAGEDOWN, StellaMod::SHIFT},
  {Event::UIHome,                   StellaKey::HOME, StellaMod::SHIFT},
  {Event::UIEnd,                    StellaKey::END, StellaMod::SHIFT},
  {Event::UIUp,                     StellaKey::UP, StellaMod::SHIFT},
  {Event::UIDown,                   StellaKey::DOWN, StellaMod::SHIFT},
  {Event::UILeft,                   StellaKey::DOWN},
  {Event::UIRight,                  StellaKey::UP},
};
#endif  // DEBUGGER_SUPPORT

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalKeyboardHandler::EventMappingArray PhysicalKeyboardHandler::DefaultJoystickMapping = {
  {Event::LeftJoystickUp,           StellaKey::UP},
  {Event::LeftJoystickDown,         StellaKey::DOWN},
  {Event::LeftJoystickLeft,         StellaKey::LEFT},
  {Event::LeftJoystickRight,        StellaKey::RIGHT},
  {Event::LeftJoystickUp,           StellaKey::KP_8},
  {Event::LeftJoystickDown,         StellaKey::KP_2},
  {Event::LeftJoystickLeft,         StellaKey::KP_4},
  {Event::LeftJoystickRight,        StellaKey::KP_6},
  {Event::LeftJoystickFire,         StellaKey::SPACE},
  {Event::LeftJoystickFire,         StellaKey::LCTRL},
  {Event::LeftJoystickFire,         StellaKey::KP_5},
  {Event::LeftJoystickFire5,        StellaKey::_4},
  {Event::LeftJoystickFire5,        StellaKey::RSHIFT},
  {Event::LeftJoystickFire5,        StellaKey::KP_9},
  {Event::LeftJoystickFire9,        StellaKey::_5},
  {Event::LeftJoystickFire9,        StellaKey::RCTRL},
  {Event::LeftJoystickFire9,        StellaKey::KP_3},

  {Event::RightJoystickUp,          StellaKey::Y},
  {Event::RightJoystickDown,        StellaKey::H},
  {Event::RightJoystickLeft,        StellaKey::G},
  {Event::RightJoystickRight,       StellaKey::J},
  {Event::RightJoystickFire,        StellaKey::F},
  {Event::RightJoystickFire5,       StellaKey::_6},
  {Event::RightJoystickFire9,       StellaKey::_7},

  // Same as Joysticks Zero & One + SHIFT
  {Event::QTJoystickThreeUp,        StellaKey::UP, StellaMod::SHIFT},
  {Event::QTJoystickThreeDown,      StellaKey::DOWN, StellaMod::SHIFT},
  {Event::QTJoystickThreeLeft,      StellaKey::LEFT, StellaMod::SHIFT},
  {Event::QTJoystickThreeRight,     StellaKey::RIGHT, StellaMod::SHIFT},
  {Event::QTJoystickThreeUp,        StellaKey::KP_8, StellaMod::SHIFT},
  {Event::QTJoystickThreeDown,      StellaKey::KP_2, StellaMod::SHIFT},
  {Event::QTJoystickThreeLeft,      StellaKey::KP_4, StellaMod::SHIFT},
  {Event::QTJoystickThreeRight,     StellaKey::KP_6, StellaMod::SHIFT},
  {Event::QTJoystickThreeFire,      StellaKey::SPACE, StellaMod::SHIFT},

  {Event::QTJoystickFourUp,         StellaKey::Y, StellaMod::SHIFT},
  {Event::QTJoystickFourDown,       StellaKey::H, StellaMod::SHIFT},
  {Event::QTJoystickFourLeft,       StellaKey::G, StellaMod::SHIFT},
  {Event::QTJoystickFourRight,      StellaKey::J, StellaMod::SHIFT},
  {Event::QTJoystickFourFire,       StellaKey::F, StellaMod::SHIFT},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalKeyboardHandler::EventMappingArray
PhysicalKeyboardHandler::DefaultPaddleMapping = {
  {Event::LeftPaddleADecrease,      StellaKey::RIGHT},
  {Event::LeftPaddleAIncrease,      StellaKey::LEFT},
  {Event::LeftPaddleAFire,          StellaKey::SPACE},
  {Event::LeftPaddleAFire,          StellaKey::LCTRL},
  {Event::LeftPaddleAFire,          StellaKey::KP_5},
  {Event::LeftPaddleAButton1,       StellaKey::UP, StellaMod::SHIFT},
  {Event::LeftPaddleAButton2,       StellaKey::DOWN, StellaMod::SHIFT},

  {Event::LeftPaddleBDecrease,      StellaKey::DOWN},
  {Event::LeftPaddleBIncrease,      StellaKey::UP},
  {Event::LeftPaddleBFire,          StellaKey::_4},
  {Event::LeftPaddleBFire,          StellaKey::RCTRL},

  {Event::RightPaddleADecrease,     StellaKey::J},
  {Event::RightPaddleAIncrease,     StellaKey::G},
  {Event::RightPaddleAFire,         StellaKey::F},
  {Event::RightPaddleAButton1,      StellaKey::Y, StellaMod::SHIFT},
  {Event::RightPaddleAButton2,      StellaKey::H, StellaMod::SHIFT},

  {Event::RightPaddleBDecrease,     StellaKey::H},
  {Event::RightPaddleBIncrease,     StellaKey::Y},
  {Event::RightPaddleBFire,         StellaKey::_6},

  // Same as Paddles Zero..Three Fire + SHIFT
  {Event::QTPaddle3AFire,           StellaKey::SPACE, StellaMod::SHIFT},
  {Event::QTPaddle3BFire,           StellaKey::_4, StellaMod::SHIFT},
  {Event::QTPaddle4AFire,           StellaKey::F, StellaMod::SHIFT},
  {Event::QTPaddle4BFire,           StellaKey::_6, StellaMod::SHIFT},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalKeyboardHandler::EventMappingArray
PhysicalKeyboardHandler::DefaultKeyboardMapping = {
  {Event::LeftKeyboard1,            StellaKey::_1},
  {Event::LeftKeyboard2,            StellaKey::_2},
  {Event::LeftKeyboard3,            StellaKey::_3},
  {Event::LeftKeyboard4,            StellaKey::Q},
  {Event::LeftKeyboard5,            StellaKey::W},
  {Event::LeftKeyboard6,            StellaKey::E},
  {Event::LeftKeyboard7,            StellaKey::A},
  {Event::LeftKeyboard8,            StellaKey::S},
  {Event::LeftKeyboard9,            StellaKey::D},
  {Event::LeftKeyboardStar,         StellaKey::Z},
  {Event::LeftKeyboard0,            StellaKey::X},
  {Event::LeftKeyboardPound,        StellaKey::C},

  {Event::RightKeyboard1,           StellaKey::_8},
  {Event::RightKeyboard2,           StellaKey::_9},
  {Event::RightKeyboard3,           StellaKey::_0},
  {Event::RightKeyboard4,           StellaKey::I},
  {Event::RightKeyboard5,           StellaKey::O},
  {Event::RightKeyboard6,           StellaKey::P},
  {Event::RightKeyboard7,           StellaKey::K},
  {Event::RightKeyboard8,           StellaKey::L},
  {Event::RightKeyboard9,           StellaKey::SEMICOLON},
  {Event::RightKeyboardStar,        StellaKey::COMMA},
  {Event::RightKeyboard0,           StellaKey::PERIOD},
  {Event::RightKeyboardPound,       StellaKey::SLASH},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalKeyboardHandler::EventMappingArray PhysicalKeyboardHandler::DefaultDrivingMapping = {
  {Event::LeftDrivingCCW,          StellaKey::LEFT},
  {Event::LeftDrivingCW,           StellaKey::RIGHT},
  {Event::LeftDrivingCCW,          StellaKey::KP_4},
  {Event::LeftDrivingCW,           StellaKey::KP_6},
  {Event::LeftDrivingFire,         StellaKey::SPACE},
  {Event::LeftDrivingFire,         StellaKey::LCTRL},
  {Event::LeftDrivingFire,         StellaKey::KP_5},
  {Event::LeftDrivingButton1,      StellaKey::UP},
  {Event::LeftDrivingButton2,      StellaKey::DOWN},
  {Event::LeftDrivingButton1,      StellaKey::KP_8},
  {Event::LeftDrivingButton2,      StellaKey::KP_2},

  {Event::RightDrivingCCW,         StellaKey::G},
  {Event::RightDrivingCW,          StellaKey::J},
  {Event::RightDrivingFire,        StellaKey::F},
  {Event::RightDrivingButton1,     StellaKey::Y},
  {Event::RightDrivingButton2,     StellaKey::H},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalKeyboardHandler::EventMappingArray
PhysicalKeyboardHandler::CompuMateMapping = {
  {Event::CompuMateShift,         StellaKey::LSHIFT},
  {Event::CompuMateShift,         StellaKey::RSHIFT},
  {Event::CompuMateFunc,          StellaKey::LCTRL},
  {Event::CompuMateFunc,          StellaKey::RCTRL},
  {Event::CompuMate0,             StellaKey::_0},
  {Event::CompuMate1,             StellaKey::_1},
  {Event::CompuMate2,             StellaKey::_2},
  {Event::CompuMate3,             StellaKey::_3},
  {Event::CompuMate4,             StellaKey::_4},
  {Event::CompuMate5,             StellaKey::_5},
  {Event::CompuMate6,             StellaKey::_6},
  {Event::CompuMate7,             StellaKey::_7},
  {Event::CompuMate8,             StellaKey::_8},
  {Event::CompuMate9,             StellaKey::_9},
  {Event::CompuMateA,             StellaKey::A},
  {Event::CompuMateB,             StellaKey::B},
  {Event::CompuMateC,             StellaKey::C},
  {Event::CompuMateD,             StellaKey::D},
  {Event::CompuMateE,             StellaKey::E},
  {Event::CompuMateF,             StellaKey::F},
  {Event::CompuMateG,             StellaKey::G},
  {Event::CompuMateH,             StellaKey::H},
  {Event::CompuMateI,             StellaKey::I},
  {Event::CompuMateJ,             StellaKey::J},
  {Event::CompuMateK,             StellaKey::K},
  {Event::CompuMateL,             StellaKey::L},
  {Event::CompuMateM,             StellaKey::M},
  {Event::CompuMateN,             StellaKey::N},
  {Event::CompuMateO,             StellaKey::O},
  {Event::CompuMateP,             StellaKey::P},
  {Event::CompuMateQ,             StellaKey::Q},
  {Event::CompuMateR,             StellaKey::R},
  {Event::CompuMateS,             StellaKey::S},
  {Event::CompuMateT,             StellaKey::T},
  {Event::CompuMateU,             StellaKey::U},
  {Event::CompuMateV,             StellaKey::V},
  {Event::CompuMateW,             StellaKey::W},
  {Event::CompuMateX,             StellaKey::X},
  {Event::CompuMateY,             StellaKey::Y},
  {Event::CompuMateZ,             StellaKey::Z},
  {Event::CompuMateComma,         StellaKey::COMMA},
  {Event::CompuMatePeriod,        StellaKey::PERIOD},
  {Event::CompuMateEnter,         StellaKey::RETURN},
  {Event::CompuMateEnter,         StellaKey::KP_ENTER},
  {Event::CompuMateSpace,         StellaKey::SPACE},
  // extra emulated keys
  {Event::CompuMateQuestion,      StellaKey::SLASH, StellaMod::SHIFT},
  {Event::CompuMateLeftBracket,   StellaKey::LEFTBRACKET},
  {Event::CompuMateRightBracket,  StellaKey::RIGHTBRACKET},
  {Event::CompuMateMinus,         StellaKey::MINUS},
  {Event::CompuMateQuote,         StellaKey::APOSTROPHE, StellaMod::SHIFT},
  {Event::CompuMateBackspace,     StellaKey::BACKSPACE},
  {Event::CompuMateEquals,        StellaKey::EQUALS},
  {Event::CompuMatePlus,          StellaKey::EQUALS, StellaMod::SHIFT},
  {Event::CompuMateSlash,         StellaKey::SLASH}
};
// NOLINTEND(bugprone-throwing-static-initialization)

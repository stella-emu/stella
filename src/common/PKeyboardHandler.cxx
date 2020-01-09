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

#include "OSystem.hxx"
#include "Console.hxx"
#include "Settings.hxx"
#include "EventHandler.hxx"
#include "Sound.hxx"
#include "StateManager.hxx"
#include "StellaKeys.hxx"
#include "TIASurface.hxx"
#include "PNGLibrary.hxx"
#include "PKeyboardHandler.hxx"

#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif
#ifdef GUI_SUPPORT
  #include "DialogContainer.hxx"
#endif

#if defined(BSPF_MACOS) || defined(MACOS_KEYS)
static constexpr int MOD3 = KBDM_GUI;
#else
static constexpr int MOD3 = KBDM_ALT;
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalKeyboardHandler::PhysicalKeyboardHandler(OSystem& system, EventHandler& handler)
  : myOSystem(system),
    myHandler(handler)
{
  Int32 version = myOSystem.settings().getInt("event_ver");

  // Compare if event list version has changed so that key maps became invalid
  if (version == Event::VERSION)
  {
    string list = myOSystem.settings().getString("keymap_emu");
    myKeyMap.loadMapping(list, EventMode::kCommonMode);
    list = myOSystem.settings().getString("keymap_joy");
    myKeyMap.loadMapping(list, EventMode::kJoystickMode);
    list = myOSystem.settings().getString("keymap_pad");
    myKeyMap.loadMapping(list, EventMode::kPaddlesMode);
    list = myOSystem.settings().getString("keymap_key");
    myKeyMap.loadMapping(list, EventMode::kKeypadMode);
    list = myOSystem.settings().getString("keymap_ui");
    myKeyMap.loadMapping(list, EventMode::kMenuMode);
  }
  myKeyMap.enableMod() = myOSystem.settings().getBool("modcombo");

  setDefaultMapping(Event::NoType, EventMode::kEmulationMode, true);
  setDefaultMapping(Event::NoType, EventMode::kMenuMode, true);
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
  bool eraseAll = !updateDefaults && (event == Event::NoType);

  if (updateDefaults)
  {
    // if there is no existing mapping for the event or
    //  the default mapping for the event is unused, set default key for event
    if (myKeyMap.getEventMapping(map.event, mode).size() == 0 ||
      !myKeyMap.check(mode, map.key, map.mod))
    {
      myKeyMap.add(map.event, mode, map.key, map.mod);
    }
  }
  else if (eraseAll || map.event == event)
  {
    //myKeyMap.eraseEvent(map.event, mode);
    myKeyMap.add(map.event, mode, map.key, map.mod);
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
      for (const auto& item: DefaultCommonMapping)
        setDefaultKey(item, event, EventMode::kCommonMode, updateDefaults);
      // put all controller events into their own mode's mappings
      for (const auto& item: DefaultJoystickMapping)
        setDefaultKey(item, event, EventMode::kJoystickMode, updateDefaults);
      for (const auto& item: DefaultPaddleMapping)
        setDefaultKey(item, event, EventMode::kPaddlesMode, updateDefaults);
      for (const auto& item: DefaultKeypadMapping)
        setDefaultKey(item, event, EventMode::kKeypadMode, updateDefaults);
      for (const auto& item : CompuMateMapping)
        setDefaultKey(item, event, EventMode::kCompuMateMode, updateDefaults);
      break;

    case EventMode::kMenuMode:
      for (const auto& item: DefaultMenuMapping)
        setDefaultKey(item, event, EventMode::kMenuMode, updateDefaults);
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::defineControllerMappings(const Controller::Type type, Controller::Jack port)
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
void PhysicalKeyboardHandler::enableEmulationMappings()
{
  // start from scratch and enable common mappings
  myKeyMap.eraseMode(EventMode::kEmulationMode);
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

    case EventMode::kCompuMateMode:
      // see below
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

    case EventMode::kCompuMateMode:
      for (const auto& item : CompuMateMapping)
        enableMapping(item.event, EventMode::kCompuMateMode);
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
    Event::Type event = static_cast<Event::Type>(i);

    if (isCommonEvent(event))
      enableMapping(event, EventMode::kCommonMode);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::enableMappings(const Event::EventSet& events, EventMode mode)
{
  for (const auto& event : events)
    enableMapping(event, mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::enableMapping(const Event::Type event, EventMode mode)
{
  // copy from controller mode into emulation mode
  KeyMap::MappingArray mappings = myKeyMap.getEventMapping(event, mode);

  for (const auto& mapping : mappings)
    myKeyMap.add(event, EventMode::kEmulationMode, mapping.key, mapping.mod);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventMode PhysicalKeyboardHandler::getEventMode(const Event::Type event, const EventMode mode) const
{
  if (mode == EventMode::kEmulationMode)
  {
    if (isJoystickEvent(event))
      return EventMode::kJoystickMode;

    if (isPaddleEvent(event))
      return EventMode::kPaddlesMode;

    if (isKeypadEvent(event))
      return EventMode::kKeypadMode;

    if (isCommonEvent(event))
      return EventMode::kCommonMode;
  }

  return mode;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalKeyboardHandler::isJoystickEvent(const Event::Type event) const
{
  return LeftJoystickEvents.find(event) != LeftJoystickEvents.end()
    || RightJoystickEvents.find(event) != RightJoystickEvents.end();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalKeyboardHandler::isPaddleEvent(const Event::Type event) const
{
  return LeftPaddlesEvents.find(event) != LeftPaddlesEvents.end()
    || RightPaddlesEvents.find(event) != RightPaddlesEvents.end();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalKeyboardHandler::isKeypadEvent(const Event::Type event) const
{
  return LeftKeypadEvents.find(event) != LeftKeypadEvents.end()
    || RightKeypadEvents.find(event) != RightKeypadEvents.end();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalKeyboardHandler::isCommonEvent(const Event::Type event) const
{
  return !(isJoystickEvent(event) || isPaddleEvent(event) || isKeypadEvent(event));
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
  myOSystem.settings().setValue("keymap_emu", myKeyMap.saveMapping(EventMode::kCommonMode));
  myOSystem.settings().setValue("keymap_joy", myKeyMap.saveMapping(EventMode::kJoystickMode));
  myOSystem.settings().setValue("keymap_pad", myKeyMap.saveMapping(EventMode::kPaddlesMode));
  myOSystem.settings().setValue("keymap_key", myKeyMap.saveMapping(EventMode::kKeypadMode));
  myOSystem.settings().setValue("keymap_ui", myKeyMap.saveMapping(EventMode::kMenuMode));
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
    EventMode evMode = getEventMode(event, mode);

    myKeyMap.add(event, evMode, key, mod);
    if (evMode == myLeftMode || evMode == myRightMode)
      myKeyMap.add(event, mode, key, mod);
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::handleEvent(StellaKey key, StellaMod mod, bool pressed, bool repeated)
{
#ifdef BSPF_UNIX
  // Swallow KBDK_TAB under certain conditions
  // See commments on 'myAltKeyCounter' for more information
  if(myAltKeyCounter > 1 && key == KBDK_TAB)
  {
    myAltKeyCounter = 0;
    return;
  }
  if (key == KBDK_TAB && pressed && StellaModTest::isAlt(mod))
  {
    // Swallow Alt-Tab, but remember that it happened
    myAltKeyCounter = 1;
    return;
  }
#endif

  EventHandlerState estate = myHandler.state();

  // special handling for CompuMate in emulation modes
  if ((estate == EventHandlerState::EMULATION || estate == EventHandlerState::PAUSE) &&
      myOSystem.console().leftController().type() == Controller::Type::CompuMate)
  {
    Event::Type event = myKeyMap.get(EventMode::kCompuMateMode, key, mod);

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
PhysicalKeyboardHandler::EventMappingArray PhysicalKeyboardHandler::DefaultCommonMapping = {
  {Event::ConsoleSelect,            KBDK_F1},
  {Event::ConsoleReset,             KBDK_F2},
  {Event::ConsoleColor,             KBDK_F3},
  {Event::ConsoleBlackWhite,        KBDK_F4},
  {Event::ConsoleLeftDiffA,         KBDK_F5},
  {Event::ConsoleLeftDiffB,         KBDK_F6},
  {Event::ConsoleRightDiffA,        KBDK_F7},
  {Event::ConsoleRightDiffB,        KBDK_F8},
  {Event::SaveState,                KBDK_F9},
  {Event::SaveAllStates,            KBDK_F9, MOD3},
  {Event::ChangeState,              KBDK_F10},
  {Event::ToggleAutoSlot,           KBDK_F10, MOD3},
  {Event::LoadState,                KBDK_F11},
  {Event::LoadAllStates,            KBDK_F11, MOD3},
  {Event::TakeSnapshot,             KBDK_F12},
  {Event::Fry,                      KBDK_BACKSPACE},
  {Event::TogglePauseMode,          KBDK_PAUSE},
  {Event::OptionsMenuMode,          KBDK_TAB},
  {Event::CmdMenuMode,              KBDK_BACKSLASH},
  {Event::TimeMachineMode,          KBDK_T},
  {Event::DebuggerMode,             KBDK_GRAVE},
  {Event::ExitMode,                 KBDK_ESCAPE},
#ifdef BSPF_MACOS
  {Event::Quit,                     KBDK_Q, MOD3},
#else
  {Event::Quit,                     KBDK_Q, KBDM_CTRL},
#endif
  {Event::ReloadConsole,            KBDK_R, KBDM_CTRL},

  {Event::VidmodeDecrease,          KBDK_MINUS, MOD3},
  {Event::VidmodeIncrease,          KBDK_EQUALS, MOD3},
  {Event::VCenterDecrease,          KBDK_PAGEDOWN, MOD3},
  {Event::VCenterIncrease,          KBDK_PAGEUP, MOD3},
  {Event::ScanlineAdjustDecrease,   KBDK_PAGEDOWN, KBDM_SHIFT | MOD3},
  {Event::ScanlineAdjustIncrease,   KBDK_PAGEUP, KBDM_SHIFT | MOD3},
  {Event::VolumeDecrease,           KBDK_LEFTBRACKET, MOD3},
  {Event::VolumeIncrease,           KBDK_RIGHTBRACKET, MOD3},
  {Event::SoundToggle,              KBDK_RIGHTBRACKET, KBDM_CTRL},

  {Event::ToggleFullScreen,         KBDK_RETURN, MOD3},
  {Event::OverscanDecrease,         KBDK_PAGEDOWN, KBDM_SHIFT},
  {Event::OverscanIncrease,         KBDK_PAGEUP, KBDM_SHIFT},
  {Event::VidmodeStd,               KBDK_1, MOD3},
  {Event::VidmodeRGB,               KBDK_2, MOD3},
  {Event::VidmodeSVideo,            KBDK_3, MOD3},
  {Event::VidModeComposite,         KBDK_4, MOD3},
  {Event::VidModeBad,               KBDK_5, MOD3},
  {Event::VidModeCustom,            KBDK_6, MOD3},
  {Event::PreviousAttribute,        KBDK_7, KBDM_SHIFT | MOD3},
  {Event::NextAttribute,            KBDK_7, MOD3},
  {Event::DecreaseAttribute,        KBDK_8, KBDM_SHIFT | MOD3},
  {Event::IncreaseAttribute,        KBDK_8, MOD3},
  {Event::PhosphorDecrease,         KBDK_9, KBDM_SHIFT | MOD3},
  {Event::PhosphorIncrease,         KBDK_9, MOD3},
  {Event::TogglePhosphor,           KBDK_P, MOD3},
  {Event::ScanlinesDecrease,        KBDK_0, KBDM_SHIFT | MOD3},
  {Event::ScanlinesIncrease,        KBDK_0, MOD3},
  {Event::ToggleColorLoss,          KBDK_L, KBDM_CTRL},
  {Event::TogglePalette,            KBDK_P, KBDM_CTRL},
  {Event::ToggleInter,              KBDK_I, KBDM_CTRL},
  {Event::ToggleJitter,             KBDK_J, MOD3},
  {Event::ToggleFrameStats,         KBDK_L, MOD3},
  {Event::ToggleTimeMachine,        KBDK_T, MOD3},
#ifdef PNG_SUPPORT
  {Event::ToggleContSnapshots,      KBDK_S, MOD3},
  {Event::ToggleContSnapshotsFrame, KBDK_S, KBDM_SHIFT | MOD3},
#endif
  {Event::HandleMouseControl,       KBDK_0, KBDM_CTRL},
  {Event::ToggleGrabMouse,          KBDK_G, KBDM_CTRL},
  {Event::ToggleSAPortOrder,        KBDK_1, KBDM_CTRL},
  {Event::FormatDecrease,           KBDK_F, KBDM_SHIFT | KBDM_CTRL},
  {Event::FormatIncrease,           KBDK_F, KBDM_CTRL},

  {Event::ToggleP0Collision,        KBDK_Z, KBDM_SHIFT | MOD3},
  {Event::ToggleP0Bit,              KBDK_Z, MOD3},
  {Event::ToggleP1Collision,        KBDK_X, KBDM_SHIFT | MOD3},
  {Event::ToggleP1Bit,              KBDK_X, MOD3},
  {Event::ToggleM0Collision,        KBDK_C, KBDM_SHIFT | MOD3},
  {Event::ToggleM0Bit,              KBDK_C, MOD3},
  {Event::ToggleM1Collision,        KBDK_V, KBDM_SHIFT | MOD3},
  {Event::ToggleM1Bit,              KBDK_V, MOD3},
  {Event::ToggleBLCollision,        KBDK_B, KBDM_SHIFT | MOD3},
  {Event::ToggleBLBit,              KBDK_B, MOD3},
  {Event::TogglePFCollision,        KBDK_N, KBDM_SHIFT | MOD3},
  {Event::TogglePFBit,              KBDK_N, MOD3},
  {Event::ToggleCollisions,         KBDK_COMMA, KBDM_SHIFT | MOD3},
  {Event::ToggleBits,               KBDK_COMMA, MOD3},
  {Event::ToggleFixedColors,        KBDK_PERIOD, MOD3},

  {Event::RewindPause,              KBDK_LEFT, KBDM_SHIFT},
  {Event::Rewind1Menu,              KBDK_LEFT, MOD3},
  {Event::Rewind10Menu,             KBDK_LEFT, KBDM_SHIFT | MOD3},
  {Event::RewindAllMenu,            KBDK_DOWN, MOD3},
  {Event::UnwindPause,              KBDK_LEFT, KBDM_SHIFT},
  {Event::Unwind1Menu,              KBDK_RIGHT, MOD3},
  {Event::Unwind10Menu,             KBDK_RIGHT, KBDM_SHIFT | MOD3},
  {Event::UnwindAllMenu,            KBDK_UP, MOD3},

#if defined(RETRON77)
  {Event::ConsoleColorToggle,       KBDK_F4},         // back ("COLOR","B/W")
  {Event::ConsoleLeftDiffToggle,    KBDK_F6},         // front ("SKILL P1")
  {Event::ConsoleRightDiffToggle,   KBDK_F8},         // front ("SKILL P2")
  {Event::CmdMenuMode,              KBDK_F13},        // back ("4:3","16:9")
  {Event::ExitMode,                 KBDK_BACKSPACE},  // back ("FRY")
#endif
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalKeyboardHandler::EventMappingArray PhysicalKeyboardHandler::DefaultMenuMapping = {
  {Event::UIUp,                     KBDK_UP},
  {Event::UIDown,                   KBDK_DOWN},
  {Event::UILeft,                   KBDK_LEFT},
  {Event::UIRight,                  KBDK_RIGHT},
  {Event::UISelect,                 KBDK_RETURN},

  {Event::UIHome,                   KBDK_HOME},
  {Event::UIEnd,                    KBDK_END},
  {Event::UIPgUp,                   KBDK_PAGEUP},
  {Event::UIPgDown,                 KBDK_PAGEDOWN},
  // same with keypad
  {Event::UIUp,                     KBDK_KP_8},
  {Event::UIDown,                   KBDK_KP_2},
  {Event::UILeft,                   KBDK_KP_4},
  {Event::UIRight,                  KBDK_KP_6},
  {Event::UISelect,                 KBDK_KP_ENTER},

  {Event::UIHome,                   KBDK_KP_7},
  {Event::UIEnd,                    KBDK_KP_1},
  {Event::UIPgUp,                   KBDK_KP_9},
  {Event::UIPgDown,                 KBDK_KP_3},

  {Event::UICancel,                 KBDK_ESCAPE},

  {Event::UINavPrev,                KBDK_TAB, KBDM_SHIFT},
  {Event::UINavNext,                KBDK_TAB},
  {Event::UITabPrev,                KBDK_TAB, KBDM_SHIFT | KBDM_CTRL},
  {Event::UITabNext,                KBDK_TAB, KBDM_CTRL},

  {Event::UIPrevDir,                KBDK_BACKSPACE},
  {Event::ToggleFullScreen,         KBDK_RETURN, MOD3},

#ifdef BSPF_MACOS
  {Event::Quit,                     KBDK_Q, MOD3},
#else
  {Event::Quit,                     KBDK_Q, KBDM_CTRL},
#endif

#if defined(RETRON77)
  {Event::UIUp,                     KBDK_F9},         // front ("SAVE")
  {Event::UIDown,                   KBDK_F2},         // front ("RESET")
  {Event::UINavPrev,                KBDK_F11},        // front ("LOAD")
  {Event::UINavNext,                KBDK_F1},         // front ("MODE")
  {Event::UISelect,                 KBDK_F6},         // front ("SKILL P1")
  {Event::UICancel,                 KBDK_F8},         // front ("SKILL P2")
  //{Event::NoType,                   KBDK_F4},         // back ("COLOR","B/W")
  {Event::UITabPrev,                KBDK_F13},        // back ("4:3","16:9")
  {Event::UITabNext,                KBDK_BACKSPACE},  // back (FRY)
#endif
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalKeyboardHandler::EventMappingArray PhysicalKeyboardHandler::DefaultJoystickMapping = {
  {Event::JoystickZeroUp,           KBDK_UP},
  {Event::JoystickZeroDown,         KBDK_DOWN},
  {Event::JoystickZeroLeft,         KBDK_LEFT},
  {Event::JoystickZeroRight,        KBDK_RIGHT},
  {Event::JoystickZeroUp,           KBDK_KP_8},
  {Event::JoystickZeroDown,         KBDK_KP_2},
  {Event::JoystickZeroLeft,         KBDK_KP_4},
  {Event::JoystickZeroRight,        KBDK_KP_6},
  {Event::JoystickZeroFire,         KBDK_SPACE},
  {Event::JoystickZeroFire,         KBDK_LCTRL},
  {Event::JoystickZeroFire,         KBDK_KP_5},
  {Event::JoystickZeroFire5,        KBDK_4},
  {Event::JoystickZeroFire5,        KBDK_RSHIFT},
  {Event::JoystickZeroFire5,        KBDK_KP_9},
  {Event::JoystickZeroFire9,        KBDK_5},
  {Event::JoystickZeroFire9,        KBDK_RCTRL},
  {Event::JoystickZeroFire9,        KBDK_KP_3},
  {Event::JoystickOneUp,            KBDK_Y},
  {Event::JoystickOneDown,          KBDK_H},
  {Event::JoystickOneLeft,          KBDK_G},
  {Event::JoystickOneRight,         KBDK_J},
  {Event::JoystickOneFire,          KBDK_F},
  {Event::JoystickOneFire5,         KBDK_6},
  {Event::JoystickOneFire9,         KBDK_7},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalKeyboardHandler::EventMappingArray PhysicalKeyboardHandler::DefaultPaddleMapping = {
  {Event::PaddleZeroDecrease,       KBDK_RIGHT},
  {Event::PaddleZeroIncrease,       KBDK_LEFT},
  {Event::PaddleZeroFire,           KBDK_SPACE},
  {Event::PaddleZeroFire,           KBDK_LCTRL},
  {Event::PaddleZeroFire,           KBDK_KP_5},

  {Event::PaddleOneDecrease,        KBDK_DOWN},
  {Event::PaddleOneIncrease,        KBDK_UP},
  {Event::PaddleOneFire,            KBDK_4},
  {Event::PaddleOneFire,            KBDK_RCTRL},

  {Event::PaddleTwoDecrease,        KBDK_J},
  {Event::PaddleTwoIncrease,        KBDK_G},
  {Event::PaddleTwoFire,            KBDK_F},

  {Event::PaddleThreeDecrease,      KBDK_H},
  {Event::PaddleThreeIncrease,      KBDK_Y},
  {Event::PaddleThreeFire,          KBDK_6},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalKeyboardHandler::EventMappingArray PhysicalKeyboardHandler::DefaultKeypadMapping = {
  {Event::KeyboardZero1,            KBDK_1},
  {Event::KeyboardZero2,            KBDK_2},
  {Event::KeyboardZero3,            KBDK_3},
  {Event::KeyboardZero4,            KBDK_Q},
  {Event::KeyboardZero5,            KBDK_W},
  {Event::KeyboardZero6,            KBDK_E},
  {Event::KeyboardZero7,            KBDK_A},
  {Event::KeyboardZero8,            KBDK_S},
  {Event::KeyboardZero9,            KBDK_D},
  {Event::KeyboardZeroStar,         KBDK_Z},
  {Event::KeyboardZero0,            KBDK_X},
  {Event::KeyboardZeroPound,        KBDK_C},

  {Event::KeyboardOne1,             KBDK_8},
  {Event::KeyboardOne2,             KBDK_9},
  {Event::KeyboardOne3,             KBDK_0},
  {Event::KeyboardOne4,             KBDK_I},
  {Event::KeyboardOne5,             KBDK_O},
  {Event::KeyboardOne6,             KBDK_P},
  {Event::KeyboardOne7,             KBDK_K},
  {Event::KeyboardOne8,             KBDK_L},
  {Event::KeyboardOne9,             KBDK_SEMICOLON},
  {Event::KeyboardOneStar,          KBDK_COMMA},
  {Event::KeyboardOne0,             KBDK_PERIOD},
  {Event::KeyboardOnePound,         KBDK_SLASH},
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalKeyboardHandler::EventMappingArray PhysicalKeyboardHandler::CompuMateMapping = {
  {Event::CompuMateShift,         KBDK_LSHIFT},
  {Event::CompuMateShift,         KBDK_RSHIFT},
  {Event::CompuMateFunc,          KBDK_LCTRL},
  {Event::CompuMateFunc,          KBDK_RCTRL},
  {Event::CompuMate0,             KBDK_0},
  {Event::CompuMate1,             KBDK_1},
  {Event::CompuMate2,             KBDK_2},
  {Event::CompuMate3,             KBDK_3},
  {Event::CompuMate4,             KBDK_4},
  {Event::CompuMate5,             KBDK_5},
  {Event::CompuMate6,             KBDK_6},
  {Event::CompuMate7,             KBDK_7},
  {Event::CompuMate8,             KBDK_8},
  {Event::CompuMate9,             KBDK_9},
  {Event::CompuMateA,             KBDK_A},
  {Event::CompuMateB,             KBDK_B},
  {Event::CompuMateC,             KBDK_C},
  {Event::CompuMateD,             KBDK_D},
  {Event::CompuMateE,             KBDK_E},
  {Event::CompuMateF,             KBDK_F},
  {Event::CompuMateG,             KBDK_G},
  {Event::CompuMateH,             KBDK_H},
  {Event::CompuMateI,             KBDK_I},
  {Event::CompuMateJ,             KBDK_J},
  {Event::CompuMateK,             KBDK_K},
  {Event::CompuMateL,             KBDK_L},
  {Event::CompuMateM,             KBDK_M},
  {Event::CompuMateN,             KBDK_N},
  {Event::CompuMateO,             KBDK_O},
  {Event::CompuMateP,             KBDK_P},
  {Event::CompuMateQ,             KBDK_Q},
  {Event::CompuMateR,             KBDK_R},
  {Event::CompuMateS,             KBDK_S},
  {Event::CompuMateT,             KBDK_T},
  {Event::CompuMateU,             KBDK_U},
  {Event::CompuMateV,             KBDK_V},
  {Event::CompuMateW,             KBDK_W},
  {Event::CompuMateX,             KBDK_X},
  {Event::CompuMateY,             KBDK_Y},
  {Event::CompuMateZ,             KBDK_Z},
  {Event::CompuMateComma,         KBDK_COMMA},
  {Event::CompuMatePeriod,        KBDK_PERIOD},
  {Event::CompuMateEnter,         KBDK_RETURN},
  {Event::CompuMateEnter,         KBDK_KP_ENTER},
  {Event::CompuMateSpace,         KBDK_SPACE},
  // extra emulated keys
  {Event::CompuMateQuestion,      KBDK_SLASH, KBDM_SHIFT},
  {Event::CompuMateLeftBracket,   KBDK_LEFTBRACKET},
  {Event::CompuMateRightBracket,  KBDK_RIGHTBRACKET},
  {Event::CompuMateMinus,         KBDK_MINUS},
  {Event::CompuMateQuote,         KBDK_APOSTROPHE, KBDM_SHIFT},
  {Event::CompuMateBackspace,     KBDK_BACKSPACE},
  {Event::CompuMateEquals,        KBDK_EQUALS},
  {Event::CompuMatePlus,          KBDK_EQUALS, KBDM_SHIFT},
  {Event::CompuMateSlash,         KBDK_SLASH}
};

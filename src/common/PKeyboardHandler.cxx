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

#include "OSystem.hxx"
#include "Console.hxx"
#include "Settings.hxx"
#include "EventHandler.hxx"
#include "Event.hxx"
#include "Sound.hxx"
#include "StateManager.hxx"
#include "StellaKeys.hxx"
#include "TIASurface.hxx"
#include "PNGLibrary.hxx"
#include "PKeyboardHandler.hxx"
#include "KeyMap.hxx"

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
PhysicalKeyboardHandler::PhysicalKeyboardHandler(
      OSystem& system, EventHandler& handler, Event& event)
  : myOSystem(system),
    myHandler(handler),
    myEvent(event),
    myAltKeyCounter(0)
{
  Int32 version = myOSystem.settings().getInt("event_ver");

  // Compare if event list version has changed so that key maps became invalid
  if (version == Event::VERSION)
  {
    string list = myOSystem.settings().getString("keymap_emu");
    myKeyMap.loadMapping(list, kEmulationMode);
    list = myOSystem.settings().getString("keymap_joy");
    myKeyMap.loadMapping(list, kJoystickMode);
    list = myOSystem.settings().getString("keymap_pad");
    myKeyMap.loadMapping(list, kPaddlesMode);
    list = myOSystem.settings().getString("keymap_key");
    myKeyMap.loadMapping(list, kKeypadMode);
    list = myOSystem.settings().getString("keymap_ui");
    myKeyMap.loadMapping(list, kMenuMode);
  }
  myKeyMap.enableMod() = myOSystem.settings().getBool("modcombo");

  setDefaultMapping(Event::NoType, kEmulationMode, true);
  setDefaultMapping(Event::NoType, kMenuMode, true);
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
    myKeyMap.eraseEvent(map.event, mode);
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
  switch(mode)
  {
    case kEmulationMode:
      for (const auto& item: DefaultEmuMapping)
        setDefaultKey(item, event, kEmulationMode, updateDefaults);
      // put all controller events into their own mode's mappings
      for (const auto& item: DefaultJoystickMapping)
        setDefaultKey(item, event, kJoystickMode, updateDefaults);
      for (const auto& item: DefaultPaddleMapping)
        setDefaultKey(item, event, kPaddlesMode, updateDefaults);
      for (const auto& item: DefaultKeypadMapping)
        setDefaultKey(item, event, kKeypadMode, updateDefaults);
      break;

    case kMenuMode:
      for (const auto& item: DefaultMenuMapping)
        setDefaultKey(item, event, kMenuMode, updateDefaults);
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::enableControllerEvents(const string& controllerName, Controller::Jack port)
{
  if ((controllerName == "KEYBOARD") || (controllerName == "KEYPAD"))
  {
    if (port == Controller::Jack::Left)
      myLeftMode = kKeypadMode;
    else
      myRightMode = kKeypadMode;
  }
  else if(BSPF::startsWithIgnoreCase(controllerName, "PADDLES"))
  {
    if (port == Controller::Jack::Left)
      myLeftMode = kPaddlesMode;
    else
      myRightMode = kPaddlesMode;
  }
  else
  {
    if (port == Controller::Jack::Left)
      myLeftMode = kJoystickMode;
    else
      myRightMode = kJoystickMode;
  }

  enableControllerEvents();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::enableControllerEvents()
{
  // enable right mode first, so that in case of mapping clashes the left controller has preference
  switch (myRightMode)
  {
    case kPaddlesMode:
      enableRightPaddlesMapping();
      break;

    case kKeypadMode:
      enableRightKeypadMapping();
      break;

    default:
      enableRightJoystickMapping();
      break;
  }

  switch (myLeftMode)
  {
    case kPaddlesMode:
      enableLeftPaddlesMapping();
      break;

    case kKeypadMode:
      enableLeftKeypadMapping();
      break;

    default:
      enableLeftJoystickMapping();
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::enableLeftJoystickMapping(bool enable)
{
  for (const auto& event: LeftJoystickEvents)
  {
    // copy from controller specific mode into emulation mode
    std::vector<KeyMap::Mapping> mappings = myKeyMap.getEventMapping(event, kJoystickMode);
    enableMappings(event, mappings, enable);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::enableRightJoystickMapping(bool enable)
{
  for (const auto& event: RightJoystickEvents)
  {
    // copy from controller specific mode into emulation mode
    std::vector<KeyMap::Mapping> mappings = myKeyMap.getEventMapping(event, kJoystickMode);
    enableMappings(event, mappings, enable);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::enableLeftPaddlesMapping(bool enable)
{
  for (const auto& event: LeftPaddlesEvents)
  {
    // copy from controller mode into emulation mode
    std::vector<KeyMap::Mapping> mappings = myKeyMap.getEventMapping(event, kPaddlesMode);
    enableMappings(event, mappings, enable);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::enableRightPaddlesMapping(bool enable)
{
  for (const auto& event: RightPaddlesEvents)
  {
    // copy from controller mode into emulation mode
    std::vector<KeyMap::Mapping> mappings = myKeyMap.getEventMapping(event, kPaddlesMode);
    enableMappings(event, mappings, enable);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::enableLeftKeypadMapping(bool enable)
{
  for (const auto& event: LeftKeypadEvents)
  {
    // copy from controller mode into emulation mode
    std::vector<KeyMap::Mapping> mappings = myKeyMap.getEventMapping(event, kKeypadMode);
    enableMappings(event, mappings, enable);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::enableRightKeypadMapping(bool enable)
{
  for (const auto& event: RightKeypadEvents)
  {
    // copy from controller mode into emulation mode
    std::vector<KeyMap::Mapping> mappings = myKeyMap.getEventMapping(event, kKeypadMode);
    enableMappings(event, mappings, enable);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::enableMappings(Event::Type event, std::vector<KeyMap::Mapping> mappings, bool enable)
{
  for (const auto& mapping: mappings)
  {
    if (enable)
      myKeyMap.add(event, kEmulationMode, mapping.key, mapping.mod);
    else
      myKeyMap.eraseEvent(event, kEmulationMode);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventMode PhysicalKeyboardHandler::getEventMode(const Event::Type event, const EventMode mode) const
{
  if (isJoystickEvent(event))
    return kJoystickMode;

  if (isPaddleEvent(event))
    return kPaddlesMode;

  if (isKeypadEvent(event))
    return kKeypadMode;

  return kEmulationMode;
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
void PhysicalKeyboardHandler::eraseMapping(Event::Type event, EventMode mode)
{
  myKeyMap.eraseEvent(event, mode);
  myKeyMap.eraseEvent(event, getEventMode(event, mode));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::saveMapping()
{
  myOSystem.settings().setValue("event_ver", Event::VERSION);
  // remove controller specific events in emulation mode mapping
  enableLeftJoystickMapping(false);
  enableRightJoystickMapping(false);
  enableLeftPaddlesMapping(false);
  enableRightPaddlesMapping(false);
  enableLeftKeypadMapping(false);
  enableRightKeypadMapping(false);
  myOSystem.settings().setValue("keymap_emu", myKeyMap.saveMapping(kEmulationMode));
  // restore current controller's event mappings
  enableControllerEvents();
  myOSystem.settings().setValue("keymap_joy", myKeyMap.saveMapping(kJoystickMode));
  myOSystem.settings().setValue("keymap_pad", myKeyMap.saveMapping(kPaddlesMode));
  myOSystem.settings().setValue("keymap_key", myKeyMap.saveMapping(kKeypadMode));
  myOSystem.settings().setValue("keymap_ui", myKeyMap.saveMapping(kMenuMode));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string PhysicalKeyboardHandler::getMappingDesc(Event::Type event, EventMode mode) const
{
  return myKeyMap.getEventMappingDesc(event, getEventMode(event, mode));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalKeyboardHandler::addMapping(Event::Type event, EventMode mode,
                                         StellaKey key, StellaMod mod)
{
  // These keys cannot be remapped
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
  // Swallow KBDK_TAB under certain conditions
  // See commments on 'myAltKeyCounter' for more information
#ifdef BSPF_UNIX
  if(myAltKeyCounter > 1 && key == KBDK_TAB)
  {
    myAltKeyCounter = 0;
    return;
  }
#endif

  // Immediately store the key state
  myEvent.setKey(key, pressed);

  // An attempt to speed up event processing; we quickly check for
  // Control or Alt/Cmd combos first
  // and don't pass the key on if we've already taken care of it
  if(handleAltEvent(key, mod, pressed))
    return;

  EventHandlerState estate = myHandler.state();

  // Arrange the logic to take advantage of short-circuit evaluation
  if(!(StellaModTest::isControl(mod) || StellaModTest::isShift(mod) || StellaModTest::isAlt(mod)))
  {
    // Handle keys which switch eventhandler state
    if (!pressed && myHandler.changeStateByEvent(myKeyMap.get(kEmulationMode, key, mod)))
      return;
  }

  // Otherwise, let the event handler deal with it
  switch(estate)
  {
    case EventHandlerState::EMULATION:
    case EventHandlerState::PAUSE:
      myHandler.handleEvent(myKeyMap.get(kEmulationMode, key, mod), pressed, repeated);
      break;

    default:
    #ifdef GUI_SUPPORT
      if (myHandler.hasOverlay())
        myHandler.overlay().handleKeyEvent(key, mod, pressed, repeated);
    #endif
      myHandler.handleEvent(myKeyMap.get(kMenuMode, key, mod), pressed, repeated);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalKeyboardHandler::handleAltEvent(StellaKey key, StellaMod mod, bool pressed)
{
  if(StellaModTest::isAlt(mod) && pressed && key == KBDK_TAB)
  {
    // Swallow Alt-Tab, but remember that it happened
    myAltKeyCounter = 1;
    return true;
  } // alt

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventSet PhysicalKeyboardHandler::LeftJoystickEvents = {
  Event::JoystickZeroUp, Event::JoystickZeroDown, Event::JoystickZeroLeft, Event::JoystickZeroRight,
  Event::JoystickZeroFire, Event::JoystickZeroFire5, Event::JoystickZeroFire9,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventSet PhysicalKeyboardHandler::RightJoystickEvents = {
  Event::JoystickOneUp, Event::JoystickOneDown, Event::JoystickOneLeft, Event::JoystickOneRight,
  Event::JoystickOneFire, Event::JoystickOneFire5, Event::JoystickOneFire9
};


EventSet PhysicalKeyboardHandler::LeftPaddlesEvents = {
  Event::PaddleZeroDecrease, Event::PaddleZeroIncrease, Event::PaddleZeroAnalog, Event::PaddleZeroFire,
  Event::PaddleOneDecrease, Event::PaddleOneIncrease, Event::PaddleOneAnalog, Event::PaddleOneFire,
};

EventSet PhysicalKeyboardHandler::RightPaddlesEvents = {
  Event::PaddleTwoDecrease, Event::PaddleTwoIncrease, Event::PaddleTwoAnalog, Event::PaddleTwoFire,
  Event::PaddleThreeDecrease, Event::PaddleThreeIncrease, Event::PaddleThreeAnalog, Event::PaddleThreeFire,
};


EventSet PhysicalKeyboardHandler::LeftKeypadEvents = {
  Event::KeyboardZero1, Event::KeyboardZero2, Event::KeyboardZero3,
  Event::KeyboardZero4, Event::KeyboardZero5, Event::KeyboardZero6,
  Event::KeyboardZero7, Event::KeyboardZero8, Event::KeyboardZero9,
  Event::KeyboardZeroStar, Event::KeyboardZero0, Event::KeyboardZeroPound,
};

EventSet PhysicalKeyboardHandler::RightKeypadEvents = {
  Event::KeyboardOne1, Event::KeyboardOne2, Event::KeyboardOne3,
  Event::KeyboardOne4, Event::KeyboardOne5, Event::KeyboardOne6,
  Event::KeyboardOne7, Event::KeyboardOne8, Event::KeyboardOne9,
  Event::KeyboardOneStar, Event::KeyboardOne0, Event::KeyboardOnePound,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalKeyboardHandler::EventMappingArray PhysicalKeyboardHandler::DefaultEmuMapping = {
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
  {Event::VolumeDecrease,           KBDK_LEFTBRACKET, MOD3},
  {Event::VolumeIncrease,           KBDK_RIGHTBRACKET, MOD3},
  {Event::SoundToggle,              KBDK_RIGHTBRACKET, KBDM_CTRL},

  {Event::ToggleFullScreen,         KBDK_RETURN, MOD3},
  {Event::DecreaseOverscan,         KBDK_PAGEDOWN, MOD3},
  {Event::IncreaseOverScan,         KBDK_PAGEUP, MOD3},
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
  {Event::DecreasePhosphor,         KBDK_9, KBDM_SHIFT | MOD3},
  {Event::IncreasePhosphor,         KBDK_9, MOD3},
  {Event::TogglePhosphor,           KBDK_P, MOD3},
  {Event::ScanlinesDecrease,        KBDK_0, KBDM_SHIFT | MOD3},
  {Event::ScanlinesIncrease,        KBDK_0, MOD3},
  {Event::ToggleColorLoss,          KBDK_L, KBDM_CTRL},
  {Event::TogglePalette,            KBDK_P, KBDM_CTRL},
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
  {Event::DecreaseFormat,           KBDK_F, KBDM_SHIFT | KBDM_CTRL},
  {Event::IncreaseFormat,           KBDK_F, KBDM_CTRL},

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

PhysicalKeyboardHandler::EventMappingArray PhysicalKeyboardHandler::DefaultMenuMapping = {
  {Event::UIUp,                     KBDK_UP},
  {Event::UIDown,                   KBDK_DOWN},
  {Event::UILeft,                   KBDK_LEFT},
  {Event::UIRight,                  KBDK_RIGHT},

  {Event::UIHome,                   KBDK_HOME},
  {Event::UIEnd,                    KBDK_END},
  {Event::UIPgUp,                   KBDK_PAGEUP},
  {Event::UIPgDown,                 KBDK_PAGEDOWN},

  {Event::UISelect,                 KBDK_RETURN},
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

PhysicalKeyboardHandler::EventMappingArray PhysicalKeyboardHandler::DefaultJoystickMapping = {
  {Event::JoystickZeroUp,           KBDK_UP},
  {Event::JoystickZeroDown,         KBDK_DOWN},
  {Event::JoystickZeroLeft,         KBDK_LEFT},
  {Event::JoystickZeroRight,        KBDK_RIGHT},
  {Event::JoystickZeroFire,         KBDK_SPACE},
  {Event::JoystickZeroFire,         KBDK_LCTRL},
  {Event::JoystickZeroFire5,        KBDK_4},
  {Event::JoystickZeroFire9,        KBDK_5},

  {Event::JoystickOneUp,            KBDK_Y},
  {Event::JoystickOneDown,          KBDK_H},
  {Event::JoystickOneLeft,          KBDK_G},
  {Event::JoystickOneRight,         KBDK_J},
  {Event::JoystickOneFire,          KBDK_F},
  {Event::JoystickOneFire5,         KBDK_6},
  {Event::JoystickOneFire9,         KBDK_7},
};

PhysicalKeyboardHandler::EventMappingArray PhysicalKeyboardHandler::DefaultPaddleMapping = {
  {Event::PaddleZeroDecrease,       KBDK_RIGHT},
  {Event::PaddleZeroIncrease,       KBDK_LEFT},
  {Event::PaddleZeroFire,           KBDK_LCTRL},

  {Event::PaddleOneDecrease,        KBDK_DOWN},
  {Event::PaddleOneIncrease,        KBDK_UP},
  {Event::PaddleOneFire,            KBDK_4},

  {Event::PaddleTwoDecrease,        KBDK_J},
  {Event::PaddleTwoIncrease,        KBDK_G},
  {Event::PaddleTwoFire,            KBDK_F},

  {Event::PaddleThreeDecrease,      KBDK_H},
  {Event::PaddleThreeIncrease,      KBDK_Y},
  {Event::PaddleThreeFire,          KBDK_6},
};

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

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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalKeyboardHandler::PhysicalKeyboardHandler(
      OSystem& system, EventHandler& handler, Event& event)
  : myOSystem(system),
    myHandler(handler),
    myEvent(event),
    myAltKeyCounter(0),
    myUseCtrlKeyFlag(myOSystem.settings().getBool("ctrlcombo"))
{
  string list = myOSystem.settings().getString("keymap_emu");
  myKeyMap.loadMapping(list, kEmulationMode);
  list = myOSystem.settings().getString("keymap_ui");
  myKeyMap.loadMapping(list, kMenuMode);

  setDefaultMapping(Event::NoType, kEmulationMode, true);
  setDefaultMapping(Event::NoType, kMenuMode, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::setDefaultMapping(Event::Type event, EventMode mode, bool updateDefaults)
{
  // If event is 'NoType', erase and reset all mappings
    // Otherwise, only reset the given event
  bool eraseAll = !updateDefaults && (event == Event::NoType);
  if (eraseAll)
    // Erase all mappings of given mode
    myKeyMap.eraseMode(mode);
  else
    myKeyMap.eraseEvent(event, mode);

  auto setDefaultKey = [&](Event::Type k_event, StellaKey key, int mod = KBDM_NONE)
  {
    if (updateDefaults)
    {
      // if there is no existing mapping for the event or
      //  the default mapping for the event is unused, set default key for event
      if (myKeyMap.getEventMapping(k_event, mode).size() == 0 ||
          !myKeyMap.check(mode, key, mod))
      {
        myKeyMap.add(k_event, mode, key, mod);
      }
    }
    else if (eraseAll || k_event == event)
    {
      myKeyMap.add(k_event, mode, key, mod);
    }
  };

  switch(mode)
  {
    case kEmulationMode:
      setDefaultKey(Event::KeyboardZero1      , KBDK_1);
      setDefaultKey(Event::KeyboardZero2      , KBDK_2);
      setDefaultKey(Event::KeyboardZero3      , KBDK_3);
      setDefaultKey(Event::KeyboardZero4      , KBDK_Q);
      setDefaultKey(Event::KeyboardZero5      , KBDK_W);
      setDefaultKey(Event::KeyboardZero6      , KBDK_E);
      setDefaultKey(Event::KeyboardZero7      , KBDK_A);
      setDefaultKey(Event::KeyboardZero8      , KBDK_S);
      setDefaultKey(Event::KeyboardZero9      , KBDK_D);
      setDefaultKey(Event::KeyboardZeroStar   , KBDK_Z);
      setDefaultKey(Event::KeyboardZero0      , KBDK_X);
      setDefaultKey(Event::KeyboardZeroPound  , KBDK_C);

      setDefaultKey(Event::KeyboardOne1       , KBDK_8);
      setDefaultKey(Event::KeyboardOne2       , KBDK_9);
      setDefaultKey(Event::KeyboardOne3       , KBDK_0);
      setDefaultKey(Event::KeyboardOne4       , KBDK_I);
      setDefaultKey(Event::KeyboardOne5       , KBDK_O);
      setDefaultKey(Event::KeyboardOne6       , KBDK_P);
      setDefaultKey(Event::KeyboardOne7       , KBDK_K);
      setDefaultKey(Event::KeyboardOne8       , KBDK_L);
      setDefaultKey(Event::KeyboardOne9       , KBDK_SEMICOLON);
      setDefaultKey(Event::KeyboardOneStar    , KBDK_COMMA);
      setDefaultKey(Event::KeyboardOne0       , KBDK_PERIOD);
      setDefaultKey(Event::KeyboardOnePound   , KBDK_SLASH);

      setDefaultKey(Event::JoystickZeroUp     , KBDK_UP);
      setDefaultKey(Event::JoystickZeroDown   , KBDK_DOWN);
      setDefaultKey(Event::JoystickZeroLeft   , KBDK_LEFT);
      setDefaultKey(Event::JoystickZeroRight  , KBDK_RIGHT);
      setDefaultKey(Event::JoystickZeroFire   , KBDK_SPACE);
      setDefaultKey(Event::JoystickZeroFire   , KBDK_LCTRL);
      setDefaultKey(Event::JoystickZeroFire5  , KBDK_4);
      setDefaultKey(Event::JoystickZeroFire9  , KBDK_5);

      setDefaultKey(Event::JoystickOneUp      , KBDK_Y);
      setDefaultKey(Event::JoystickOneDown    , KBDK_H);
      setDefaultKey(Event::JoystickOneLeft    , KBDK_G);
      setDefaultKey(Event::JoystickOneRight   , KBDK_J);
      setDefaultKey(Event::JoystickOneFire    , KBDK_F);
      setDefaultKey(Event::JoystickOneFire5   , KBDK_6);
      setDefaultKey(Event::JoystickOneFire9   , KBDK_7);

      setDefaultKey(Event::ConsoleSelect      , KBDK_F1);
      setDefaultKey(Event::ConsoleReset       , KBDK_F2);
      setDefaultKey(Event::ConsoleColor       , KBDK_F3);
      setDefaultKey(Event::ConsoleBlackWhite  , KBDK_F4);
      setDefaultKey(Event::ConsoleLeftDiffA   , KBDK_F5);
      setDefaultKey(Event::ConsoleLeftDiffB   , KBDK_F6);
      setDefaultKey(Event::ConsoleRightDiffA  , KBDK_F7);
      setDefaultKey(Event::ConsoleRightDiffB  , KBDK_F8);
      setDefaultKey(Event::SaveState          , KBDK_F9);
      setDefaultKey(Event::ChangeState        , KBDK_F10);
      setDefaultKey(Event::LoadState          , KBDK_F11);
      setDefaultKey(Event::TakeSnapshot       , KBDK_F12);
      setDefaultKey(Event::Fry                , KBDK_BACKSPACE);
      setDefaultKey(Event::PauseMode          , KBDK_PAUSE);
      setDefaultKey(Event::OptionsMenuMode    , KBDK_TAB);
      setDefaultKey(Event::CmdMenuMode        , KBDK_BACKSLASH);
      setDefaultKey(Event::TimeMachineMode    , KBDK_T);
      setDefaultKey(Event::DebuggerMode       , KBDK_GRAVE);
      setDefaultKey(Event::LauncherMode       , KBDK_ESCAPE);

      setDefaultKey(Event::VidmodeDecrease    , KBDK_MINUS, KBDM_ALT);
      setDefaultKey(Event::VidmodeIncrease    , KBDK_EQUALS, KBDM_ALT);
      setDefaultKey(Event::VolumeDecrease     , KBDK_LEFTBRACKET, KBDM_ALT);
      setDefaultKey(Event::VolumeIncrease     , KBDK_RIGHTBRACKET, KBDM_ALT);
      setDefaultKey(Event::SoundToggle        , KBDK_RIGHTBRACKET, KBDM_CTRL);

      setDefaultKey(Event::ToggleFullScreen   , KBDK_RETURN, KBDM_ALT);
      setDefaultKey(Event::VidmodeStd         , KBDK_1, KBDM_ALT);
      setDefaultKey(Event::VidmodeRGB         , KBDK_2, KBDM_ALT);
      setDefaultKey(Event::VidmodeSVideo      , KBDK_3, KBDM_ALT);
      setDefaultKey(Event::VidModeComposite   , KBDK_4, KBDM_ALT);
      setDefaultKey(Event::VidModeBad         , KBDK_5, KBDM_ALT);
      setDefaultKey(Event::VidModeCustom      , KBDK_6, KBDM_ALT);
      setDefaultKey(Event::ScanlinesDecrease  , KBDK_7, KBDM_SHIFT | KBDM_ALT);
      setDefaultKey(Event::ScanlinesIncrease  , KBDK_7, KBDM_ALT);
      setDefaultKey(Event::PreviousAttribute  , KBDK_9, KBDM_SHIFT | KBDM_ALT);
      setDefaultKey(Event::NextAttribute      , KBDK_9, KBDM_ALT);
      setDefaultKey(Event::DecreaseAttribute  , KBDK_0, KBDM_SHIFT | KBDM_ALT);
      setDefaultKey(Event::IncreaseAttribute  , KBDK_0, KBDM_ALT);

    // FIXME - use the R77 define in the final release
    //         use the '1' define for testing
    #if defined(RETRON77)
//    #if 1
      setDefaultKey(Event::ConsoleColorToggle     , KBDK_F4);         // back ("COLOR","B/W")
      setDefaultKey(Event::ConsoleLeftDiffToggle  , KBDK_F6);         // front ("SKILL P1")
      setDefaultKey(Event::ConsoleRightDiffToggle , KBDK_F8);         // front ("SKILL P2")
      setDefaultKey(Event::CmdMenuMode            , KBDK_F13);        // back ("4:3","16:9")
      setDefaultKey(Event::LauncherMode           , KBDK_BACKSPACE);  // back ("FRY")
    #endif
      break;

    case kMenuMode:
      setDefaultKey(Event::UIUp      , KBDK_UP);
      setDefaultKey(Event::UIDown    , KBDK_DOWN);
      setDefaultKey(Event::UILeft    , KBDK_LEFT);
      setDefaultKey(Event::UIRight   , KBDK_RIGHT);

      setDefaultKey(Event::UIHome    , KBDK_HOME);
      setDefaultKey(Event::UIEnd     , KBDK_END);
      setDefaultKey(Event::UIPgUp    , KBDK_PAGEUP);
      setDefaultKey(Event::UIPgDown  , KBDK_PAGEDOWN);

      setDefaultKey(Event::UISelect  , KBDK_RETURN);
      setDefaultKey(Event::UICancel  , KBDK_ESCAPE);

      setDefaultKey(Event::UINavPrev , KBDK_TAB, KBDM_SHIFT);
      setDefaultKey(Event::UINavNext , KBDK_TAB);
      setDefaultKey(Event::UITabPrev , KBDK_TAB, KBDM_SHIFT|KBDM_CTRL);
      setDefaultKey(Event::UITabNext , KBDK_TAB, KBDM_CTRL);

      setDefaultKey(Event::UIPrevDir , KBDK_BACKSPACE);
      setDefaultKey(Event::ToggleFullScreen, KBDK_RETURN, KBDM_ALT);

    // FIXME - use the R77 define in the final release
    //         use the '1' define for testing
    #if defined(RETRON77)
//    #if 1
      setDefaultKey(Event::UIUp       , KBDK_F9);         // front ("SAVE")
      setDefaultKey(Event::UIDown     , KBDK_F2);         // front ("RESET")
      setDefaultKey(Event::UINavPrev  , KBDK_F11);        // front ("LOAD")
      setDefaultKey(Event::UINavNext  , KBDK_F1);         // front ("MODE")
      setDefaultKey(Event::UISelect   , KBDK_F6);         // front ("SKILL P1")
      setDefaultKey(Event::UICancel   , KBDK_F8);         // front ("SKILL P2")
      //setDefaultKey(Event::NoType     , KBDK_F4);         // back ("COLOR","B/W")
      setDefaultKey(Event::UITabPrev  , KBDK_F13);        // back ("4:3","16:9")
      setDefaultKey(Event::UITabNext  , KBDK_BACKSPACE);  // back (FRY)
    #endif
      break;

    default:
      return;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::eraseMapping(Event::Type event, EventMode mode)
{
  myKeyMap.eraseEvent(event, mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::saveMapping()
{
  myOSystem.settings().setValue("keymap_emu", myKeyMap.saveMapping(kEmulationMode));
  myOSystem.settings().setValue("keymap_ui", myKeyMap.saveMapping(kMenuMode));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string PhysicalKeyboardHandler::getMappingDesc(Event::Type event, EventMode mode) const
{
  return myKeyMap.getEventMappingDesc(event, mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalKeyboardHandler::addMapping(Event::Type event, EventMode mode,
                                         StellaKey key, StellaMod mod)
{
  // These keys cannot be remapped
  if(Event::isAnalog(event))
    return false;
  else
    myKeyMap.add(event, mode, key, mod);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalKeyboardHandler::handleEvent(StellaKey key, StellaMod mod, bool pressed)
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
  if(handleAltEvent(key, mod, pressed) || handleControlEvent(key, mod, pressed))
    return;

  EventHandlerState estate = myHandler.state();

  // Arrange the logic to take advantage of short-circuit evaluation
  if(!(StellaModTest::isControl(mod) || StellaModTest::isShift(mod) || StellaModTest::isAlt(mod)))
  {
    // Special handling for Escape key
    // Basically, exit whichever mode we're currently in
    if(pressed && key == KBDK_ESCAPE)
    {
      switch(estate)
      {
        case EventHandlerState::PAUSE:
          myHandler.changeStateByEvent(Event::PauseMode);
          return;
        case EventHandlerState::CMDMENU:
          myHandler.changeStateByEvent(Event::CmdMenuMode);
          return;
        case EventHandlerState::TIMEMACHINE:
          myHandler.changeStateByEvent(Event::TimeMachineMode);
          return;
#if 0 // FIXME - exits ROM too, when it should just go back to ROM
        case EventHandlerState::DEBUGGER:
          myHandler.changeStateByEvent(Event::DebuggerMode);
          return;
#endif
        default:
          break;
      }
    }

    // Handle keys which switch eventhandler state
    if (!pressed && myHandler.changeStateByEvent(myKeyMap.get(kEmulationMode, key, mod)))
      return;
  }

  // Otherwise, let the event handler deal with it
  switch(estate)
  {
    case EventHandlerState::EMULATION:
    case EventHandlerState::PAUSE:
      myHandler.handleEvent(myKeyMap.get(kEmulationMode, key, mod), pressed);
      break;

    default:
    #ifdef GUI_SUPPORT
      if(myHandler.hasOverlay())
        myHandler.overlay().handleKeyEvent(key, mod, pressed);
    #endif
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalKeyboardHandler::handleAltEvent(StellaKey key, StellaMod mod, bool pressed)
{
  bool handled = true;

  if(StellaModTest::isAlt(mod) && pressed)
  {
    EventHandlerState estate = myHandler.state();
#ifdef BSPF_MACOS
    // These keys work in all states
    if(key == KBDK_Q)
    {
      myHandler.handleEvent(Event::Quit);
    }
    else
#endif

    if (key == KBDK_RETURN)
    {
      int i = 0;
    }

    if(key == KBDK_TAB)
    {
      // Swallow Alt-Tab, but remember that it happened
      myAltKeyCounter = 1;
      return true;
    }
    // State rewinding must work in pause mode too
    else if(estate == EventHandlerState::EMULATION || estate == EventHandlerState::PAUSE)
    {
      switch(key)
      {
        case KBDK_LEFT:  // Alt-left(-shift) rewinds 1(10) states
          myHandler.enterTimeMachineMenuMode((StellaModTest::isShift(mod) && pressed) ? 10 : 1, false);
          break;

        case KBDK_RIGHT:  // Alt-right(-shift) unwinds 1(10) states
          myHandler.enterTimeMachineMenuMode((StellaModTest::isShift(mod) && pressed) ? 10 : 1, true);
          break;

        case KBDK_DOWN:  // Alt-down rewinds to start of list
          myHandler.enterTimeMachineMenuMode(1000, false);
          break;

        case KBDK_UP:  // Alt-up rewinds to end of list
          myHandler.enterTimeMachineMenuMode(1000, true);
          break;

        case KBDK_PAGEUP:    // Alt-PageUp increases YStart
          myOSystem.console().changeYStart(+1);
          break;

        case KBDK_PAGEDOWN:  // Alt-PageDown decreases YStart
          myOSystem.console().changeYStart(-1);
          break;

        case KBDK_Z:
          if(StellaModTest::isShift(mod))
            myOSystem.console().toggleP0Collision();
          else
            myOSystem.console().toggleP0Bit();
          break;

        case KBDK_X:
          if(StellaModTest::isShift(mod))
            myOSystem.console().toggleP1Collision();
          else
            myOSystem.console().toggleP1Bit();
          break;

        case KBDK_C:
          if(StellaModTest::isShift(mod))
            myOSystem.console().toggleM0Collision();
          else
            myOSystem.console().toggleM0Bit();
          break;

        case KBDK_V:
          if(StellaModTest::isShift(mod))
            myOSystem.console().toggleM1Collision();
          else
            myOSystem.console().toggleM1Bit();
          break;

        case KBDK_B:
          if(StellaModTest::isShift(mod))
            myOSystem.console().toggleBLCollision();
          else
            myOSystem.console().toggleBLBit();
          break;

        case KBDK_N:
          if(StellaModTest::isShift(mod))
            myOSystem.console().togglePFCollision();
          else
            myOSystem.console().togglePFBit();
          break;

        case KBDK_COMMA:
          myOSystem.console().toggleFixedColors();
          break;

        case KBDK_PERIOD:
          if(StellaModTest::isShift(mod))
            myOSystem.console().toggleCollisions();
          else
            myOSystem.console().toggleBits();
          break;

        case KBDK_I:  // Alt-i decreases phosphor blend
          myOSystem.console().changePhosphor(-1);
          break;

        case KBDK_O:  // Alt-o increases phosphor blend
          myOSystem.console().changePhosphor(+1);
          break;

        case KBDK_P:  // Alt-p toggles phosphor effect
          myOSystem.console().togglePhosphor();
          break;

        case KBDK_J:  // Alt-j toggles scanline jitter
          myOSystem.console().toggleJitter();
          break;

        case KBDK_L:
          myOSystem.frameBuffer().toggleFrameStats();
          break;

        case KBDK_T:  // Alt-t toggles Time Machine
          myOSystem.state().toggleTimeMachine();
          break;

    #ifdef PNG_SUPPORT
        case KBDK_S:
          myOSystem.png().toggleContinuousSnapshots(StellaModTest::isShift(mod));
          break;
    #endif

        default:
          handled = false;
          break;
      } // switch
    }
    else
      handled = false;
  } // alt
  else
    handled = false;

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalKeyboardHandler::handleControlEvent(StellaKey key, StellaMod mod, bool pressed)
{
  bool handled = true;

  if(StellaModTest::isControl(mod) && pressed && myUseCtrlKeyFlag)
  {
    EventHandlerState estate = myHandler.state();
    // These keys work in all states
    if(key == KBDK_Q)
    {
      myHandler.handleEvent(Event::Quit);
    }
    // These only work when in emulation mode
    else if(estate == EventHandlerState::EMULATION || estate == EventHandlerState::PAUSE)
    {
      switch(key)
      {
        case KBDK_0:  // Ctrl-0 switches between mouse control modes
          myHandler.handleMouseControl();
          break;

        case KBDK_1:  // Ctrl-1 swaps Stelladaptor/2600-daptor ports
          myHandler.toggleSAPortOrder();
          break;

        case KBDK_F:  // (Shift) Ctrl-f toggles NTSC/PAL/SECAM mode
          myOSystem.console().toggleFormat(StellaModTest::isShift(mod) ? -1 : 1);
          break;

        case KBDK_G:  // Ctrl-g (un)grabs mouse
          if(!myOSystem.frameBuffer().fullScreen())
          {
            myOSystem.frameBuffer().toggleGrabMouse();
            myOSystem.frameBuffer().showMessage(myOSystem.frameBuffer().grabMouseEnabled()
                                                ? "Grab mouse enabled" : "Grab mouse disabled");
          }
          break;

        case KBDK_L:  // Ctrl-l toggles PAL color-loss effect
          myOSystem.console().toggleColorLoss();
          break;

        case KBDK_P:  // Ctrl-p toggles different palettes
          myOSystem.console().togglePalette();
          break;

        case KBDK_R:  // Ctrl-r reloads the currently loaded ROM
          myOSystem.reloadConsole();
          break;

        default:
          handled = false;
          break;
      } // switch
    }
    else
      handled = false;
  } // control
  else
    handled = false;

  return handled;
}
